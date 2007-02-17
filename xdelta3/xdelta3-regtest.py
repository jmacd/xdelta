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

# TODO: Test 1.5 vs. greedy
# TODO: Compare w/ bsdiff and generate more summary

import os, sys, math, re, time, types, array, random
import xdelta3main
import xdelta3

HIST_SIZE      = 10   # the number of buckets
MIN_SIZE       = 0

TIME_TOO_SHORT = 0.050

SKIP_TRIALS    = 2
MIN_TRIALS     = 3
MAX_TRIALS     = 15

MIN_STDDEV_PCT = 1.5

MIN_RUN        = 1000 * 1000 * 1
MAX_RUN        = 1000 * 1000 * 10

# How many results per round
MAX_RESULTS = 6
KEEP_P = (1.0)
FAST_P = (0.0)
SLOW_P = (0.0)
FILE_P = (0.30)

# the first 10 args go to -C
SOFT_CONFIG_CNT = 10

CONFIG_ORDER = [ 'large_look',
                 'large_step',
                 'small_look',
                 'small_chain',
                 'small_lchain',
                 'ssmatch',
                 'trylazy',
                 'max_lazy',
                 'long_enough',
                 'promote',
                 'winsize',
                 'srcwinsize',
                 ]

CONFIG_ARGMAP = {
    'winsize' : '-W',
    'srcwinsize' : '-B',
    }

def INPUT_SPEC(rand):
    return {
    'large_look' : lambda d: rand.choice([9]),
    'large_step' : lambda d: rand.choice([15]),

    'small_chain'  : lambda d: rand.choice([1]),
    'small_lchain' : lambda d: rand.choice([1]),

    'max_lazy'     : lambda d: rand.choice([18]),
    'long_enough'  : lambda d: rand.choice([18]),

    'small_look'   : lambda d: rand.choice([4]),
    'promote'      : lambda d: 0,
    'trylazy'      : lambda d: 1,
    'ssmatch'      : lambda d: 0,

    'winsize' :    lambda d: rand.choice(
        [x * (1<<20) for x in [1, 4, 8, 16, 32, 64]]),

    'srcwinsize' : lambda d: 1<<26,
    }


#
#
#RCSDIR = '/mnt/polaroid/Polaroid/orbit_linux/home/jmacd/PRCS'
#RCSDIR = '/tmp/PRCS_read_copy'

RCSDIR = 'G:/jmacd/PRCS/prcs/b'

SAMPLEDIR = "C:/sample_data/Wesnoth/tar"

TMPDIR = '/tmp/xd3regtest.%d' % os.getpid()

RUNFILE = os.path.join(TMPDIR, 'run')
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
#end

def SumList(l):
    return reduce(lambda x,y: x+y, l)
#end

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
        self.siqr   = (self.q3-self.q1)/2.0;
        self.spread = (self.q4-self.q0)
        self.str    = '%s %d; mean %d; sdev %d; q2 %d; .5(q3-q1) %.1f; spread %d' % \
                      (desc, self.total, self.mean, self.s, self.q2, self.siqr, self.spread)
    #end
#end

def RunCommand(args, ok = [0]):
    #print 'run command %s' % (' '.join(args))
    p = os.spawnvp(os.P_WAIT, args[0], args)
    if p not in ok:
        raise CommandError(args, 'exited %d' % p)
    #end
#end

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
        #end
    #end
#end

class TimedTest:
    def __init__(self, target, source, runnable,
                 skip_trials = SKIP_TRIALS,
                 min_trials = MIN_TRIALS,
                 max_trials = MAX_TRIALS,
                 min_stddev_pct = MIN_STDDEV_PCT):
        self.target = target
        self.source = source
        self.runnable = runnable

        self.skip_trials = skip_trials
        self.min_trials = min(min_trials, max_trials)
        self.max_trials = max_trials
        self.min_stddev_pct = min_stddev_pct

        self.encode_time = self.DoTest(DFILE,
                                       lambda x: x.Encode(self.target, self.source, DFILE))
        self.encode_size = runnable.EncodeSize(DFILE)
        self.decode_time = self.DoTest(RFILE,
                                       lambda x: x.Decode(DFILE, self.source, RFILE))

        # verify
        runnable.Verify(self.target, RFILE)
    #end

    def DoTest(self, fname, func):
        trials   = 0
        measured = []

        while 1:
            try:
                os.remove(fname)
            except OSError:
                pass

            start_time  = time.time()
            start_clock = time.clock()

            func(self.runnable)

            total_clock = (time.clock() - start_clock)
            total_time  = (time.time() - start_time)

            elap_time  = max(total_time,  0.0000001)
            elap_clock = max(total_clock, 0.0000001)

            trials = trials + 1

            # skip some of the first trials
            if trials > self.skip_trials:
                measured.append((elap_clock, elap_time))
                #print 'measurement total: %.1f ms' % (total_time * 1000.0)

            # at least so many
            if trials < (self.skip_trials + self.min_trials):
                #print 'continue: need more trials: %d' % trials
                continue

            # compute %variance
            done = 0
            if self.skip_trials + self.min_trials <= 2:
                measured = measured + measured;
                done = 1
            #end

            time_stat = StatList([x[1] for x in measured], 'elap time')
            sp = float(time_stat.s) / float(time_stat.mean)

            # what if MAX_TRIALS is exceeded?
            too_many = (trials - self.skip_trials) >= self.max_trials
            good = (100.0 * sp) < self.min_stddev_pct
            if done or too_many or good:
                trials = trials - self.skip_trials
                if not done and not good:
                    #print 'too many trials: %d' % trials
                    pass
                #clock = StatList([x[0] for x in measured], 'elap clock')
                return time_stat
            #end
        #end
    #end
#end

def Decimals(start, end):
    l = []
    step = start
    while 1:
        r = range(step, step * 10, step)
        l = l + r
        if step * 10 >= end:
            l.append(step * 10)
            break
        step = step * 10
    return l
#end

# This tests the raw speed of 0-byte inputs
def RunSpeedTest():
    for L in Decimals(MIN_RUN, MAX_RUN):
        SetFileSize(RUNFILE, L)

        trx = TimedTest(RUNFILE, None, Xdelta3Runner(['-W', str(1<<20)]))
        ReportSpeed(L, trx, '1MB ')

        trx = TimedTest(RUNFILE, None, Xdelta3Runner(['-W', str(1<<19)]))
        ReportSpeed(L, trx, '512k')

        trx = TimedTest(RUNFILE, None, Xdelta3Runner(['-W', str(1<<18)]))
        ReportSpeed(L, trx, '256k')

        trm = TimedTest(RUNFILE, None, Xdelta3Mod1(RUNFILE))
        ReportSpeed(L, trm, 'swig')

        trg = TimedTest(RUNFILE, None, GzipRun1())
        ReportSpeed(L,trg,'gzip')
    #end
#end

def SetFileSize(F,L):
    fd = os.open(F, os.O_CREAT | os.O_WRONLY)
    os.ftruncate(fd,L)
    assert os.fstat(fd).st_size == L
    os.close(fd)
#end

def ReportSpeed(L,tr,desc):
    print '%s run length %u: size %u: time %.3f ms: decode %.3f ms' % \
          (desc, L,
           tr.encode_size,
           tr.encode_time.mean * 1000.0,
           tr.decode_time.mean * 1000.0)
#end

class Xdelta3RunClass:
    def __init__(self, extra):
        self.extra = extra
    #end

    def __str__(self):
        return ' '.join(self.extra)
    #end

    def New(self):
        return Xdelta3Runner(self.extra)
    #end
#end

class Xdelta3Runner:
    def __init__(self, extra):
        self.extra = extra
    #end

    def Encode(self, target, source, output):
        args = (testwide_encode_args +
                self.extra +
                ['-eqf'])
        if source:
            args.append('-s')
            args.append(source)
        #end
        args = args + [target, output]
        self.Main(args)
    #end

    def Decode(self, input, source, output):
        args = ['-dqf']
        if source:
            args.append('-s')
            args.append(source)
        #end
        args = args + [input, output]
        self.Main(args)
    #end

    def Verify(self, target, recon):
        RunCommand(('cmp', target, recon))
    #end

    def EncodeSize(self, output):
        return os.stat(output).st_size
    #end

    def Main(self, args):
        try:
            xdelta3main.main(args)
        except Exception, e:
            raise CommandError(args, "xdelta3.main exception")
        #end
    #end
#end

class Xdelta3Mod1:
    def __init__(self, file):
        self.target_data = open(file, 'r').read()
    #end

    def Encode(self, ignore1, ignore2, ignore3):
        r1, encoded = xdelta3.xd3_encode_memory(self.target_data, None, 1000000, 1<<10)
        if r1 != 0:
            raise CommandError('memory', 'encode failed: %s' % r1)
        #end
        self.encoded = encoded
    #end

    def Decode(self, ignore1, ignore2, ignore3):
        r2, data1 = xdelta3.xd3_decode_memory(self.encoded, None, len(self.target_data))
        if r2 != 0:
            raise CommandError('memory', 'decode failed: %s' % r1)
        #end
        self.decoded = data1
    #end

    def Verify(self, ignore1, ignore2):
        if self.target_data != self.decoded:
            raise CommandError('memory', 'bad decode')
        #end
    #end

    def EncodeSize(self, ignore1):
        return len(self.encoded)
    #end
#end

class GzipRun1:
    def Encode(self, target, source, output):
        assert source == None
        RunCommandIO(['gzip', '-cf'], target, output)
    #end

    def Decode(self, input, source, output):
        assert source == None
        RunCommandIO(['gzip', '-dcf'], input, output)
    #end

    def Verify(self, target, recon):
        RunCommand(('cmp', target, recon))
    #end

    def EncodeSize(self, output):
        return os.stat(output).st_size
    #end
#end

class Xdelta1RunClass:
    def __str__(self):
        return 'xdelta1'
    #end

    def New(self):
        return Xdelta1Runner()
    #end
#end

class Xdelta1Runner:
    def Encode(self, target, source, output):
        assert source != None
        args = ['xdelta1', 'delta', '-q', source, target, output]
        RunCommand(args, [0, 1])
    #end

    def Decode(self, input, source, output):
        assert source != None
        args = ['xdelta1', 'patch', '-q', input, source, output]
        # Note: for dumb historical reasons, xdelta1 returns 1 or 0
        RunCommand(args)
    #end

    def Verify(self, target, recon):
        RunCommand(('cmp', target, recon))
    #end

    def EncodeSize(self, output):
        return os.stat(output).st_size
    #end
#end

# TODO: cleanup below this line

# exceptions
class SkipRcsException:
    def __init__(self,reason):
        self.reason = reason
    #end
#end

class NotEnoughVersions:
    def __init__(self):
        pass
    #end
#end

class CommandError:
    def __init__(self,cmd,str):
        if type(cmd) is types.TupleType or \
           type(cmd) is types.ListType:
            cmd = reduce(lambda x,y: '%s %s' % (x,y),cmd)
        #end
        print 'command was: ',cmd
        print 'command failed: ',str
        print 'have fun debugging'
    #end
#end

class RcsVersion:
    def __init__(self,vstr):
        self.vstr = vstr
    #end
    def __cmp__(self,other):
        return cmp(self.date, other.date)
    #end
    def __str__(self):
        return str(self.vstr)
    #end
#end

class RcsFile:

    def __init__(self, fname):
        self.fname    = fname
        self.versions = []
        self.state    = HEAD_STATE
    #end

    def SetTotRev(self,s):
        self.totrev = int(s)
    #end

    def Rev(self,s):
        self.rev = RcsVersion(s)
        if len(self.versions) >= self.totrev:
            raise SkipRcsException('too many versions (in log messages)')
        #end
        self.versions.append(self.rev)
    #end

    def Date(self,s):
        self.rev.date = s
    #end

    def Match(self, line, state, rx, gp, newstate, f):
        if state == self.state:
            m = rx.match(line)
            if m:
                if f:
                    f(m.group(gp))
                #end
                self.state = newstate
                return 1
            #end
        #end
        return None
    #end

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
            #end
            l = f.readline()
        #end
        c = f.close()
        if c != None:
            raise c
        #end
    #end

    def Sum1(self):
        st = os.stat(self.fname)
        self.rcssize = st.st_size
        self.Sum1Rlog()
        if self.totrev != len(self.versions):
            raise SkipRcsException('wrong version count')
        #end
        self.versions.sort()
    #end

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
        #end
        v.vsize = total
        estr = ''
        buf = err.read()
        while buf:
            estr = estr + buf
            buf = err.read()
        #end
        if stream.close():
            raise CommandError(cmd, 'checkout failed: %s\n%s\n%s' % (v.vstr, self.fname, estr))
        #end
        out.close()
        err.close()
    #end

    def Vdate(self,n):
        return self.versions[n].date
    #end

    def Vstr(self,n):
        return self.versions[n].vstr
    #end

    def Verf(self,n):
        return os.path.join(TMPDIR, 'input.%d' % n)
    #end

    def FilePairsByDate(self, runclass):
        if self.totrev < 2:
            raise NotEnoughVersions()
        #end
        self.Checkout(0)
        ntrials = []
        if self.totrev < 2:
            return vtrials
        #end
        for v in range(0,self.totrev-1):
            if v > 1:
                os.remove(self.Verf(v-1))
            #end
            self.Checkout(v+1)
            if os.stat(self.Verf(v)).st_size < MIN_SIZE or \
               os.stat(self.Verf(v+1)).st_size < MIN_SIZE:
                continue
            #end

            result = TimedTest(self.Verf(v+1),
                               self.Verf(v),
                               runclass.New())

            target_size = os.stat(self.Verf(v+1)).st_size

            if 0:
                print '%s %s %s: %.2f%% encode %.3f ms: decode %.3f ms' % \
                  (runclass,
                   os.path.basename(self.fname),
                   self.Vstr(v+1),
                   target_size > 0 and (100.0 * result.encode_size / target_size) or 0,
                   result.encode_time.mean * 1000.0,
                   result.decode_time.mean * 1000.0)
            #end
            ntrials.append(result)
        #end

        os.remove(self.Verf(self.totrev-1))
        os.remove(self.Verf(self.totrev-2))
        return ntrials
    #end

    def AppendVersion(self, f, n):
        self.Checkout(n)
        rf = open(self.Verf(n), "r")
        data = rf.read()
        f.write(data)
        rf.close()
        return len(data)
    #end

class RcsFinder:
    def __init__(self):
        self.subdirs  = []
        self.rcsfiles = []
        self.others   = []
        self.skipped  = []
    #end

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
            #end
        #end
        self.subdirs  = self.subdirs  + subdirs
        self.rcsfiles = self.rcsfiles + rcsfiles
        self.others   = self.others   + others
        return subdirs
    #end

    def Crawl(self, dir):
        subdirs = [dir]
        while subdirs:
            s1 = self.Scan1(subdirs[0])
            subdirs = subdirs[1:] + s1
        #end
    #end

    def Summarize(self):
        good = []
        for rf in self.rcsfiles:
            try:
                rf.Sum1()
                if rf.totrev < 2:
                    raise SkipRcsException('too few versions (< 2)')
                #end
            except SkipRcsException, e:
                #print 'skipping file %s: %s' % (rf.fname, e.reason)
                self.skipped.append(rf)
            else:
                good.append(rf)
            #end
        self.rcsfiles = good
    #end

    def AllPairsByDate(self,runclass):
        results = []
        good = []
        for rf in self.rcsfiles:
            try:
                results = results + rf.FilePairsByDate(runclass)
            except SkipRcsException:
                print 'file %s has compressed versions: skipping' % (rf.fname)
            except NotEnoughVersions:
                print 'testing %s on %s: not enough versions' % (runclass, rf.fname)
            else:
                good.append(rf)
            #end
        self.rcsfiles = good
        ReportPairs(runclass, results)
        return results
    #end

def ReportPairs(name, results):
    encode_time = 0
    decode_time = 0
    encode_size = 0
    for r in results:
        encode_time += r.encode_time.mean
        decode_time += r.decode_time.mean
        encode_size += r.encode_size
    #end
    print '%s rcs: encode %.2f s: decode %.2f s: size %d' % \
          (name, encode_time, decode_time, encode_size)
#end

def GetTestRcsFiles():
    rcsf = RcsFinder()
    rcsf.Crawl(RCSDIR)
    if len(rcsf.rcsfiles) == 0:
        raise CommandError('', 'no RCS files')
    #end
    rcsf.Summarize()
    print "rcsfiles: rcsfiles %d; subdirs %d; others %d; skipped %d" % (len(rcsf.rcsfiles),
                                                                        len(rcsf.subdirs),
                                                                        len(rcsf.others),
                                                                        len(rcsf.skipped))
    print StatList([x.rcssize for x in rcsf.rcsfiles], "rcssize", 1).str
    print StatList([x.totrev for x in rcsf.rcsfiles], "totrev", 1).str
    return rcsf
#end

# TODO: cleanup below this line

#
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

    def ConfigToArgs(self, config):
        args = [ '-C',
                 ','.join([str(x) for x in config[0:SOFT_CONFIG_CNT]])]
        for i in range(SOFT_CONFIG_CNT, len(CONFIG_ORDER)):
            key = CONFIG_ARGMAP[CONFIG_ORDER[i]]
            val = config[i]
            args.append('%s=%s' % (key, val))
        #end
        return args
    #end

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

        print 'source %u bytes; target %u bytes' % (f1sz, f2sz)
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

        args = self.ConfigToArgs(config)
        result = TimedTest(f2, f1, Xdelta3Runner(args))

        tr = RandomTestResult(self.round_num,
                              config,
                              result.encode_time.mean,
                              result.encode_size)

        self.results.append(tr)

        print 'Test %d: %s' % (self.trial_num, tr)
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

        pos = 0
        for (score, test) in scored:
            pos += 1
            test.score_pos = pos
            c = c2s(test.config())
            if not test_totals.has_key(c):
                test_totals[c] = [test]
            else:
                test_totals[c].append(test)
            #end
        #end

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
        for test in scored:
            c = c2s(test.config())
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

def RunRandomRcsTest(rcsf):
    configs = []
    while 1:
        test = RandomTester(configs)
        f1, f2 = test.MakeBigFiles(rcsf)
        while not test.HasEnoughResults():
            test.RandomFileTest(f1, f2)
        #end
        configs = test.ScoreTests()
        test.Reset()
    #end
#end

def RunSampleTest(d, files):
    # TODO: consolidate w/ the above
    print 'testing %s with %d files' % (d, len(files))
    configs = []
    while len(files) > 1:
        test = RandomTester(configs)
        f1 = files[0]
        f2 = files[1]
        while not test.HasEnoughResults():
            test.RandomFileTest(f1, f2)
        #end
        configs = test.ScoreTests()
        test.Reset()
        files = files[1:]
    #end
#end

def RunSampleDataTest():
    dirs = [SAMPLEDIR]
    while dirs:
        d = dirs[0]
        dirs = dirs[1:]
        l = os.listdir(d)
        files = []
        for e in l:
            p = os.path.join(d, e)
            if os.path.isdir(p):
                dirs.append(p)
            else:
                files.append(p)
            #end
        #end
        if files:
            files.sort()
            RunSampleTest(d, files)
        #end
    #end
#end

if __name__ == "__main__":
    try:
        RunCommand(['rm', '-rf', TMPDIR])
        os.mkdir(TMPDIR)

        #RunSpeedTest()

        #rcsf = GetTestRcsFiles()

        #x3r = rcsf.AllPairsByDate(Xdelta3RunClass(['-9']))
        #x3r = rcsf.AllPairsByDate(Xdelta3RunClass(['-9', '-S', 'djw']))
        #x3r = rcsf.AllPairsByDate(Xdelta3RunClass(['-9', '-T']))

        #x3r = rcsf.AllPairsByDate(Xdelta3RunClass([]))
        #ReportPairs('xdelta3', x3r)

        #x1r = rcsf.AllPairsByDate(Xdelta1RunClass())
        #ReportPairs('xdelta1', x1r)

        #RunRandomRcsTest(rcsf)
        RunSampleDataTest()

    except CommandError:
        pass
    else:
        RunCommand(['rm', '-rf', TMPDIR])
        pass
