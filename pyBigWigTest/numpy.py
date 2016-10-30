#!/usr/bin/env python
import pyBigWig
import numpy as np
import os

bw = pyBigWig.open("/tmp/delete.bw", "w")
bw.addHeader([("1", 1000)], maxZooms=0)
# Type 0
chroms = np.array(["1"] * 10)
starts = np.array([0, 10, 20, 30, 40, 50, 60, 70, 80, 90], dtype=np.int64)
ends = np.array([5, 15, 25, 35, 45, 55, 65, 75, 85, 95], dtype=np.int64)
values0 = np.array(np.random.random_sample(10), dtype=np.float64)
bw.addEntries(chroms, starts, ends=ends, values=values0)

starts = np.array([100, 110, 120, 130, 140, 150, 160, 170, 180, 190], dtype=np.int64)
ends = np.array([105, 115, 125, 135, 145, 155, 165, 175, 185, 195], dtype=np.int64)
values1 = np.array(np.random.random_sample(10), dtype=np.float64)
bw.addEntries(chroms, starts, ends=ends, values=values1)

# Type 1, single chrom, multiple starts/values, single span
starts = np.array([200, 210, 220, 230, 240, 250, 260, 270, 280, 290], dtype=np.int64)
values2 = np.array(np.random.random_sample(10), dtype=np.float64)
bw.addEntries(np.str("1"), starts, span=np.int(8), values=values2)

starts = np.array([300, 310, 320, 330, 340, 350, 360, 370, 380, 390], dtype=np.int64)
values3 = np.array(np.random.random_sample(10), dtype=np.float64)
bw.addEntries(np.str("1"), starts, span=np.int(8), values=values3)

# Type 2, single chrom/start/span/step, multiple values
values4 = np.array(np.random.random_sample(10), dtype=np.float64)
bw.addEntries(np.str("1"), np.int(400), span=np.int(8), step=np.int64(2), values=values4)

values5 = np.array(np.random.random_sample(10), dtype=np.float64)
bw.addEntries(np.str("1"), np.int(500), span=np.int(8), step=np.int64(2), values=values5)

bw.close()

bw = pyBigWig.open("/tmp/delete.bw")

def compy(start, v2):
    v = []
    for t in bw.intervals("1", start, start + 100):
        v.append(t[2])
    v = np.array(v)
    assert(np.all(np.abs(v - v2) < 1e-5))

compy(0, values0)
compy(100, values1)
compy(200, values2)
compy(300, values3)
compy(400, values4)
compy(500, values5)

bw.close()
os.remove("/tmp/delete.bw")
