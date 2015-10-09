#include <Python.h>
#include <structmember.h>
#include "bigWig.h"

typedef struct {
    PyObject_HEAD
    bigWigFile_t *bw;
} pyBigWigFile_t;

static PyObject* pyBwOpen(PyObject *self, PyObject *pyFname);
static PyObject* pyBwClose(pyBigWigFile_t *pybw, PyObject *args);
static PyObject *pyBwGetChroms(pyBigWigFile_t *pybw, PyObject *args);
static PyObject *pyBwGetStats(pyBigWigFile_t *pybw, PyObject *args, PyObject *kwds);
static PyObject *pyBwGetValues(pyBigWigFile_t *pybw, PyObject *args, PyObject *kwds);
static void pyBwDealloc(pyBigWigFile_t *pybw);

//The function types aren't actually correct...
static PyMethodDef bwMethods[] = {
    {"open", (PyCFunction)pyBwOpen, METH_VARARGS,
"Open a bigWig file.\n\
\n\
Returns:\n\
   A bigWigFile object on success, otherwise None.\n\
\n\
Arguments:\n\
    file: The name of a bigWig file.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"some_file.bw\")\n"},
    {"close", (PyCFunction)pyBwClose, METH_VARARGS,
"Close a bigWig file.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"some_file.bw\")\n\
>>> bw.close()\n"},
    {"chroms", (PyCFunction)pyBwGetChroms, METH_VARARGS,
"Return a chromosome: length dictionary. The order is typically not alphabetical\n\
and the lengths are long (thus the 'L' suffix).\n\
\n\
Optional arguments:\n\
    chrom: An optional chromosome name\n\
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
"Return summary statistics for a given range.\n\
\n\
Positional arguments:\n\
    chr:   Chromosome name\n\
    start: Starting position\n\
    end:   Ending position\n\
\n\
Keyword arguments:\n\
    type:  Summary type (mean, min, max, coverage, std), default 'mean'.\n\
    nBins: Number of bins into which the range should be divided before computing\n\
           summary statistics, default 1.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"test/test.bw\")\n\
>>> bw.stats(\"1\", 0, 3)\n\
[0.2000000054637591]\n\
\n\
This is the mean value over the range 1:1-3 (in 1-based coordinates). There are\n\
additional optional parameters 'type' and 'nBins'. 'type' specifies the type of\n\
summary information to calculate, which is 'mean' by default. Other possibilites\n\
for 'type' are: 'min' (minimum value), 'max' (maximum value), 'coverage' (number\n\
of covered bases), and 'std' (standard deviation). 'nBins' defines how many bins\n\
the region will be divided into and defaults to 1.\n\
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
"Retrieve the value stored for each position (or None)\n\
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
The length of the returned list will always match the length of the range. Any\n\
uncovered bases will have a value of None.\n\
\n\
>>> bw.values(\"1\", 0, 4)\n\
[0.10000000149011612, 0.20000000298023224, 0.30000001192092896, None]\n\
\n"},
    {NULL, NULL, 0, NULL}
};

//Should set tp_dealloc, tp_print, tp_repr, tp_str, tp_members
static PyTypeObject bigWigFile = {
    PyObject_HEAD_INIT(NULL)
    0,              /*ob_size*/
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
    Py_TPFLAGS_HAVE_CLASS,     /*tp_flags*/
    "bigWig File",             /*tp_doc*/
    0,0,0,0,
    0,
    0,
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
