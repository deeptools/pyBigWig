# pyBigWig
A python extension written in C for quick access to bigWig files. This extension uses the C library functions written by Jim Kent, which have a separate license but are none-the-less available for everyone to use (including for commercial use).

This extension is still under development and the functions may change. In particular, support for writing a bigWig file has yet to be added.

# Installation
You can install this extension directly from github with:

    pip install git+git://github.com/dpryan79/pyBigWig.git

# Usage
Basic usage is as follows:

## Load the extension

    >>> import pyBigWig

## Open a bigWig file

    >>> bw = pyBigWig.open("file.bw")

Note that if the file doesn't exist you'll see an error message and `None` will be returned.

## Access the list of chromosomes and their lengths

`bigWigFile` objects contain a dictionary holding the chromosome lengths, which can be accessed with the `chroms()` accessor.

    >>> bw.chroms()
    dict_proxy({'chrX': 155270560L, 'chr13': 115169878L, 'chr12': 133851895L, 'chr11': 135006516L, 'chr10': 135534747L, 'chr17': 81195210L, 'chr16': 90354753L, 'chr15': 102531392L, 'chr14': 107349540L, 'chr19': 59128983L, 'chr18': 78077248L, 'chr22': 51304566L, 'chr20': 63025520L, 'chr21': 48129895L, 'chr7': 159138663L, 'chr6': 171115067L, 'chr5': 180915260L, 'chr4': 191154276L, 'chr3': 198022430L, 'chr2': 243199373L, 'chr1': 249250621L, 'chr9': 141213431L, 'chr8': 146364022L})

You can also directly query a particular chromosome.

    >>> bw.chroms("chr2")
    243199373L

The lengths are stored a the "long" integer type, which is why there's an `L? suffix. If you specify a non-existant chromosome then nothing is output.

    >>> bw.chroms("c")
    >>> 

## Compute summary information on a range

BigWig files are used to store values associated with positions and ranges of them. Typically we want to quickly access the average value over a range, which is very simple:

    >>> bw.values("chr2", 1000000, 2000000)
    [2.960610510599166]

Suppose instead of the mean value, we instead wanted the maximum value:

    >>> bw.values("chr2", 1000000, 2000000, type="max")
    [53.0]

Other options are "min" (the minimum value), "coverage" (the fraction of bases covered), and "std" (the standard deviation of the values).

It's often the case that we would instead like to compute values of some number of evenly spaced bins in a given interval, which is also simple:

    >>> bw.values("chr2", 1000000, 2000000, type="std", nBins=5)
   [2.046074329512312, 2.261047665458866, 1.8803398932488133, 2.199727647726934, 2.530289082567373]

`nBins` defaults to 1, just as `type` defaults to `mean`.

## Close a bigWig file

A file can be closed with a simple `bw.close()`, as is commonly done with other file types.

