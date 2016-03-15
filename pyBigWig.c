#include <Python.h>
#include <assert.h>
#include <inttypes.h>
#include "pyBigWig.h"

//Need to add proper error handling rather than just assert()
PyObject* pyBwOpen(PyObject *self, PyObject *pyFname) {
    char *fname = NULL;
    char *mode = "r";
    pyBigWigFile_t *pybw;
    bigWigFile_t *bw = NULL;

    if(!PyArg_ParseTuple(pyFname, "s|s", &fname, &mode)) goto error;

    //Open the local/remote file
    bw = bwOpen(fname, NULL, mode);
    if(!bw) goto error;
    if(!mode || !strchr(mode, 'w')) {
        if(!bw->cl) goto error;
    }

    pybw = PyObject_New(pyBigWigFile_t, &bigWigFile);
    if(!pybw) goto error;
    pybw->bw = bw;
    pybw->lastTid = -1;
    pybw->lastType = -1;
    pybw->lastSpan = (uint32_t) -1;
    pybw->lastStep = (uint32_t) -1;
    pybw->lastStart = (uint32_t) -1;
    return (PyObject*) pybw;

error:
    if(bw) bwClose(bw);
    PyErr_SetString(PyExc_RuntimeError, "Received an error during file opening!");
    return NULL;
}

static void pyBwDealloc(pyBigWigFile_t *self) {
    if(self->bw) bwClose(self->bw);
    PyObject_DEL(self);
}

static PyObject *pyBwClose(pyBigWigFile_t *self, PyObject *args) {
    bwClose(self->bw);
    self->bw = NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

//Accessor for the header (version, nLevels, nBasesCovered, minVal, maxVal, sumData, sumSquared
static PyObject *pyBwGetHeader(pyBigWigFile_t *self, PyObject *args) {
    bigWigFile_t *bw = self->bw;
    PyObject *ret, *val;

    ret = PyDict_New();
    val = PyLong_FromUnsignedLong(bw->hdr->version);
    if(PyDict_SetItemString(ret, "version", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromUnsignedLong(bw->hdr->nLevels);
    if(PyDict_SetItemString(ret, "nLevels", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromUnsignedLongLong(bw->hdr->nBasesCovered);
    if(PyDict_SetItemString(ret, "nBasesCovered", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromDouble(bw->hdr->minVal);
    if(PyDict_SetItemString(ret, "minVal", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromDouble(bw->hdr->maxVal);
    if(PyDict_SetItemString(ret, "maxVal", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromDouble(bw->hdr->sumData);
    if(PyDict_SetItemString(ret, "sumData", val) == -1) goto error;
    Py_DECREF(val);
    val = PyLong_FromDouble(bw->hdr->sumSquared);
    if(PyDict_SetItemString(ret, "sumSquared", val) == -1) goto error;
    Py_DECREF(val);

    return ret;

error :
    Py_XDECREF(val);
    Py_XDECREF(ret);
    PyErr_SetString(PyExc_RuntimeError, "Received an error while getting the bigWig header!");
    return NULL;
}

//Accessor for the chroms, args is optional
static PyObject *pyBwGetChroms(pyBigWigFile_t *self, PyObject *args) {
    PyObject *ret = NULL, *val;
    bigWigFile_t *bw = self->bw;
    char *chrom = NULL;
    uint32_t i;

    if(!(PyArg_ParseTuple(args, "|s", &chrom)) || !chrom) {
        ret = PyDict_New();
        for(i=0; i<bw->cl->nKeys; i++) {
            val = PyLong_FromUnsignedLong(bw->cl->len[i]);
            if(PyDict_SetItemString(ret, bw->cl->chrom[i], val) == -1) goto error;
            Py_DECREF(val);
        }
    } else {
        for(i=0; i<bw->cl->nKeys; i++) {
            if(strcmp(bw->cl->chrom[i],chrom) == 0) {
                ret = PyLong_FromUnsignedLong(bw->cl->len[i]);
                break;
            }
        }
    }

    if(!ret) {
        Py_INCREF(Py_None);
        ret = Py_None;
    }

    return ret;

error :
    Py_XDECREF(val);
    Py_XDECREF(ret);
    PyErr_SetString(PyExc_RuntimeError, "Received an error while adding an item to the output dictionary!");
    return NULL;
}

enum bwStatsType char2enum(char *s) {
    if(strcmp(s, "mean") == 0) return mean;
    if(strcmp(s, "std") == 0) return std;
    if(strcmp(s, "dev") == 0) return dev;
    if(strcmp(s, "max") == 0) return max;
    if(strcmp(s, "min") == 0) return min;
    if(strcmp(s, "cov") == 0) return cov;
    if(strcmp(s, "coverage") == 0) return cov;
    return -1;
};

//Fetch summary statistics, default is the mean of the entire chromosome.
static PyObject *pyBwGetStats(pyBigWigFile_t *self, PyObject *args, PyObject *kwds) {
    bigWigFile_t *bw = self->bw;
    double *val;
    uint32_t start, end = -1, tid;
    unsigned long startl = 0, endl = -1;
    static char *kwd_list[] = {"chrom", "start", "end", "type", "nBins", "exact", NULL};
    char *chrom, *type = "mean";
    PyObject *ret, *exact = Py_False;
    int i, nBins = 1;
    errno = 0; //In the off-chance that something elsewhere got an error and didn't clear it...

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|kksiO", kwd_list, &chrom, &startl, &endl, &type, &nBins, &exact)) {
        PyErr_SetString(PyExc_RuntimeError, "You must supply at least a chromosome!");
        return NULL;
    }

    //Check inputs, reset to defaults if nothing was input
    if(!nBins) nBins = 1; //For some reason, not specifying this overrides the default!
    if(!type) type = "mean";
    tid = bwGetTid(bw, chrom);
    if(endl == (unsigned long) -1 && tid != (uint32_t) -1) endl = bw->cl->len[tid];
    if(tid == (uint32_t) -1 || startl > end || endl > end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }
    start = (uint32_t) startl;
    end = (uint32_t) endl;
    if(end <= start || end > bw->cl->len[tid] || start >= end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    if(char2enum(type) == doesNotExist) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid type!");
        return NULL;
    }

    //Get the actual statistics
    if(exact == Py_True) {
        val = bwStatsFromFull(bw, chrom, start, end, nBins, char2enum(type));
    } else {
        val = bwStats(bw, chrom, start, end, nBins, char2enum(type));
    }

    if(!val) {
        PyErr_SetString(PyExc_RuntimeError, "An error was encountered while fetching statistics.");
        return NULL;
    }

    ret = PyList_New(nBins);
    for(i=0; i<nBins; i++) {
        if(isnan(val[i])) {
            Py_INCREF(Py_None);
            PyList_SetItem(ret, i, Py_None);
        } else {
            PyList_SetItem(ret, i, PyFloat_FromDouble(val[i]));
        }
    }
    free(val);

    return ret;
}

//Fetch a list of individual values
//For bases with no coverage, the value should be None
static PyObject *pyBwGetValues(pyBigWigFile_t *self, PyObject *args) {
    bigWigFile_t *bw = self->bw;
    int i;
    uint32_t start, end = -1, tid;
    unsigned long startl, endl;
    char *chrom;
    PyObject *ret;
    bwOverlappingIntervals_t *o;

    if(!PyArg_ParseTuple(args, "skk", &chrom, &startl, &endl)) {
        PyErr_SetString(PyExc_RuntimeError, "You must supply a chromosome, start and end position.\n");
        return NULL;
    }

    tid = bwGetTid(bw, chrom);
    if(endl == (unsigned long) -1 && tid != (uint32_t) -1) endl = bw->cl->len[tid];
    if(tid == (uint32_t) -1 || startl > end || endl > end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }
    start = (uint32_t) startl;
    end = (uint32_t) endl;
    if(end <= start || end > bw->cl->len[tid] || start >= end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    o = bwGetValues(self->bw, chrom, start, end, 1);
    if(!o) {
        PyErr_SetString(PyExc_RuntimeError, "An error occurred while fetching values!");
        return NULL;
    }

    ret = PyList_New(end-start);
    for(i=0; i<(int) o->l; i++) PyList_SetItem(ret, i, PyFloat_FromDouble(o->value[i]));
    bwDestroyOverlappingIntervals(o);

    return ret;
}

static PyObject *pyBwGetIntervals(pyBigWigFile_t *self, PyObject *args, PyObject *kwds) {
    bigWigFile_t *bw = self->bw;
    uint32_t start, end = -1, tid, i;
    unsigned long startl = 0, endl = -1;
    static char *kwd_list[] = {"chrom", "start", "end", NULL};
    bwOverlappingIntervals_t *intervals = NULL;
    char *chrom;
    PyObject *ret;

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|kk", kwd_list, &chrom, &startl, &endl)) {
        PyErr_SetString(PyExc_RuntimeError, "You must supply at least a chromosome.\n");
        return NULL;
    }

    //Sanity check
    tid = bwGetTid(bw, chrom);
    if(endl == (unsigned long) -1 && tid != (uint32_t) -1) endl = bw->cl->len[tid];
    if(tid == (uint32_t) -1 || startl > end || endl > end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }
    start = (uint32_t) startl;
    end = (uint32_t) endl;
    if(end <= start || end > bw->cl->len[tid] || start >= end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    //Get the intervals
    intervals = bwGetOverlappingIntervals(bw, chrom, start, end);
    if(!intervals) {
        PyErr_SetString(PyExc_RuntimeError, "An error occurred while fetching the overlapping intervals!");
        return NULL;
    }
    if(!intervals->l) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    ret = PyTuple_New(intervals->l);
    for(i=0; i<intervals->l; i++) {
        if(PyTuple_SetItem(ret, i, Py_BuildValue("(iif)", intervals->start[i], intervals->end[i], intervals->value[i]))) {
            Py_DECREF(ret);
            bwDestroyOverlappingIntervals(intervals);
            PyErr_SetString(PyExc_RuntimeError, "An error occurred while constructing the output tuple!");
            return NULL;
        }
    }

    bwDestroyOverlappingIntervals(intervals);
    return ret;
}

#if PY_MAJOR_VERSION >= 3
//Return 1 iff obj is a ready unicode type
int PyString_Check(PyObject *obj) {
    if(PyUnicode_Check(obj)) {
        return PyUnicode_READY(obj)+1;
    }
    return 0;
}

//I don't know what happens if PyBytes_AsString(NULL) is used...
char *PyString_AsString(PyObject *obj) {
    return PyBytes_AsString(PyUnicode_AsASCIIString(obj));
}
#endif

//Will return 1 for long or int types currently
int isNumeric(PyObject *obj) {
#if PY_MAJOR_VERSION < 3
    if(PyInt_Check(obj)) return 1;
#endif
    return PyLong_Check(obj);
}

//On error, throws a runtime error, so use PyErr_Occurred() after this
uint32_t Numeric2Uint(PyObject *obj) {
    long l;
#if PY_MAJOR_VERSION < 3
    if(PyInt_Check(obj)) {
        return (uint32_t) PyInt_AsLong(obj);
    }
#endif
    l = PyLong_AsLong(obj);
    //Check bounds
    if(l > 0xFFFFFFFF) {
        PyErr_SetString(PyExc_RuntimeError, "Length out of bounds for a bigWig file!");
        return (uint32_t) -1;
    }
    return (uint32_t) l;
}

//This runs bwCreateHdr, bwCreateChromList, and bwWriteHdr
PyObject *pyBwAddHeader(pyBigWigFile_t *self, PyObject *args, PyObject *kwds) {
    bigWigFile_t *bw = self->bw;
    char **chroms = NULL;
    int64_t n;
    uint32_t *lengths = NULL, len;
    int32_t maxZooms = 10;
    long zoomTmp = 10;
    static char *kwd_list[] = {"cl", "maxZooms", NULL};
    PyObject *InputTuple = NULL, *tmpObject, *tmpObject2;
    Py_ssize_t i, pyLen;

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "O|k", kwd_list, &InputTuple, &zoomTmp)) {
        PyErr_SetString(PyExc_RuntimeError, "Illegal arguments");
        return NULL;
    }
    maxZooms = zoomTmp;

    //Ensure that we received a list
    if(!PyList_Check(InputTuple)) {
        PyErr_SetString(PyExc_RuntimeError, "You MUST input a list of tuples (e.g., [('chr1', 1000), ('chr2', 2000)]!");
        goto error;
    }
    pyLen = PyList_Size(InputTuple);
    if(pyLen < 1) {
        PyErr_SetString(PyExc_RuntimeError, "You input an empty list!");
        goto error;
    }
    n = pyLen;

    lengths = calloc(n, sizeof(uint32_t));
    chroms = calloc(n, sizeof(char*));
    if(!lengths || !chroms) {
        PyErr_SetString(PyExc_RuntimeError, "Couldn't allocate lengths or chroms!");
        goto error;
    }

    //Convert the tuple into something more useful in C
    for(i=0; i<pyLen; i++) {
        tmpObject = PyList_GetItem(InputTuple, i);
        if(!tmpObject) {
            PyErr_SetString(PyExc_RuntimeError, "Couldn't get a tuple!");
            goto error;
        }
        if(!PyTuple_Check(tmpObject)) {
            PyErr_SetString(PyExc_RuntimeError, "The input list is not made up of tuples!");
            goto error;
        }
        if(PyTuple_Size(tmpObject) != 2) {
            PyErr_SetString(PyExc_RuntimeError, "One tuple does not contain exactly 2 members!");
            goto error;
        }

        //Chromosome
        tmpObject2 = PyTuple_GetItem(tmpObject, 0); //This returns NULL in python3?!?
        if(!PyString_Check(tmpObject2)) {
            PyErr_SetString(PyExc_RuntimeError, "The first element of each tuple MUST be a string!");
            goto error;
        }
        chroms[i] = PyString_AsString(tmpObject2);
        if(!chroms[i]) {
            PyErr_SetString(PyExc_RuntimeError, "Received something other than a string for a chromosome name!");
            goto error;
        }

        //Length
        tmpObject2 = PyTuple_GetItem(tmpObject, 1);
        if(!isNumeric(tmpObject2)) {
            PyErr_SetString(PyExc_RuntimeError, "The second element of each tuple MUST be an integer!");
            goto error;
        }
        len = Numeric2Uint(tmpObject2);
        if(PyErr_Occurred()) goto error;
        if(zoomTmp > 0xFFFFFFFF) {
            PyErr_SetString(PyExc_RuntimeError, "A requested length is longer than what can be stored in a bigWig file!");
            goto error;
        }
        lengths[i] = len;
    }

    //Create the header
    if(bwCreateHdr(bw, maxZooms)) {
        PyErr_SetString(PyExc_RuntimeError, "Received an error in bwCreateHdr");
        goto error;
    }

    //Create the chromosome list
    bw->cl = bwCreateChromList(chroms, lengths, n);
    if(!bw->cl) {
        PyErr_SetString(PyExc_RuntimeError, "Received an error in bwCreateChromList");
        goto error;
    }

    //Write the header
    if(bwWriteHdr(bw)) {
        PyErr_SetString(PyExc_RuntimeError, "Received an error while writing the bigWig header");
        goto error;
    }

    Py_INCREF(Py_None);
    return Py_None;

error:
    if(lengths) free(lengths);
    if(chroms) free(chroms);
    return NULL;
}

//1 on true, 0 on false
int isType0(PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *values) {
    int rv = 0;
    Py_ssize_t i, sz;
    PyObject *tmp;

    if(chroms && starts && ends) {
        if(!PyList_Check(chroms)) return rv;
        if(!PyList_Check(starts)) return rv;
        if(!PyList_Check(ends)) return rv;
        if(!PyList_Check(values)) return rv;
        sz = PyList_Size(chroms);

        if(sz != PyList_Size(starts)) return rv;
        if(sz != PyList_Size(ends)) return rv;
        if(sz != PyList_Size(values)) return rv;

        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(chroms, i);
            if(!PyString_Check(tmp)) return rv;
            tmp = PyList_GetItem(starts, i);
            if(!isNumeric(tmp)) return rv;
            tmp = PyList_GetItem(ends, i);
            if(!isNumeric(tmp)) return rv;
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
        rv = 1;
    }
    return rv;
}

//single chrom, multiple starts, single span
int isType1(PyObject *chroms, PyObject *starts, PyObject *values, PyObject *span) {
    int rv = 0;
    Py_ssize_t i, sz;
    PyObject *tmp;

    if(span) {
        if(!PyString_Check(chroms)) return rv;
        if(!PyList_Check(starts)) return rv;
        if(!PyList_Check(values)) return rv;
        if(!isNumeric(span)) return rv;
        sz = PyList_Size(starts);

        if(sz != PyList_Size(values)) return rv;

        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(starts, i);
            if(!isNumeric(tmp)) return rv;
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
        rv = 1;
    }
    return rv;
}

//Single chrom, single start, single span, single step, multiple values
int isType2(PyObject *chroms, PyObject *starts, PyObject *values, PyObject *span, PyObject *step) {
    int rv = 0;
    Py_ssize_t i, sz;
    PyObject *tmp;

    if(!isNumeric(span)) return rv;
    if(!isNumeric(step)) return rv;
    if(!PyString_Check(chroms)) return rv;
    if(!isNumeric(starts)) return rv;

    sz = PyList_Size(values);
    for(i=0; i<sz; i++) {
        tmp = PyList_GetItem(values, i);
        if(!PyFloat_Check(tmp)) return rv;
    }
    rv = 1;
    return rv;
}

int getType(PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *values, PyObject *span, PyObject *step) {
    if(!chroms) return -1;
    if(!starts) return -1;
    if(!values) return -1;
    if(chroms && starts && ends && values && isType0(chroms, starts, ends, values)) return 0;
    if(chroms && starts && span && values && isType1(chroms, starts, values, span)) return 1;
    if(chroms && starts && values && span && step && isType2(chroms, starts, values, span, step)) return 2;
    return -1;
}

//1: Can use a bwAppend* function. 0: must use a bwAdd* function
int canAppend(pyBigWigFile_t *self, int desiredType, PyObject *chroms, PyObject *starts, PyObject *span, PyObject *step) {
    bigWigFile_t *bw = self->bw;
    Py_ssize_t i, sz;
    uint32_t tid, uspan, ustep, ustart;
    PyObject *tmp;

    if(self->lastType == -1) return 0;
    if(self->lastTid == -1) return 0;
    if(self->lastType != desiredType) return 0;

    //We can only append if (A) we have the same type or (B) the same chromosome (and compatible span/step/starts
    if(desiredType == 0) {
        //We need (A) chrom == lastTid and (B) all chroms to be the same
        sz = PyList_Size(chroms);
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(chroms, i);
            tid = bwGetTid(bw, PyString_AsString(tmp));
            if(tid != (uint32_t) self->lastTid) return 0;
        }
        ustart = Numeric2Uint(PyList_GetItem(starts, 0));
        if(PyErr_Occurred()) return 0;
        if(ustart < self->lastStart) return 0;
        return 1;
    } else if(desiredType == 1) {
        //We need (A) chrom == lastTid, (B) all chroms to be the same, and (C) equal spans
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan != self->lastSpan) return 0;
        if(!PyString_Check(chroms)) return 0;
        tid = bwGetTid(bw, PyString_AsString(chroms));
        if(tid != (uint32_t) self->lastTid) return 0;

        ustart = Numeric2Uint(PyList_GetItem(starts, 0));
        if(PyErr_Occurred()) return 0;
        if(ustart < self->lastStart) return 0;
        return 1;
    } else if(desiredType == 2) {
        //We need (A) chrom == lastTid, (B) span/step to be equal and (C) compatible starts
        tid = bwGetTid(bw, PyString_AsString(chroms));
        if(tid != (uint32_t) self->lastTid) return 0;
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan != self->lastSpan) return 0;
        ustep = Numeric2Uint(step);
        if(PyErr_Occurred()) return 0;
        if(ustep != self->lastStep) return 0;

        //But is the start position compatible?
        ustart = Numeric2Uint(starts);
        if(PyErr_Occurred()) return 0;
        if(ustart != self->lastStart) return 0;
        return 1;
    }

    return 0;
}

//Returns 0 on success, 1 on error. Sets self->lastTid && self->lastStart (unless there was an error)
int PyAddIntervals(pyBigWigFile_t *self, PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *values) {
    bigWigFile_t *bw = self->bw;
    Py_ssize_t i, sz;
    char **cchroms = NULL;
    uint32_t n, *ustarts = NULL, *uends = NULL;
    float *fvalues = NULL;
    int rv;

    sz = PyList_Size(starts);
    n = (uint32_t) sz;

    //Allocate space
    cchroms = calloc(n, sizeof(char*));
    ustarts = calloc(n, sizeof(uint32_t));
    uends = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!cchroms || !ustarts || !uends || !fvalues) goto error;

    for(i=0; i<sz; i++) {
        cchroms[i] = PyString_AsString(PyList_GetItem(chroms, i));
        ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
        uends[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(ends, i));
        fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
    }

    rv = bwAddIntervals(bw, cchroms, ustarts, uends, fvalues, n);
    if(!rv) {
        self->lastTid = bwGetTid(bw, cchroms[n-1]);
        self->lastStart = uends[n-1];
    }
    free(cchroms);
    free(ustarts);
    free(uends);
    free(fvalues);
    return rv;

error:
    if(cchroms) free(cchroms);
    if(ustarts) free(ustarts);
    if(uends) free(uends);
    if(fvalues) free(fvalues);
    return 1;
}

//Returns 0 on success, 1 on error. Update self->lastStart
int PyAppendIntervals(pyBigWigFile_t *self, PyObject *starts, PyObject *ends, PyObject *values) {
    bigWigFile_t *bw = self->bw;
    Py_ssize_t i, sz;
    uint32_t n, *ustarts = NULL, *uends = NULL;
    float *fvalues = NULL;
    int rv;

    sz = PyList_Size(starts);
    n = (uint32_t) sz;

    //Allocate space
    ustarts = calloc(n, sizeof(uint32_t));
    uends = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!ustarts || !uends || !fvalues) goto error;

    for(i=0; i<sz; i++) {
        ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
        uends[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(ends, i));
        fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
    }
    rv = bwAppendIntervals(bw, ustarts, uends, fvalues, n);
    if(rv) self->lastStart = uends[n-1];
    free(ustarts);
    free(uends);
    free(fvalues);
    return rv;

error:
    if(ustarts) free(ustarts);
    if(uends) free(uends);
    if(fvalues) free(fvalues);
    return 1;
}

//Returns 0 on success, 1 on error. Sets self->lastTid/lastStart/lastSpan (unless there was an error)
int PyAddIntervalSpans(pyBigWigFile_t *self, PyObject *chroms, PyObject *starts, PyObject *values, PyObject *span) {
    bigWigFile_t *bw = self->bw;
    Py_ssize_t i, sz;
    char *cchroms = NULL;
    uint32_t n, *ustarts = NULL, uspan;
    float *fvalues = NULL;
    int rv;

    sz = PyList_Size(starts);
    n = (uint32_t) sz;

    //Allocate space
    ustarts = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!ustarts || !fvalues) goto error;
    uspan = (uint32_t) PyLong_AsLong(span);
    cchroms = PyString_AsString(chroms);

    for(i=0; i<sz; i++) {
        ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
        fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
    }

    rv = bwAddIntervalSpans(bw, cchroms, ustarts, uspan, fvalues, n);
    if(!rv) {
        self->lastTid = bwGetTid(bw, cchroms);
        self->lastSpan = uspan;
        self->lastStart = ustarts[n-1]+uspan;
    }
    free(ustarts);
    free(fvalues);
    return rv;

error:
    if(ustarts) free(ustarts);
    if(fvalues) free(fvalues);
    return 1;
}

//Returns 0 on success, 1 on error. Updates self->lastStart
int PyAppendIntervalSpans(pyBigWigFile_t *self, PyObject *starts, PyObject *values) {
    bigWigFile_t *bw = self->bw;
    Py_ssize_t i, sz;
    uint32_t n, *ustarts = NULL;
    float *fvalues = NULL;
    int rv;

    sz = PyList_Size(starts);
    n = (uint32_t) sz;

    //Allocate space
    ustarts = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!ustarts || !fvalues) goto error;

    for(i=0; i<sz; i++) {
        ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
        fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
    }

    rv = bwAppendIntervalSpans(bw, ustarts, fvalues, n);
    if(rv) self->lastStart = ustarts[n-1] + self->lastSpan;
    free(ustarts);
    free(fvalues);
    return rv;

error:
    if(ustarts) free(ustarts);
    if(fvalues) free(fvalues);
    return 1;
}

//Returns 0 on success, 1 on error. Sets self->lastTid/self->lastSpan/lastStep/lastStart (unless there was an error)
int PyAddIntervalSpanSteps(pyBigWigFile_t *self, PyObject *chroms, PyObject *starts, PyObject *values, PyObject *span, PyObject *step) {
    bigWigFile_t *bw = self->bw;
    Py_ssize_t i, sz;
    char *cchrom = NULL;
    uint32_t n, ustarts, uspan, ustep;
    float *fvalues = NULL;
    int rv;

    sz = PyList_Size(values);
    n = (uint32_t) sz;

    //Allocate space
    fvalues = calloc(n, sizeof(float));
    if(!fvalues) goto error;
    uspan = (uint32_t) PyLong_AsLong(span);
    ustep = (uint32_t) PyLong_AsLong(step);
    ustarts = (uint32_t) PyLong_AsLong(starts);
    cchrom = PyString_AsString(chroms);

    for(i=0; i<sz; i++) fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));

    rv = bwAddIntervalSpanSteps(bw, cchrom, ustarts, uspan, ustep, fvalues, n);
    if(!rv) {
        self->lastTid = bwGetTid(bw, cchrom);
        self->lastSpan = uspan;
        self->lastStep = ustep;
        self->lastStart = ustarts + ustep*n;
    }
    free(fvalues);
    return rv;

error:
    if(fvalues) free(fvalues);
    return 1;
}

//Returns 0 on success, 1 on error. Sets self->lastStart
int PyAppendIntervalSpanSteps(pyBigWigFile_t *self, PyObject *values) {
    bigWigFile_t *bw = self->bw;
    Py_ssize_t i, sz;
    uint32_t n;
    float *fvalues = NULL;
    int rv;

    sz = PyList_Size(values);
    n = (uint32_t) sz;

    //Allocate space
    fvalues = calloc(n, sizeof(float));
    if(!fvalues) goto error;

    for(i=0; i<sz; i++) fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));

    rv = bwAppendIntervalSpanSteps(bw, fvalues, n);
    if(!rv) self->lastStart += self->lastStep * n;
    free(fvalues);
    return rv;

error:
    if(fvalues) free(fvalues);
    return 1;
}

//Checks and ensures that (A) the entries are sorted correctly and don't overlap and (B) that the come after things that have already been added.
//Returns 1 on correct input, 0 on incorrect input
int addEntriesInputOK(pyBigWigFile_t *self, PyObject *chroms, PyObject *starts, PyObject *ends, PyObject *span, PyObject *step, int type) {
    uint32_t lastTid = self->lastTid;
    uint32_t lastEnd = self->lastStart;
    uint32_t cTid, ustart, uend, uspan, ustep;
    Py_ssize_t i, sz;
    PyObject *chrom;

    if(type == 0) {
        //Each chrom:start-end needs to be properly formed and come after prior entries
        sz = PyList_Size(starts);
        if(sz == 0) return 0;
        for(i=0; i<sz; i++) {
            chrom = PyList_GetItem(chroms, i);
            cTid = bwGetTid(self->bw, PyString_AsString(chrom));
            if(cTid == (uint32_t) -1) return 0;
            ustart = Numeric2Uint(PyList_GetItem(starts, i));
            if(PyErr_Occurred()) return 0;
            uend = Numeric2Uint(PyList_GetItem(ends, i));
            if(PyErr_Occurred()) return 0;

            if(ustart >= uend) return 0;
            if(lastTid != (uint32_t) -1) {
                if(lastTid > cTid) return 0;
                if(lastTid == cTid) {
                    if(ustart < lastEnd) return 0;
                }
            }
            lastTid = cTid;
            lastEnd = uend;
        }
        return 1;
    } else if(type == 1) {
        //each chrom:start-(start+span) needs to be properly formed and come after prior entries
        if(!PyList_Check(starts)) return 0;
        sz = PyList_Size(starts);
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan < 1) return 0;
        if(sz == 0) return 0;
        cTid = bwGetTid(self->bw, PyString_AsString(chroms));
        if(cTid == (uint32_t) -1) return 0;
        if(lastTid != (uint32_t) -1) {
            if(lastTid > cTid) return 0;
        }
        for(i=0; i<sz; i++) {
            ustart = Numeric2Uint(PyList_GetItem(starts, i));
            if(PyErr_Occurred()) return 0;
            uend = ustart + uspan;

            if(lastTid == cTid) {
                if(ustart < lastEnd) return 0;
            }
            lastTid = cTid;
            lastEnd = uend;
        }
        return 1;
    } else if(type == 2) {
        //The chrom and start need to be appropriate
        cTid = bwGetTid(self->bw, PyString_AsString(chroms));
        if(cTid == (uint32_t) -1) return 0;
        ustart = Numeric2Uint(starts);
        if(PyErr_Occurred()) return 0;
        uspan = Numeric2Uint(span);
        if(PyErr_Occurred()) return 0;
        if(uspan < 1) return 0;
        ustep = Numeric2Uint(step);
        if(PyErr_Occurred()) return 0;
        if(ustep < 1) return 0;
        if(lastTid != (uint32_t) -1) {
            if(lastTid > cTid) return 0;
            if(lastTid == cTid) {
                if(ustart < lastEnd) return 0;
            }
        }
        return 1;
    }
    return 0;
}

PyObject *pyBwAddEntries(pyBigWigFile_t *self, PyObject *args, PyObject *kwds) {
    static char *kwd_list[] = {"chroms", "starts", "ends", "values", "span", "step", "validate", NULL};
    PyObject *chroms = NULL, *starts = NULL, *ends = NULL, *values = NULL, *span = NULL, *step = NULL;
    PyObject *validate = Py_True;
    int desiredType;

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOOOO", kwd_list, &chroms, &starts, &ends, &values, &span, &step, &validate)) {
        PyErr_SetString(PyExc_RuntimeError, "Illegal arguments");
        return NULL;
    }

    desiredType = getType(chroms, starts, ends, values, span, step);
    if(desiredType == -1) {
        PyErr_SetString(PyExc_RuntimeError, "You must provide a valid set of entries. These can be comprised of any of the following: \n"
"1. A list of each of chromosomes, start positions, end positions and values.\n"
"2. A list of each of start positions and values. Also, a chromosome and span must be specified.\n"
"3. A list values, in which case a single chromosome, start position, span and step must be specified.\n");
        return NULL;
    }

    if(validate == Py_True  && !addEntriesInputOK(self, chroms, starts, ends, span, step, desiredType)) {
        PyErr_SetString(PyExc_RuntimeError, "The entries you tried to add are out of order, precede already added entries, or otherwise use illegal values.\n"
" Please correct this and try again.\n");
        return NULL;
    }

    if(canAppend(self, desiredType, chroms, starts, span, step)) {
        switch(desiredType) {
            case 0:
                if(PyAppendIntervals(self, starts, ends, values)) goto error;
                break;
            case 1:
                if(PyAppendIntervalSpans(self, starts, values)) goto error;
                break;
            case 2:
                if(PyAppendIntervalSpanSteps(self, values)) goto error;
                break;
        }
    } else {
        switch(desiredType) {
            case 0:
                if(PyAddIntervals(self, chroms, starts, ends, values)) goto error;
                break;
            case 1:
                if(PyAddIntervalSpans(self, chroms, starts, values, span)) goto error;
                break;
            case 2:
                if(PyAddIntervalSpanSteps(self, chroms, starts, values, span, step)) goto error;
                break;
        }
    }
    self->lastType = desiredType;

    Py_INCREF(Py_None);
    return Py_None;

error:
    PyErr_SetString(PyExc_RuntimeError, "Received an error while adding the intervals.");
    return NULL;
}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pyBigWig(void) {
    PyObject *res;
    errno = 0; //just in case

    if(Py_AtExit(bwCleanup)) return NULL;

    if(PyType_Ready(&bigWigFile) < 0) return NULL;
    if(Py_AtExit(bwCleanup)) return NULL;
    if(bwInit(128000)) return NULL;
    res = PyModule_Create(&pyBigWigmodule);
    if(!res) return NULL;

    Py_INCREF(&bigWigFile);
    PyModule_AddObject(res, "pyBigWig", (PyObject *) &bigWigFile);

    return res;
}
#else
//Python2 initialization
PyMODINIT_FUNC initpyBigWig(void) {
    errno=0; //Sometimes libpython2.7.so is missing some links...
    if(Py_AtExit(bwCleanup)) return;
    if(PyType_Ready(&bigWigFile) < 0) return;
    if(bwInit(128000)) return; //This is temporary
    Py_InitModule3("pyBigWig", bwMethods, "A module for handling bigWig files");
}
#endif
