#!/usr/bin/env python
import pyBigWig
#bw = pyBigWig.open("test/test.bw")
bw = pyBigWig.open("https://raw.githubusercontent.com/dpryan79/pyBigWig/master/test/test.bw")
print(bw.header())
print(bw.chroms()) #All chromosomes
print(bw.chroms("1")) #"1" exists
print(bw.chroms("c")) #Non-existent chromosome
print(bw.stats("1")) #Entire chromosome
print(bw.stats("1", 0, 3)) #Defaults to type="mean", nBins=1
print(bw.stats("1", 99, 200, type="max"))
print(bw.stats("1", 99, 200, type="max", nBins=2))
print(bw.values("1", 0, 3))
print(bw.values("1", 0, 4)) #Get an "nan"
print(bw.intervals("1")) #All intervals on the chromosome
print(bw.intervals("1",0,4))
print(bw.intervals("10")) #Ensure that we can get chr10 as well
bw.close()
