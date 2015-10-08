#!/usr/bin/env python
import pyBigWig
#bw = pyBigWig.open("test/test.bw")
bw = pyBigWig.open("https://raw.githubusercontent.com/dpryan79/pyBigWig/master/test/test.bw")
print(bw.chroms())
print(bw.chroms("1"))
print(bw.chroms("c"))
print(bw.stats("1", 0, 3))
print(bw.stats("1", 99, 200, type="max"))
print(bw.stats("1", 99, 200, type="max", nBins=2))
print(bw.values("1", 0, 3))
print(bw.values("1", 0, 4))
bw.close()
