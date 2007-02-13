#!/usr/bin/python2.4
# xdelta 3 - delta compression tools and library
# Copyright (C) 2003, 2006, 2007.  Joshua P. MacDonald
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# TODO: Test IOPT (1.5 vs. greedy)

# TODO: Start testing window sizes

# TODO: Note: xd3_encode_memory is underperforming the command-line
#   at run-speed tests (due to excess memory allocation?). Fix.

import os, sys, math, re, time, types, array, random
import xdelta3main
import xdelta3

HIST_SIZE      = 10   # the number of buckets
MIN_SIZE       = 0

TIME_TOO_SHORT = 0.050

SKIP_TRIALS    = 2
MIN_TRIALS     = 3
MAX_TRIALS     = 15

#SKIP_TRIALS    = 0
#MIN_TRIALS     = 1
#MAX_TRIALS     = 1

MIN_STDDEV_PCT = 1.5 # stop
MAX_RUN        = 1000 * 1000 * 10

# How many results per round
MAX_RESULTS = 100
KEEP_P = (0.5)
FAST_P = (0.0)
SLOW_P = (0.0)
FILE_P = (0.30)

CONFIG_ORDER = [ 'large_look',
                 'large_step',
                 'small_look',
                 'small_chain',
                 'small_lchain',
                 'ssmatch',
                 'trylazy',
                 'max_lazy',
                 'long_enough',
                 'promote' ]

def INPUT_SPEC(rand):
    return {
    'large_look' : lambda d: rand.choice([9, 11, 13, 15]),
    'large_step' : lambda d: rand.choice([11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 ]),

    'small_chain'  : lambda d: rand.choice([1]),
    'small_lchain' : lambda d: rand.choice([1]),

    'max_lazy'     : lambda d: rand.choice([9, 13, 18]),
    'long_enough'  : lambda d: rand.choice([9, 13, 18]),

    'small_look'   : lambda d: rand.choice([4]),
    'promote'      : lambda d: 0,
    'trylazy'      : lambda d: 1,
    'ssmatch'      : lambda d: 0,
    }


#
#
#RCSDIR = '/mnt/polaroid/Polaroid/orbit_linux/home/jmacd/PRCS'
RCSDIR = '/tmp/PRCS_read_copy'
#RCSDIR = 'G:/jmacd/PRCS'

SAMPLEDIR = "C:/sample_data/WESNOTH_tmp/tar"

TMPDIR = '/tmp/xd3regtest.%d' % os.getpid()

RUNFILE = os.path.join(TMPDIR, 'run')
HFILE   = os.path.join(TMPDIR, 'hdr')
DFILE   = os.path.join(TMPDIR, 'output')
RFILE   = os.path.join(TMPDIR, 'recon')

HEAD_STATE = 0
BAR_STATE  = 1
REV_STATE  = 2
DATE_STATE = 3

# 
IGNORE_FILENAME  = re.compile('.*\\.(gif|jpg).*')

# rcs output
RE_TOTREV  = re.compile('total revisions: (\\d+)')
RE_BAR     = re.compile('----------------------------')
RE_REV     = re.compile('revision (.+)')
RE_DATE    = re.compile('date: ([^;]+);.*')
# xdelta output
RE_HDRSZ   = re.compile('VCDIFF header size: +(\\d+)')
RE_EXTCOMP = re.compile('XDELTA ext comp.*')

# Testwide defaults
testwide_encode_args = [

    # secondary compression on or off
    #'-S', 'djw',

    # do not measure instruction buffer effects
    '-I', '0',

    # do not attempt external decompression
    '-D'
    ]

def c2s(c):
    return ' '.join(['%02d' % x for x in c])

#
# exceptions
class SkipRcsException:
    def __init__(self,reason):
        self.reason = reason
class NotEnoughVersions:
    def __init__(self):
        pass
class CommandError:
    def __init__(self,cmd,str):
        if type(cmd) is types.TupleType or \
           type(cmd) is types.ListType:
            cmd = reduce(lambda x,y: '%s %s' % (x,y),cmd)
        print 'command was: ',cmd
        print 'command failed: ',str
        print 'have fun debugging'
#
# one version
class RcsVersion:
    def __init__(self,vstr):
        self.vstr = vstr
    def __cmp__(self,other):
        return cmp(self.date, other.date)
    def __str__(self):
        return str(self.vstr)
#
# one rcsfile
class RcsFile:

    def __init__(self, fname):
        self.fname    = fname
        self.versions = []
        self.state    = HEAD_STATE

    def SetTotRev(self,s):
        self.totrev = int(s)

    def Rev(self,s):
        self.rev = RcsVersion(s)
        if len(self.versions) >= self.totrev:
            raise SkipRcsException('too many versions (in log messages)')
        self.versions.append(self.rev)

    def Date(self,s):
        self.rev.date = s

    def Match(self, line, state, rx, gp, newstate, f):
        if state == self.state:
            m = rx.match(line)
            if m:
                if f:
                    f(m.group(gp))
                self.state = newstate
                return 1
        return None

    def Sum1Rlog(self):
        f = os.popen('rlog '+self.fname, "r")
        l = f.readline()
        while l:
            if self.Match(l, HEAD_STATE, RE_TOTREV, 1, BAR_STATE, self.SetTotRev):
                pass
            elif self.Match(l, BAR_STATE, RE_BAR, 1, REV_STATE, None):
                pass
            elif self.Match(l, REV_STATE, RE_REV, 1, DATE_STATE, self.Rev):
                pass
            elif self.Match(l, DATE_STATE, RE_DATE, 1, BAR_STATE, self.Date):
                pass
            l = f.readline()
        c = f.close()
        if c != None:
            raise c
        #print '%s versions %d' % (self.fname, len(self.versions))
        #for v in self.versions:
        #    v.Print()

    def Sum1(self):
        st = os.stat(self.fname)
        self.rcssize = st.st_size
        self.Sum1Rlog()
        if self.totrev != len(self.versions):
            raise SkipRcsException('wrong version count')
        self.versions.sort()

    def Checkout(self,n):
        v      = self.versions[n]
        out    = open(self.Verf(n), "w")
        cmd    = 'co -ko -p%s %s' % (v.vstr, self.fname)
        total  = 0
        (inf,
         stream,
         err)  = os.popen3(cmd, "r")
        inf.close()
        buf    = stream.read()
        while buf:
            total = total + len(buf)
            out.write(buf)
            buf = stream.read()
        v.vsize = total
        estr = ''
        buf = err.read()
        while buf:
            estr = estr + buf
            buf = err.read()
        if stream.close():
            raise CommandError(cmd, 'checkout failed: %s\n%s\n%s' % (v.vstr, self.fname, estr))
        out.close()
        err.close()

    def Vdate(self,n):
        return self.versions[n].date

    def Vstr(self,n):
        return self.versions[n].vstr

    def Verf(self,n):
        return os.path.join(TMPDIR, 'input.%d' % n)

    def PairsByDate(self,runnable):
        if self.totrev < 2:
            raise NotEnoughVersions()
        self.Checkout(0)
        ntrials = []
        if self.totrev < 2:
            return vtrials
        for v in range(0,self.totrev-1):
            if v > 1:
                os.remove(self.Verf(v-1))
            self.Checkout(v+1)
            if os.stat(self.Verf(v)).st_size < MIN_SIZE or \
               os.stat(self.Verf(v+1)).st_size < MIN_SIZE:
                continue

            runnable.SetInputs(self.Verf(v),
                               self.Vstr(v),
                               self.Verf(v+1),
                               self.Vstr(v+1))
            result = TimedTest(runnable)
            print 'testing %s %s: ratio %.3f%%: time %.7f: in %u trials' % \
                  (os.path.basename(self.fname),
                   self.Vstr(v+1),
                   result.r1.ratio,
                   result.time.mean,
                   result.trials)
            ntrials.append(result)

        os.remove(self.Verf(self.totrev-1))
        os.remove(self.Verf(self.totrev-2))
        return ntrials

    def AppendVersion(self, f, n):
        self.Checkout(n)
        rf = open(self.Verf(n), "r")
        data = rf.read()
        f.write(data)
        rf.close()
        return len(data)

#
# This class recursively scans a directory for rcsfiles
class RcsFinder:
    def __init__(self):
        self.subdirs  = []
        self.rcsfiles = []
        self.others   = []
        self.skipped  = []

    def Scan1(self,dir):
        dents = os.listdir(dir)
        subdirs  = []
        rcsfiles = []
        others   = []
        for dent in dents:
            full = os.path.join(dir, dent)
            if os.path.isdir(full):
                subdirs.append(full)
            elif dent[len(dent)-2:] == ",v":
                rcsfiles.append(RcsFile(full))
            else:
                others.append(full)
        self.subdirs  = self.subdirs  + subdirs
        self.rcsfiles = self.rcsfiles + rcsfiles
        self.others   = self.others   + others
        return subdirs

    def Crawl(self, dir):
        subdirs = [dir]
        while subdirs:
            s1 = self.Scan1(subdirs[0])
            subdirs = subdirs[1:] + s1

    def Summarize(self):
        good = []
        for rf in self.rcsfiles:
            try:
                rf.Sum1()
                if rf.totrev < 2:
                    raise SkipRcsException('too few versions (< 2)')
            except SkipRcsException, e:
                #print 'skipping file %s: %s' % (rf.fname, e.reason)
                self.skipped.append(rf)
            else:
                good.append(rf)
        self.rcsfiles = good

    def PairsByDate(self,runnable):
        allvtrials = []
        good = []
        for rf in self.rcsfiles:
            print 'testing %s on %s with %d versions' % (runnable.type, rf.fname, rf.totrev)
            try:
                allvtrials.append(rf.PairsByDate(runnable))
            except SkipRcsException:
                print 'file %s has compressed versions: skipping' % (rf.fname)
            except NotEnoughVersions:
                print 'testing %s on %s: not enough versions' % (runnable.type, rf.fname)
            else:
                good.append(rf)
        self.rcsfiles = good
        return allvtrials
#
#
class Bucks:
    def __init__(self,low,high):
        self.low    = low
        self.high   = high
        self.spread = high - low
        self.bucks  = []
        for i in range(0,HIST_SIZE):
            self.bucks.append([low+(self.spread * (i+0.0) / float(HIST_SIZE)),
                               low+(self.spread * (i+0.5) / float(HIST_SIZE)),
                               low+(self.spread * (i+1.0) / float(HIST_SIZE)),
                               0])
    def Add(self, x):
        assert(x>=self.low)
        assert(x<self.high)
        t = self.bucks[int((x-self.low)/float(self.spread)*HIST_SIZE)]
        t[3] = t[3] + 1
    def Print(self, f):
        for i in self.bucks:
            # gnuplot -persist "plot %s using 2:4
            f.write("%.1f %.1f %.1f %d\n" % (i[0],i[1],i[2],i[3]))
#
#
class TimedTest:
    def __init__(self,runnable,
                 skip_trials=SKIP_TRIALS,
                 min_trials=MIN_TRIALS,
                 max_trials=MAX_TRIALS,
                 min_stddev_pct=MIN_STDDEV_PCT):

        min_trials = min(min_trials,max_trials)
        self.trials   = 0
        self.measured = []
        self.r1       = None
        while 1:
            try:
                os.remove(DFILE)
                os.remove(RFILE)
            except OSError:
                pass

            start_time  = time.time()
            start_clock = time.clock()

            result = runnable.Run(self.trials)

            if self.r1 == None:
                self.r1 = result

            total_clock = (time.clock() - start_clock)
            total_time  = (time.time()  - start_time)

            elap_time  = max((total_time),  0.000001)
            elap_clock = max((total_clock), 0.000001)

            self.trials = self.trials + 1

            # skip some of the first trials
            if self.trials > skip_trials:
                self.measured.append((elap_clock,elap_time))
                #print 'measurement total: %.1f ms' % (total_time * 1000.0)

            # at least so many
            if self.trials < (skip_trials + min_trials):
                #print 'continue: need more trials: %d' % self.trials
                continue

            # compute %variance
            done = 0
            if skip_trials + min_trials <= 2:
                done = 1
                self.measured = self.measured + self.measured;

            self.time = StatList([x[1] for x in self.measured], 'elap time')
            sp = float(self.time.s) / float(self.time.mean)

            # what if MAX_TRIALS is exceeded?
            too_many = (self.trials-skip_trials) >= max_trials
            good     = (100.0 * sp) < min_stddev_pct
            if done or too_many or good:
                self.trials = self.trials - skip_trials
                if not done and not good:
                    #print 'too many trials: %d' % self.trials
                    pass
                self.clock  = StatList([x[0] for x in self.measured], 'elap clock')
                return
#
#
#
def SumList(l):
    return reduce(lambda x,y: x+y, l)
#
# returns (total, mean, stddev, q2 (median),
#          (q3-q1)/2 ("semi-interquartile range"), max-min (spread))
class StatList:
    def __init__(self,l,desc,hist=0):
        cnt = len(l)
        assert(cnt > 1)
        l.sort()
        self.cnt    = cnt
        self.l      = l
        self.total  = SumList(l)
        self.mean   = self.total / float(self.cnt)
        self.s      = math.sqrt(SumList([(x-self.mean) * (x - self.mean) for x in l]) / float(self.cnt-1))
        self.q0     = l[0]
        self.q1     = l[int(self.cnt/4.0+0.5)]
        self.q2     = l[int(self.cnt/2.0+0.5)]
        self.q3     = l[min(self.cnt-1,int((3.0*self.cnt)/4.0+0.5))]
        self.q4     = l[self.cnt-1]+1
        self.hf     = "./%s.hist" % desc
        self.siqr   = (self.q3-self.q1)/2.0;
        self.spread = (self.q4-self.q0)
        self.str    = '%s %d; mean %d; sdev %d; q2 %d; .5(q3-q1) %.1f; spread %d' % \
                      (desc, self.total, self.mean, self.s, self.q2, self.siqr, self.spread)
        if hist:
            f = open(self.hf, "w")
            self.bucks = Bucks(self.q0,self.q4)
            for i in l:
                self.bucks.Add(i)
            self.bucks.Print(f)
            f.close()

def RunCommand(args):
    #print "run command", args
    p = os.spawnvp(os.P_WAIT, args[0], args)
    if p != 0:
        raise CommandError(args, 'exited %d' % p)

def RunCommandIO(args,infn,outfn):
    p = os.fork()
    if p == 0:
        os.dup2(os.open(infn,os.O_RDONLY),0)
        os.dup2(os.open(outfn,os.O_CREAT|os.O_TRUNC|os.O_WRONLY),1)
        os.execvp(args[0], args)
    else:
        s = os.waitpid(p,0)
        o = os.WEXITSTATUS(s[1])
        if not os.WIFEXITED(s[1]) or o != 0:
            raise CommandError(args, 'exited %d' % o)

def RunXdelta3(args):
    try:
        xdelta3main.main(args)
    except Exception, e:
        raise CommandError(args, "xdelta3.main exception")

class GzipInfo:
    def __init__(self,target,delta):
        self.tgtsize = os.stat(target).st_size
        self.dsize   = os.stat(delta).st_size

class Xdelta3Info:
    def __init__(self,target,delta):
        self.tgtsize = os.stat(target).st_size
        self.dsize   = os.stat(delta).st_size
        if self.tgtsize > 0:
            self.ratio = 100.0 * self.dsize / self.tgtsize;
        else:
            self.ratio = 0.0

class Xdelta3ModInfo:
    def __init__(self,target,delta):
        #tmp = open(DFILE, 'w')
        #tmp.write(patch)
        #tmp.close()
        #r3 = xdelta3.xd3_main_cmdline(['xdelta3', 'printhdr', DFILE, RFILE])
        #if r3 != 0:
        #    raise CommandError('memory', 'print failed: %s' % r3)
        #hdr = open(RFILE, 'r').read()
        #print hdr
        self.tgtsize = len(target)
        self.dsize   = len(delta)
        if self.tgtsize > 0:
            self.ratio = 100.0 * self.dsize / self.tgtsize;
        else:
            self.ratio = 0.0

class Xdelta3Pair:
    def __init__(self, extra):
        self.type        = 'xdelta3'
        self.decode_args = '-dqf'
        self.encode_args = '-eqf'
        self.extra       = extra
        self.presrc      = '-s'
        self.canrep      = 1

    def SetInputs(self,old,oldv,new,newv):
        self.old = old
        self.oldv = oldv
        self.new = new
        self.newv = newv
        return self

    def Run(self,trial):

        encode_args =  self.extra + \
                       testwide_encode_args + \
                      [self.encode_args,
                       self.presrc,
                       self.old,
                       self.new,
                       DFILE]

        decode_args = [self.decode_args,
                       self.presrc,
                       self.old,
                       DFILE,
                       RFILE]
        try:
            RunXdelta3(encode_args)
            if trial > 0:
                return None
            self.dinfo = Xdelta3Info(self.new,DFILE)
            if self.dinfo.extcomp:
                raise SkipRcsException('ext comp')
            RunXdelta3(decode_args)
            RunCommand(('cmp',
                        self.new,
                        RFILE))
            return self.dinfo
        except CommandError:
            print 'encode args: %s' % ' '.join(encode_args)
            print 'decode args: %s' % ' '.join(decode_args)
            raise CommandError("Run failed")

def Test():
    rcsf = RcsFinder()
    rcsf.Crawl(RCSDIR)
    if len(rcsf.rcsfiles) == 0:
        sys.exit(1)
    rcsf.Summarize()
    print "rcsfiles: rcsfiles %d; subdirs %d; others %d; skipped %d" % (len(rcsf.rcsfiles),
                                                                        len(rcsf.subdirs),
                                                                        len(rcsf.others),
                                                                        len(rcsf.skipped))
    print StatList([x.rcssize for x in rcsf.rcsfiles], "rcssize", 1).str
    print StatList([x.totrev for x in rcsf.rcsfiles], "totrev", 1).str
    return rcsf

def Decimals(max):
    l = [0]
    step = 1
    while 1:
        r = range(step, step * 10, step)
        l = l + r
        if step * 10 >= max:
            l.append(step * 10)
            break
        step = step * 10
    return l

class Xdelta3Run1:
    def __init__(self,file):
        self.file = file
    def Run(self,trial):
        RunXdelta3(testwide_encode_args +
                   ['-efqW', str(1<<20), self.file, DFILE])
        if trial > 0:
            return None
        return Xdelta3Info(self.file,DFILE)

class Xdelta3Mod1:
    def __init__(self,file):
        self.data = open(file, 'r').read()
    def Run(self,trial):
        r1, patch = xdelta3.xd3_encode_memory(self.data, None, 1000000, 1<<10)
        if r1 != 0:
            raise CommandError('memory', 'encode failed: %s' % r1)
        if trial > 0:
            return None
        r2, data1 = xdelta3.xd3_decode_memory(patch, None, len(self.data))
        if r2 != 0:
            raise CommandError('memory', 'decode failed: %s' % r1)
        if self.data != data1:
            raise CommandError('memory', 'bad output: %s' % self.data, data1)
        return Xdelta3ModInfo(self.data,patch)

class GzipRun1:
    def __init__(self,file):
        self.file = file
        self.canrep = 0
    def Run(self,trial):
        RunCommandIO(['gzip', '-cf'], self.file, DFILE)
        if trial > 0:
            return None
        return GzipInfo(self.file,DFILE)

def SetFileSize(F,L):
    fd = os.open(F, os.O_CREAT | os.O_WRONLY)
    os.ftruncate(fd,L)
    assert(os.fstat(fd).st_size == L)
    os.close(fd)

def ReportSpeed(L,tr,desc):
    print '%s 0-run length %u: dsize %u: time %.3f ms: encode %.0f B/sec: in %u trials' % \
          (desc, L, tr.r1.dsize, tr.time.mean * 1000.0,
           ((L+tr.r1.dsize) / tr.time.mean), tr.trials)

class RandomTestResult:
    def __init__(self, round, config, runtime, compsize):
        self.round = round
        self.myconfig = config
        self.runtime = runtime
        self.compsize = compsize
        self.score = None
        self.time_pos = None
        self.size_pos = None
        self.score_pos = None
    #end

    def __str__(self):
        return 'time %.6f%s size %d%s << %s >>' % (
            self.time(), ((self.time_pos != None) and (" (%s)" % self.time_pos) or ""),
            self.size(), ((self.size_pos != None) and (" (%s)" % self.size_pos) or ""),
            c2s(self.config()))
    #end

    def time(self):
        return self.runtime
    #end

    def size(self):
        return self.compsize
    #end

    def config(self):
        return self.myconfig
    #end

    def score(self):
        return self.score
    #end
#end

def PosInAlist(l, e):
    for i in range(0, len(l)):
        if l[i][1] == e:
            return i;
        #end
    #end
    return -1
#end

test_totals = {}

class RandomTester:
    def __init__(self, old_results):
        self.old_configs = old_results

        # these get reset each round so we don't test the same config twice
        self.results = []

        self.trial_num = 0
        self.round_num = 0
        self.random = random.Random()
    #end

    def Reset(self):
        self.results = []
    #end

    def HasEnoughResults(self):
        return len(self.results) >= MAX_RESULTS
    #end

    def RandomConfig(self):

        config = []
        map = {}

        for key in CONFIG_ORDER:

            val = map[key] = (INPUT_SPEC(self.random)[key])(map)

            config.append(val)
        #end

        if map['small_chain'] < map['small_lchain']:
            return None

        if map['large_look'] < map['small_look']:
            return None

        for r in self.results:
            if c2s(r.config()) == c2s(config):
                return None
            #end
        #end

        return config

    def MakeBigFiles(self, rcsf):
        f1 = open(TMPDIR + "/big.1", "w")
        f2 = open(TMPDIR + "/big.2", "w")
        population = []
        for file in rcsf.rcsfiles:
            if len(file.versions) < 2:
                continue
            population.append(file)
        #end
        f1sz = 0
        f2sz = 0
        fcount = int(len(population) * FILE_P)
        assert fcount > 0
        for file in self.random.sample(population, fcount):
            m = IGNORE_FILENAME.match(file.fname)
            if m != None:
                continue
            #end
            r1, r2 = self.random.sample(xrange(0, len(file.versions)), 2)
            f1sz += file.AppendVersion(f1, r1)
            f2sz += file.AppendVersion(f2, r2)
        #end

        print 'from %u; to %u' % (f1sz, f2sz)
        f1.close()
        f2.close()
        return (TMPDIR + "/big.1",
                TMPDIR + "/big.2")

    def RandomFileTest(self, f1, f2):
        config = None
        if len(self.old_configs) > 0:
            config = self.old_configs[0]
            self.old_configs = self.old_configs[1:]
        #end

        while config is None:
            config = self.RandomConfig()
        #end

        runner = Xdelta3Pair([ '-C', ','.join([str(x) for x in config]) ])
        runner.SetInputs(f1, 1, f2, 2)
        result = TimedTest(runner)

        tr = RandomTestResult(self.round_num,
                              config,
                              result.time.mean,
                              result.r1.dsize)

        self.results.append(tr)

        print 'Test %d: %s in %u trials' % \
              (self.trial_num,
               tr,
               result.trials)

        self.trial_num += 1
        return
    #end

    def ScoreTests(self):
        scored = []
        timed = []
        sized = []

        t_min = float(min([test.time() for test in self.results]))
        t_max = float(max([test.time() for test in self.results]))
        s_min = float(min([test.size() for test in self.results]))
        s_max = float(max([test.size() for test in self.results]))

        # These are the major axes of an ellipse, after normalizing for the
        # mininum values.  Time should be major, size should be minor.
        time_major = (t_max / t_min)
        size_minor = (s_max / s_min)

        # Dimensions of the rectangular region bounding the results.
        t_rect = time_major - 1.0
        s_rect = size_minor - 1.0

        rect_ratio = s_rect / t_rect

        for test in self.results:

            # Transform the major min/max region linearly to normalize the
            # min-max variation in time (major) and size (minor).

            s_norm = test.size() / s_min
            t_norm = 1.0 + rect_ratio * (test.time() / t_min - 1.0)

            assert t_norm >= 1.0
            assert t_norm <= size_minor + 0.000001

            # Projects the coords onto a min-unit circle.  Use the
            # root-mean-square.  Smaller scores are better, 1.0 is the minimum.
            test.score = math.sqrt(t_norm * t_norm / 2.0 + s_norm * s_norm / 2.0)

            scored.append((test.score, test))
            timed.append((test.time(), test))
            sized.append((test.size(), test))
        #end

        scored.sort()
        timed.sort()
        sized.sort()

        best_by_size = []
        best_by_time = []

        print 'Worst: %s' % scored[len(scored)-1][1]
        scored = [x[1] for x in scored[0:int(MAX_RESULTS * KEEP_P)]]

        for fast in [x[1] for x in timed[0:int(MAX_RESULTS * FAST_P)]]:
            if fast in scored:
                continue
            print 'Carry fast: %s' % (fast)
            scored.append(fast)
        #end

        for slow in [x[1] for x in sized[0:int(MAX_RESULTS * SLOW_P)]]:
            if slow in scored:
                continue
            print 'Carry slow: %s' % (slow)
            scored.append(slow)
        #end

        # Do not carry slow. It causes bad compressors to perpetuate extra
        # weight.
        for test in scored:
            test.size_pos = PosInAlist(sized, test)
            test.time_pos = PosInAlist(timed, test)
        #end

        r = []
        pos = 0
        for test in scored:
            pos += 1
            test.score_pos = pos
            c = c2s(test.config())
            if not test_totals.has_key(c):
                test_totals[c] = [test]
            else:
                test_totals[c].append(test)
            #end
            s = 0.0
            self.results.append(test)
            r.append(test.config())
            all_r = test_totals[c]
            for t in all_r:
                s += float(t.score_pos)
            #end
            if len(all_r) == 1:
                stars = ''
            elif len(all_r) >= 10:
                stars = ' ***'
            elif len(all_r) >= int(1/KEEP_P):
                stars = ' **'
            else:
                stars = ' *'
            print 'Score: %0.6f %s (%.1f%s%s)' % \
                  (test.score, test, s / len(all_r), stars,
                   (len(all_r) > 2) and
                   (' in %d' % len(all_r)) or "")
        #end
        
        return r
    #end
#end

# This tests the raw speed of 0-byte inputs
def RunSpeed():
    for L in Decimals(MAX_RUN):
        SetFileSize(RUNFILE, L)
        trx = TimedTest(Xdelta3Run1(RUNFILE))
        ReportSpeed(L,trx,'xdelta3')
        trm = TimedTest(Xdelta3Mod1(RUNFILE))
        ReportSpeed(L,trm,'module ')
        trg = TimedTest(GzipRun1(RUNFILE))
        ReportSpeed(L,trg,'gzip   ')
    #end
#end

if __name__ == "__main__":
    try:
        RunCommand(['rm', '-rf', TMPDIR])
        os.mkdir(TMPDIR)

        RunSpeed()

        # This tests pairwise (date-ordered) performance
        #rcsf = Test()
        #rcsf.PairsByDate(Xdelta3Pair([]))

        configs = []

        while 0:
            #f1 = '/tmp/big.1'
            #f2 = '/tmp/big.2'
            test = RandomTester(configs)
            #f1, f2 = test.MakeBigFiles(rcsf)
            while not test.HasEnoughResults():
                f1 = '/tmp/WESNOTH_tmp/wesnoth-1.1.12.tar'
                f2 = '/tmp/WESNOTH_tmp/wesnoth-1.1.13.tar'
                #f1 = '/tmp/big.1'
                #f2 = '/tmp/big.2'
                test.RandomFileTest(f1, f2)
            #end
            configs = test.ScoreTests()

            #test.Reset()
            test.results = test.results[0:len(configs)]
            configs = []
            #break
            #end
        #end

    except CommandError:
        pass
    else:
        RunCommand(['rm', '-rf', TMPDIR])
        pass
