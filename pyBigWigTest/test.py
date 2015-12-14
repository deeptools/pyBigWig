import pyBigWig
import tempfile
import os

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
        chroms = []
        for k,v in bw.chroms().items():
            chroms.append((k, v))
        assert(len(chroms) == 2)
        bw2.addHeader(chroms)
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

    def testAll(self):
        bw = self.doOpen()
        self.doChroms(bw)
        self.doHeader(bw)
        self.doStats(bw)
        self.doValues(bw)
        self.doIntervals(bw)
        self.doWrite(bw)
        bw.close()

class TestLocal():
    def testFoo(self):
        blah = TestRemote()
        blah.fname = "pyBigWigTest/test.bw"
        blah.testAll()
