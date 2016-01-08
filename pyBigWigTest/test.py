import pyBigWig
import tempfile
import os
import hashlib

class TestRemote():
    fname = "https://raw.githubusercontent.com/dpryan79/pyBigWig/master/pyBigWigTest/test.bw"

    def doOpen(self):
        print(self.fname)
        bw = pyBigWig.open(self.fname)
        assert(bw is not None)
        return bw

    def doChroms(self, bw):
        assert(bw.chroms() == {'1': 195471971, '10': 130694993})
        assert(bw.chroms("1") == 195471971)
        assert(bw.chroms("c") is None)

    def doHeader(self, bw):
        assert(bw.header() == {'maxVal': 2, 'sumData': 272, 'minVal': 0, 'version': 4, 'sumSquared': 500, 'nLevels': 1, 'nBasesCovered': 154})

    def doStats(self, bw):
        assert(bw.stats("1", 0, 3) == [0.2000000054637591])
        assert(bw.stats("1", 0, 3, type="max") == [0.30000001192092896])
        assert(bw.stats("1",99,200, type="max", nBins=2) == [1.399999976158142, 1.5])
        assert(bw.stats("1") == [1.3351851569281683])

    def doValues(self, bw):
        assert(bw.values("1", 0, 3) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896])
        #assert(bw.values("1", 0, 4) == [0.10000000149011612, 0.20000000298023224, 0.30000001192092896, 'nan'])

    def doIntervals(self, bw):
        assert(bw.intervals("1", 0, 3) == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896)))
        assert(bw.intervals("1") == ((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224), (2, 3, 0.30000001192092896), (100, 150, 1.399999976158142), (150, 151, 1.5)))

    def doWrite(self, bw):
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        bw2 = pyBigWig.open(oname, "w")
        assert(bw2 is not None)
        #Since this is an unordered dict(), iterating over the items can swap the order!
        chroms = [("1", bw.chroms("1")), ("10", bw.chroms("10"))]
        assert(len(bw.chroms()) == 2)
        bw2.addHeader(chroms, maxZooms=1)
        #Copy the input file
        for c in chroms:
            ints = bw.intervals(c[0])
            chroms2 = []
            starts = []
            ends = []
            values = []
            for entry in ints:
                chroms2.append(c[0])
                starts.append(entry[0])
                ends.append(entry[1])
                values.append(entry[2])
            bw2.addEntries(chroms2, starts, ends=ends, values=values)
        bw2.close()
        #Ensure that the copied file has the same entries and max/min/etc.
        bw2 = pyBigWig.open(oname)
        assert(bw.header() == bw2.header())
        assert(bw.chroms() == bw2.chroms())
        for c in chroms:
            ints1 = bw.intervals(c[0])
            ints2 = bw2.intervals(c[0])
            assert(ints1 == ints2)
        bw.close()
        bw2.close()
        #Clean up
        os.remove(oname)

    def doWrite2(self):
        '''
        Test all three modes of storing entries. Also test to ensure that we get error messages when doing something silly

        This is a modified version of the writing example from libBigWig
        '''
        chroms = ["1"]*6
        starts = [0, 100, 125, 200, 220, 230, 500, 600, 625, 700, 800, 850]
        ends = [5, 120, 126, 205, 226, 231]
        values = [0.0, 1.0, 200.0, -2.0, 150.0, 25.0, 0.0, 1.0, 200.0, -2.0, 150.0, 25.0, -5.0, -20.0, 25.0, -5.0, -20.0, 25.0]
        ofile = tempfile.NamedTemporaryFile(delete=False)
        oname = ofile.name
        ofile.close()
        bw = pyBigWig.open(oname, "w")
        bw.addHeader([("1", 1000000), ("2", 1500000)])

        #Intervals
        bw.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
        bw.addEntries(chroms[3:6], starts[3:6], ends=ends[3:6], values=values[3:6])

        #IntervalSpans
        bw.addEntries("1", starts[6:9], values=values[6:9], span=20)
        bw.addEntries("1", starts[9:12], values=values[9:12], span=20)

        #IntervalSpanSteps, this should instead take an int
        bw.addEntries("1", 900, values=values[12:15], span=20, step=30)
        bw.addEntries("1", 990, values=values[15:18], span=20, step=30)

        #Attempt to add incorrect values. These MUST raise an exception
        try:
            bw.addEntries(chroms[0:3], starts[0:3], ends=ends[0:3], values=values[0:3])
            assert(1==0)
        except RuntimeError:
            pass
        try:
            bw.addEntries("1", starts[6:9], values=values[6:9], span=20)
            assert(1==0)
        except RuntimeError:
            pass
        try:
            bw.addEntries("3", starts[6:9], values=values[6:9], span=20)
            assert(1==0)
        except RuntimeError:
            pass
        try:
            bw.addEntries("1", 900, values=values[12:15], span=20, step=30)
            assert(1==0)
        except RuntimeError:
            pass

        #Add a few intervals on a new chromosome
        bw.addEntries(["2"]*3, starts[0:3], ends=ends[0:3], values=values[0:3])
        bw.close()
        #check md5sum, this is the simplest method to check correctness
        h = hashlib.md5(open(oname, "rb").read()).hexdigest()
        assert(h=="b1ca91d2ff42afdd2efa19a007c1ded4")
        #Clean up
        os.remove(oname)

    def testAll(self):
        bw = self.doOpen()
        self.doChroms(bw)
        self.doHeader(bw)
        self.doStats(bw)
        self.doValues(bw)
        self.doIntervals(bw)
        self.doWrite(bw)
        if self.fname == "pyBigWigTest/test.bw":
            self.doWrite2()
        bw.close()

class TestLocal():
    def testFoo(self):
        blah = TestRemote()
        blah.fname = os.path.dirname(pyBigWig.__file__) + "/pyBigWigTest/test.bw"
        blah.testAll()
