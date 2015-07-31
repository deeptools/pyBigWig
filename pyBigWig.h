#include <Python.h>
#include <structmember.h>
#include "bigWig.h"

typedef struct {
    PyObject_HEAD
    struct bbiFile *bbi;
    PyObject *chroms; //A chromosome: length dictionary
} bigWigFile_t;

static PyObject* bwOpen(PyObject *self, PyObject *pyFname);
static PyObject* bwClose(bigWigFile_t *bbi, PyObject *args);
static PyObject *bwGetChroms(bigWigFile_t *bbi, PyObject *args);
static PyObject *bwGetValRange(bigWigFile_t *bbi, PyObject *args, PyObject *kwds);
//static PyObject *bwGetValRange(bigWigFile_t *bbi, PyObject *args);
PyObject *printBW(PyObject *o);
static void bwDealloc(bigWigFile_t *bbi);

//The function types aren't actually correct...
static PyMethodDef bwMethods[] = {
    {"open", (PyCFunction)bwOpen, METH_VARARGS,
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
    {"close", (PyCFunction)bwClose, METH_VARARGS,
"Close a bigWig file.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"some_file.bw\")\n\
>>> bw.close()\n"},
    {"chroms", (PyCFunction)bwGetChroms, METH_VARARGS,
"Return a chromosome: length dictionary. The order is typically not alphabetical\n\
and the lengths are long (thus the 'L' suffix).\n\
\n\
Optional arguments:\n\
    chrom: An optional chromosome name\n\
Returns:\n\
    A list of chromosome lengths or a dictionary of them.\n\
\n\
>>> import pyBigWig\n\
>>> bw = pyBigWig.open(\"some_file.bw\")\n\
>>> bw.chroms()\n\
'1': 195471971L, 'GL456389.1': 28772L, '3': 160039680L, '2': 182113224L,\n\
...\n\
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
    {"values", (PyCFunction)bwGetValRange, METH_VARARGS|METH_KEYWORDS,
"Return summary values for a given range.\n\
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
>>> bw = pyBigWig.open(\"some_file.bw\")\n\
>>> bw.values(\"chr1\", 1000000, 2000000)\n\
[17.7491145717263]\n\
\n\
This is the mean value over the range chr1:1000000-2000000. There are additional\n\
optional parameters 'type' and 'nBins'. 'type' specifies the type of summary\n\
information to calculate, which is 'mean' by default. Other possibilites for\n\
'type' are: 'min' (minimum value), 'max' (maximum value), 'coverage' (number of\n\
covered bases), and 'std' (standard deviation). 'nBins' defines how many bins\n\
the region will be divided into and defaults to 1.\n\
\n\
>>> bw.values(\"chr1\", 1000000, 2000000, type=\"min\")\n\
[1.0]\n\
>>> bw.values(\"chr1\", 1000000, 2000000, type=\"max\")\n\
[2707.0]\n\
>>> bw.values(\"chr1\", 1000000, 2000000, type=\"coverage\")\n\
[0.878054]\n\
>>> bw.values(\"chr1\", 1000000, 2000000, type=\"std\")\n\
[87.55780895322955]\n\
>>> bw.values(\"chr1\", 1000000, 2000000, type=\"coverage\", nBins=5)\n\
[0.932795, 0.9187200000000001, 0.8186000000000001, 0.78622, 0.9180900000000001]\n\
>>> bw.values(\"chr1\", 1000000, 2000000, nBins=5)\n\
[17.354876630539948, 23.50821450484022, 18.657274436073084, 14.496840947098928, 14.572063763860072]\n"},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef bwMembers[] = {
    {"chroms", T_OBJECT_EX, offsetof(bigWigFile_t, chroms), READONLY, "A dictionary with chromosomes as keys and chromosome lengths as values"},
    {NULL}
};

//Should set tp_dealloc, tp_print, tp_repr, tp_str, tp_members
static PyTypeObject bigWigFile = {
    PyObject_HEAD_INIT(NULL)
    0,              /*ob_size*/
    "pyBigWig.bigWigFile",     /*tp_name*/
    sizeof(bigWigFile_t),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)bwDealloc,     /*tp_dealloc*/
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
    bwMembers,                 /*tp_members*/
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
