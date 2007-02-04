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

# Under construction.

# TODO: This is really part test, part performance evaluation suite, and
# really incomplete.

# TODO: Test IOPT (1.5 vs. greedy)

import os, sys, math, re, time, types, array, random
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

#
#
RCSDIR = '/mnt/polaroid/Polaroid/orbit_linux/home/jmacd/PRCS'
RCSDIR = '/tmp/PRCS_read_copy'
#RCSDIR = 'G:/jmacd/PRCS'

TMPDIR = '/tmp/xd3regtest.%d' % os.getpid()

RUNFILE = os.path.join(TMPDIR, 'run')
HFILE   = os.path.join(TMPDIR, 'hdr')
DFILE   = os.path.join(TMPDIR, 'output')
RFILE   = os.path.join(TMPDIR, 'recon')

HEAD_STATE = 0
BAR_STATE  = 1
REV_STATE  = 2
DATE_STATE = 3

# rcs output
RE_TOTREV  = re.compile('total revisions: (\\d+)')
RE_BAR     = re.compile('----------------------------')
RE_REV     = re.compile('revision (.+)')
RE_DATE    = re.compile('date: ([^;]+);.*')
# xdelta output
RE_HDRSZ   = re.compile('VCDIFF header size: +(\\d+)')
RE_EXTCOMP = re.compile('XDELTA ext comp.*')

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
    def Print(self):
        print '%s %s' % (self.vstr, self.date)
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
            
            result = TimeRun(runnable.Runner(self.Verf(v),
                                             self.Vstr(v),
                                             self.Verf(v+1),
                                             self.Vstr(v+1)))
            print 'testing %s %s: ideal %.3f%%: time %.7f: in %u trials' % \
                  (os.path.basename(self.fname),
                   self.Vstr(v+1),
                   result.r1.ideal,
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
class TimeRun:
    def __init__(self,runnable,
                 skip_trials=SKIP_TRIALS,min_trials=MIN_TRIALS,max_trials=MAX_TRIALS,
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
    #print "run command io", args
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
        #print 'RUN', args
        xdelta3.main(args)
    except Exception, e:
        raise CommandError(args, "xdelta3.main exception")

class GzipInfo:
    def __init__(self,target,delta):
        self.tgtsize = os.stat(target).st_size
        self.dsize   = os.stat(delta).st_size
        
class Xdelta3Info:
    def __init__(self,target,delta):
        self.extcomp = 0  # TODO: I removed some code that called printhdr
        self.hdrsize = 0  # to compute these, but printhdr uses stdout (now)
        self.tgtsize = os.stat(target).st_size
        self.dsize   = os.stat(delta).st_size
        if self.tgtsize > 0:
            self.ideal = 100.0 * self.dsize / self.tgtsize;
        else:
            self.ideal = 0.0

class Xdelta3Pair:
    def __init__(self):
        self.type        = 'xdelta3'
        self.decode_args = '-dqf'
        self.encode_args = '-eqf'
        self.extra       = []
        self.presrc      = '-s'
        self.canrep      = 1

    def Runner(self,old,oldv,new,newv):
        self.old = old
        self.oldv = oldv
        self.new = new
        self.newv = newv        
        return self

    def Run(self,trial):

        # TODO: move '-S djw' somewhere else
        encode_args =  self.extra + \
                       [ '-S', 'djw' ] + \
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
        RunXdelta3(['-efq', self.file, DFILE])
        if trial > 0:
            return None
        return Xdelta3Info(self.file,DFILE)

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
          (desc, L, tr.r1.dsize, tr.time.mean * 1000.0, ((L+tr.r1.dsize) / tr.time.mean), tr.trials)

def MakeBigFiles(rcsf):
    rand = random.Random()
    f1 = open(TMPDIR + "/big.1", "w")
    f2 = open(TMPDIR + "/big.2", "w")
    f1sz = 0
    f2sz = 0
    for file in rcsf.rcsfiles:
        if file.versions < 2:
            continue
        r1 = 0
        r2 = 0
        while r1 == r2:
            r1 = rand.randint(0, len(file.versions) - 1)
            r2 = rand.randint(0, len(file.versions) - 1)
        f1sz += file.AppendVersion(f1, r1)
        f2sz += file.AppendVersion(f2, r2)

    print 'from %u; to %u' % (f1sz, f2sz)
    f1.close()
    f2.close()
    return (TMPDIR + "/big.1",
            TMPDIR + "/big.2")

class RandomTestResult:
    def __init__(self, round, config, runtime, compsize):
        self.round = round
        self.myconfig = config
        self.runtime = runtime
        self.compsize = compsize
    #end

    def __str__(self):
        return '%.4f %d [%s]' % (self.time(), self.size(), ' '.join([str(x) for x in self.config()]))
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
#end

def PosInAlist(l, e):
    for i in range(len(l)):
        if l[i][1] == e:
            return i;
        #end
    #end
    return -1
#end

# How many results per round
MAX_RESULTS = 500

class RandomTester:
    def __init__(self, old_results):
        self.old_configs = old_results
        self.results = []
        self.trial_num = 0
        self.round_num = 0
        self.random = random.Random()
    #end

    def HasEnoughResults(self):
        return len(self.results) >= MAX_RESULTS
    #end

    def RandomConfig(self):

        input_ranges = [
            (9, 9, 9, 'large_look'),
            (1, 4.5, 8, 'large_step'),
            (4, 4, 4, 'small_look'),  # Note: disabled
            (1, 10, 30, 'small_chain'),
            (1, 3.5, 6, 'small_lchain'),
            (0, 0, 0, 'ssmatch'),     # Note: disabled
            (1, 1, 1, 'trylazy'),     # Note: enabled
            (1, 128, 256, 'max_lazy'),
            (1, 256, 512, 'long_enough'),
            (0, 0, 0, 'promote'),     # Note: disabled
        ]

        config = []
        map = {}

        for input in input_ranges:
            minv = input[0]
            mean = input[1]
            maxv = input[2]
            name = input[3]
            if minv == maxv:
                val = minv
            else:
                val = -1
                while val < minv or val > maxv:
                    val = int(self.random.expovariate(1.0 / mean) + 0.5)
                #end
            #end

            config.append(val)
            map[name] = val
        #end

        if map['small_chain'] < map['small_lchain']:
            return None

        if map['large_look'] < map['small_look']:
            return None

        return config

    def RandomBigRun(self, f1, f2):
        config = None
        if len(self.old_configs) > 0:
            config = self.old_configs[0]
            self.old_configs = self.old_configs[1:]
        #end

        while config is None:
            config = self.RandomConfig()
        #end

        runner = Xdelta3Pair()
        runner.extra = ['-I',
                        '0',
                        '-D',
                        '-C', ','.join([str(x) for x in config])]
        result = TimeRun(runner.Runner(f1, 1, f2, 2))

        tr = RandomTestResult(self.round_num,
                              config,
                              result.time.mean,
                              result.r1.dsize)

        self.results.append(tr)

        print 'Trial %d: %s in %u trials' % \
              (self.trial_num,
               tr,
               result.trials)

        self.trial_num += 1
        return
    #end

    def ScoreTests(self):
        mint = float(min([test.time() for test in self.results]))
        maxt = float(max([test.time() for test in self.results]))
        mins = float(min([test.size() for test in self.results]))
        maxs = float(max([test.size() for test in self.results]))

        scored = []
        timed = []
        sized = []

        for test in self.results:

            # This scores ellipse has x-major (time) and y-minor (size)
            ntime = (test.time()) / float(maxt)
            nsize = (test.size()) / float(maxs)

            wntime = ntime * (maxs / mins)
            wnsize = nsize * (maxt / mint)
            score = math.sqrt(wntime * wntime + wnsize * wnsize)
            scored.append((score, test))
            timed.append((test.time(), test, score))
            sized.append((test.size(), test, score))
        #end
        scored.sort()
        timed.sort()
        sized.sort()
        for (score, test) in scored:
            spos = PosInAlist(sized, test)
            tpos = PosInAlist(timed, test)
            print 'Score %f: %s (%d, %d)' % (score, test, spos, tpos)
        #end

        sized = sized[0:MAX_RESULTS/2]
        timed = timed[0:MAX_RESULTS/2]

        for (size, test, score) in sized:
            print 'Size: %s (%f)' % (test, score)
        #end

        for (time, test, score) in timed:
            print 'Time: %s (%f)' % (test, score)
        #end

        self.results = []
        r = []
        for (score, test) in scored[0:MAX_RESULTS/2]:
            r.append(test.config())
        #end

        return r
    #end
#end

def RunSpeed():
    for L in Decimals(MAX_RUN):
        SetFileSize(RUNFILE, L)
        trx = TimeRun(Xdelta3Run1(RUNFILE))
        ReportSpeed(L,trx,'xdelta3')
        trg = TimeRun(GzipRun1(RUNFILE))
        ReportSpeed(L,trg,'gzip   ')
    #end
#end

if __name__ == "__main__":
    try:
        RunCommand(['rm', '-rf', TMPDIR])
        os.mkdir(TMPDIR)
        rcsf = Test()
        configs = []

        while 1:
            f1, f2 = MakeBigFiles(rcsf)
            #f1 = '/tmp/big.1'
            #f2 = '/tmp/big.2'
            test = RandomTester(configs)
            while not test.HasEnoughResults():
                test.RandomBigRun(f1, f2)
            #end
            configs = test.ScoreTests()
            #end
        #end

        # This tests pairwise (date-ordered) performance
        #rcsf.PairsByDate(Xdelta3Pair())

        # This tests the raw speed of 0-byte inputs
        #RunSpeed()

    except CommandError:
        pass
    else:
        RunCommand(['rm', '-rf', TMPDIR])
        pass
