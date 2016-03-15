#include <Python.h>
#include <structmember.h>
#include "bigWig.h"

typedef struct {
    PyObject_HEAD
    bigWigFile_t *bw;
    int32_t lastTid; //The TID of the last written entry (or -1)
    uint32_t lastSpan; //The span of the last written entry (if applicable)
    uint32_t lastStep; //The step of the last written entry (if applicable)
    uint32_t lastStart; //The next start position (if applicable)
    int lastType; //The type of the last written entry
} pyBigWigFile_t;

static PyObject* pyBwOpen(PyObject *self, PyObject *pyFname);
static PyObject* pyBwClose(pyBigWigFile_t *pybw, PyObject *args);
static PyObject *pyBwGetChroms(pyBigWigFile_t *pybw, PyObject *args);
static PyObject *pyBwGetStats(pyBigWigFile_t *pybw, PyObject *args, PyObject *kwds);
static PyObject *pyBwGetValues(pyBigWigFile_t *pybw, PyObject *args);
static PyObject *pyBwGetIntervals(pyBigWigFile_t *pybw, PyObject *args, PyObject *kwds);
static PyObject *pyBwGetHeader(pyBigWigFile_t *pybw, PyObject *args);
static PyObject *pyBwAddHeader(pyBigWigFile_t *pybw, PyObject *args, PyObject *kwds);
static PyObject *pyBwAddEntries(pyBigWigFile_t *pybw, PyObject *args, PyObject *kwds);
static void pyBwDealloc(pyBigWigFile_t *pybw);

//The function types aren't actually correct...
static PyMethodDef bwMethods[] = {
    {"open", (PyCFunction)pyBwOpen, METH_VARARGS,
"Open a bigWig file. For remote files, give a URL starting with HTTP,\n\
FTP, or HTTPS.\n\
\n\
Optional arguments:\n\
    mode: An optional mode. The default is 'r', which opens a file for reading.\n\
          If you specify a mode containing 'w' then you'll instead open a file\n\
          for writing. Note that you then need to add an appropriate header\n\
          before use.\n\
\n\
Returns:\n\
   A bigWigFile object on success, otherwise None.\n\
\n\
Arguments:\n\
    file: The name of a bigWig file.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"some_file.bw\")\n"},
    {"header", (PyCFunction)pyBwGetHeader, METH_VARARGS,
"Returns the header of a bigWig file. This contains information such as: \n\
  * The version number of the file ('version').\n\
  * The number of zoom levels ('nLevels').\n\
  * The number of bases covered ('nBasesCovered').\n\
  * The minimum value ('minVal').\n\
  * The maximum value ('maxVal').\n\
  * The sum of all values ('sumData').\n\
  * The sum of the square of all values ('sumSquared').\n\
These are returned as a dictionary.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"some_file.bw\")\n\
>>> bw.header()\n\
{'maxVal': 2L, 'sumData': 272L, 'minVal': 0L, 'version': 4L,\n\
'sumSquared': 500L, 'nLevels': 1L, 'nBasesCovered': 154L}\n\
>>> bw.close()\n"},

    {"close", (PyCFunction)pyBwClose, METH_VARARGS,
"Close a bigWig file.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"some_file.bw\")\n\
>>> bw.close()\n"},
    {"chroms", (PyCFunction)pyBwGetChroms, METH_VARARGS,
"Return a chromosome: length dictionary. The order is typically not\n\
alphabetical and the lengths are long (thus the 'L' suffix).\n\
\n\
Optional arguments:\n\
    chrom: An optional chromosome name\n\
\n\
Returns:\n\
    A list of chromosome lengths or a dictionary of them.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"test/test.bw\")\n\
>>> bw.chroms()\n\
{'1': 195471971L, '10': 130694993L}\n\
\n\
Note that you may optionally supply a specific chromosome:\n\
\n\
>>> bw.chroms(\"chr1\")\n\
195471971L\n\
\n\
If you specify a non-existant chromosome then no output is produced:\n\
\n\
>>> bw.chroms(\"foo\")\n\
>>>\n"},
    {"stats", (PyCFunction)pyBwGetStats, METH_VARARGS|METH_KEYWORDS,
"Return summary statistics for a given range. On error, this function throws a\n\
runtime exception.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
\n\
Keyword arguments:\n\
    start: Starting position\n\
    end:   Ending position\n\
    type:  Summary type (mean, min, max, coverage, std), default 'mean'.\n\
    nBins: Number of bins into which the range should be divided before\n\
           computing summary statistics. The default is 1.\n\
    exact: By default, pyBigWig uses the same method as Kent's tools from UCSC\n\
           for computing statistics. This means that 'zoom levels' may be\n\
           used, rather than actual values (please see the pyBigWig repository\n\
           on github for further information on this). To avoid this behaviour,\n\
           simply specify 'exact=True'. Note that values returned will then\n\
           differ from what UCSC, IGV, and similar other tools will report.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"test/test.bw\")\n\
>>> bw.stats(\"1\", 0, 3)\n\
[0.2000000054637591]\n\
\n\
This is the mean value over the range 1:1-3 (in 1-based coordinates). If\n\
the start and end positions aren't given the entire chromosome is used.\n\
There are additional optional parameters 'type' and 'nBins'. 'type'\n\
specifies the type of summary information to calculate, which is 'mean'\n\
by default. Other possibilites for 'type' are: 'min' (minimum value),\n\
'max' (maximum value), 'coverage' (number of covered bases), and 'std'\n\
 (standard deviation). 'nBins' defines how many bins the region will be\n\
 divided into and defaults to 1.\n\
\n\
>>> bw.stats(\"1\", 0, 3, type=\"min\")\n\
[0.10000000149011612]\n\
>>> bw.stats(\"1\", 0, 3, type=\"max\")\n\
[0.30000001192092896]\n\
>>> bw.stats(\"1\", 0, 10, type=\"coverage\")\n\
[0.30000000000000004]\n\
>>> bw.stats(\"1\", 0, 3, type=\"std\")\n\
[0.10000000521540645]\n\
>>> bw.stats(\"1\",99,200, type=\"max\", nBins=2)\n\
[1.399999976158142, 1.5]\n"},
    {"values", (PyCFunction)pyBwGetValues, METH_VARARGS,
"Retrieve the value stored for each position (or None). On error, a runtime\n\
exception is thrown.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
    start: Starting position\n\
    end:   Ending position\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"test/test.bw\")\n\
>>> bw.values(\"1\", 0, 3)\n\
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896]\n\
\n\
The length of the returned list will always match the length of the\n\
range. Any uncovered bases will have a value of None.\n\
\n\
>>> bw.values(\"1\", 0, 4)\n\
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896, None]\n\
\n"},
    {"intervals", (PyCFunction)pyBwGetIntervals, METH_VARARGS|METH_KEYWORDS,
"Retrieve each interval covering a part of a chromosome/region. On error, a\n\
runtime exception is thrown.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
\n\
Keyword arguments:\n\
    start: Starting position\n\
    end:   Ending position\n\
\n\
If start and end aren't specified, the entire chromosome is returned.\n\
The returned object is a tuple containing the starting position, end\n\
position, and value of each interval in the file. As with all bigWig\n\
positions, those returned are 0-based half-open (e.g., a start of 0 and\n\
end of 10 specifies the first 10 positions).\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"test/test.bw\")\n\
>>> bw.intervals(\"1\", 0, 3)\n\
((0, 1, 0.10000000149011612), (1, 2, 0.20000000298023224),\n\
 (2, 3, 0.30000001192092896))\n\
>>> bw.close()"},
    {"addHeader", (PyCFunction)pyBwAddHeader, METH_VARARGS|METH_KEYWORDS,
"Adds a header to a file opened for writing. This MUST be called before adding\n\
any entries. On error, a runtime exception is thrown.\n\
\n\
Positional arguments:\n\
    cl:    A chromosome list, of the form (('chr1', 1000), ('chr2', 2000), ...).\n\
           In other words, each element of the list is a tuple containing a\n\
           chromosome name and its associated length.\n\
\n\
Keyword arguments:\n\
    maxZooms:  The maximum number of zoom levels. The value must be >=0. The\n\
               default is 10.\n\
\n\
>>> import pyBigWig\n\
>>> import tempfile\n\
>>> import os\n\
>>> ofile = tempfile.NamedTemporaryFile(delete=False)\n\
>>> oname = ofile.name\n\
>>> ofile.close()\n\
>>> bw = pyBigWig.open(oname, 'w')\n\
>>> bw.addHeader([(\"1\", 1000000), (\"2\", 1500000)], maxZooms=0)\n\
>>> bw.close()\n\
>>> os.remove(oname)"},
    {"addEntries", (PyCFunction)pyBwAddEntries, METH_VARARGS|METH_KEYWORDS,
"Adds one or more entries to a bigWig file. This returns nothing, but throws a\n\
runtime exception on error.\n\
\n\
This function always accepts an optional 'validate' option. If set to 'True',\n\
which is the default, the input entries are checked to ensure that they come\n\
after previously entered entries. This comes with significant overhead, so if\n\
this is instead 'False' then this validation is not performed.\n\
\n\
There are three manners in which entries can be stored in bigWig files.\n\
\n\
\n\
bedGraph-like entries (12 bytes each):\n\
\n\
Positional arguments:\n\
    chrom:  A list of chromosome. These MUST match those added with addHeader().\n\
    starts: A list of start positions. These are 0-based.\n\
\n\
Keyword arguments:\n\
    ends:   A list of end positions. These are 0-based half open, so a start of\n\
            0 and end of 10 specifies the first 10 bases.\n\
    values: A list of values.\n\
\n\
\n\
Variable-step entries (8 bytes each):\n\
\n\
Positional arguments:\n\
    chrom:  A chromosome name. This MUST match one added with addHeader().\n\
    starts: A list of start positions. These are 0-based.\n\
\n\
Keyword arguments:\n\
    values: A list of values.\n\
    span:   A span width. This is an integer value and specifies how many bases\n\
            each entry describes. An entry with a start position of 0 and a span\n\
            of 10 describes the first 10 bases.\n\
\n\
\n\
Fixed-step entries (4 bytes each):\n\
\n\
Positional arguments:\n\
    chrom:  A chromosome name. This MUST match one added with addHeader().\n\
    starts: A start position. These are 0-based. The start position of each\n\
            entry starts 'step' after the previous and describes 'span' bases.\n\
\n\
Keyword arguments:\n\
    values: A list of values.\n\
    span:   A span width. This is an integer value and specifies how many bases\n\
            each entry describes. An entry with a start position of 0 and a span\n\
            of 10 describes the first 10 bases.\n\
    step:   A step width. Each subsequent entry begins this number of bases\n\
            after the previous. So if the first entry has a start of 0 and step\n\
            or 30, the second entry will start at 30.\n\
\n\
>>> import pyBigWig\n\
>>> import tempfile\n\
>>> import os\n\
>>> ofile = tempfile.NamedTemporaryFile(delete=False)\n\
>>> oname = ofile.name\n\
>>> ofile.close()\n\
>>> bw = pyBigWig.open(oname, 'w')\n\
>>> bw.addHeader([(\"1\", 1000000), (\"2\", 1500000)])\n\
>>> #Add some bedGraph-like entries\n\
>>> bw.addEntries([\"1\", \"1\", \"1\"], [0, 100, 125], ends=[5, 120, 126], values=[0.0, 1.0, 200.0])\n\
>>> #Variable-step entries, the span 500-520, 600-620, and 635-655\n\
>>> bw.addEntries(\"1\", [500, 600, 635], values=[-2.0, 150.0, 25.0], span=20)\n\
>>> #Fixed-step entries, the bases described are 900-920, 930-950, and 960-980\n\
>>> bw.addEntries(\"1\", 900, values=[-5.0, -20.0, 25.0], span=20, step=30)\n\
>>> #This only works due to using validate=False. Obviously the file is then corrupt.\n\
>>> bw.addEntries([\"1\", \"1\", \"1\"], [0, 100, 125], ends=[5, 120, 126], values=[0.0, 1.0, 200.0], validate=False)\n\
>>> bw.close()\n\
>>> os.remove(oname)"},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
struct pyBigWigmodule_state {
    PyObject *error;
};

#define GETSTATE(m) ((struct pyBigWigmodule_state*)PyModule_GetState(m))

static PyModuleDef pyBigWigmodule = {
    PyModuleDef_HEAD_INIT,
    "pyBigWig",
    "A python module for bigWig file access",
    -1,
    bwMethods,
    NULL, NULL, NULL, NULL
};
#endif

//Should set tp_dealloc, tp_print, tp_repr, tp_str, tp_members
static PyTypeObject bigWigFile = {
#if PY_MAJOR_VERSION >= 3
    PyVarObject_HEAD_INIT(NULL, 0)
#else
    PyObject_HEAD_INIT(NULL)
    0,              /*ob_size*/
#endif
    "pyBigWig.bigWigFile",     /*tp_name*/
    sizeof(pyBigWigFile_t),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)pyBwDealloc,     /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash*/
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro*/
    0,                         /*tp_as_buffer*/
#if PY_MAJOR_VERSION >= 3
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
#else
    Py_TPFLAGS_HAVE_CLASS,     /*tp_flags*/
#endif
    "bigWig File",             /*tp_doc*/
    0,                         /*tp_traverse*/
    0,                         /*tp_clear*/
    0,                         /*tp_richcompare*/
    0,                         /*tp_weaklistoffset*/
    0,                         /*tp_iter*/
    0,                         /*tp_iternext*/
    bwMethods,                 /*tp_methods*/
    0,                         /*tp_members*/
    0,                         /*tp_getset*/
    0,                         /*tp_base*/
    0,                         /*tp_dict*/
    0,                         /*tp_descr_get*/
    0,                         /*tp_descr_set*/
    0,                         /*tp_dictoffset*/
    0,                         /*tp_init*/
    0,                         /*tp_alloc*/
    0,                         /*tp_new*/
    0,0,0,0,0,0
};
