# pyBigWig
A python extension written in C for quick access to bigWig files. This extension uses [libBigWig](https://github.com/dpryan79/libBigWig) for local and remote file access.

This extension is still under development and the functions may change. Note that compiling with `-O2` or `-O1` seems to result in errors, while `-O0` does not. The cause of this is currently unclear.

# Installation
You can install this extension directly from github with:

    pip install git+git://github.com/dpryan79/pyBigWig.git

# Usage
Basic usage is as follows:

## A note on coordinates

Wiggle and BigWig files use 0-based half-open coordinates, which are also used by this extension. So to access the value for the first base on `chr1`, one would specify the starting position as `0` and the end position as `1`. Similarly, bases 100 to 115 would have a start of `99` and an end of `115`. This is simply for the sake of consistency with the underlying bigWig file and may change in the future.

## Load the extension

    >>> import pyBigWig

## Open a bigWig file

This will work if your working directory is the pyBigWig source code directory.

    >>> bw = pyBigWig.open("test/test.bw")

Note that if the file doesn't exist you'll see an error message and `None` will be returned.

## Access the list of chromosomes and their lengths

`bigWigFile` objects contain a dictionary holding the chromosome lengths, which can be accessed with the `chroms()` accessor.

    >>> bw.chroms()
    dict_proxy({'1': 195471971L, '10': 130694993L})

You can also directly query a particular chromosome.

    >>> bw.chroms("1")
    195471971L

The lengths are stored a the "long" integer type, which is why there's an `L` suffix. If you specify a non-existant chromosome then nothing is output.

    >>> bw.chroms("c")
    >>> 

## Compute summary information on a range

BigWig files are used to store values associated with positions and ranges of them. Typically we want to quickly access the average value over a range, which is very simple:

    >>> bw.stats("1", 0, 3)
    [0.2000000054637591]

Suppose instead of the mean value, we instead wanted the maximum value:

    >>> bw.stats("1", 0, 3, type="max")
    [0.30000001192092896]

Other options are "min" (the minimum value), "coverage" (the fraction of bases covered), and "std" (the standard deviation of the values).

It's often the case that we would instead like to compute values of some number of evenly spaced bins in a given interval, which is also simple:

    >>> bw.stats("1",99,200, type="max", nBins=2)
    [1.399999976158142, 1.5]

`nBins` defaults to 1, just as `type` defaults to `mean`.

## Retrieve values for individual bases in a range

While the `stats()` method **can** be used to retrieve the original values for each base (e.g., by setting `nBins` to the number of bases), it's preferable to instead use the `values()` accessor.

    >>> bw.values("1", 0, 3)
    [0.10000000149011612, 0.20000000298023224, 0.30000001192092896]

The list produced will always contain one value for every base in the range specified. If a particular base has no associated value in the bigWig file then the returned value will be `nan`.

    >>> bw.values("1", 0, 4)
    [0.10000000149011612, 0.20000000298023224, 0.30000001192092896, nan]

## Close a bigWig file

A file can be closed with a simple `bw.close()`, as is commonly done with other file types.

# To do

 - [ ] Use numpy arrays instead of lists? This requires having numpy installed, which seems rather over the top but Fidel might want this for deeptools.
 - [ ] Writer functions? It's unclear how much these would even be used.
