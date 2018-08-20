#include <Python.h>
#include <inttypes.h>
#include "pyBigWig.h"

#ifdef WITHNUMPY
#include <float.h>
#include "numpy/npy_common.h"
#include "numpy/halffloat.h"
#include "numpy/ndarrayobject.h"
#include "numpy/arrayscalars.h"

int lsize = NPY_SIZEOF_LONG;

//Raises an exception on error, which should be checked
uint32_t getNumpyU32(PyArrayObject *obj, Py_ssize_t i) {
    int dtype;
    char *p;
    uint32_t o = 0;
    npy_intp stride;

    //Get the dtype
    dtype = PyArray_TYPE(obj);
    //Get the stride
    stride = PyArray_STRIDE(obj, 0);
    p = PyArray_BYTES(obj) + i*stride;

    switch(dtype) {
    case NPY_INT8:
        if(((int8_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int8_t *) p)[0];
        break;
    case NPY_INT16:
        if(((int16_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int16_t *) p)[0];
        break;
    case NPY_INT32:
        if(((int32_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int32_t *) p)[0];
        break;
    case NPY_INT64:
        if(((int64_t *) p)[0] < 0) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer < 0!\n");
            goto error;
        }
        o += ((int64_t *) p)[0];
        break;
    case NPY_UINT8:
        o += ((uint8_t *) p)[0];
        break;
    case NPY_UINT16:
        o += ((uint16_t *) p)[0];
        break;
    case NPY_UINT32:
        o += ((uint32_t *) p)[0];
        break;
    case NPY_UINT64:
        if(((uint64_t *) p)[0] > (uint32_t) -1) {
            PyErr_SetString(PyExc_RuntimeError, "Received an integer larger than possible for a 32bit unsigned integer!\n");
            goto error;
        }
        o += ((uint64_t *) p)[0];
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "Received unknown data type for conversion to uint32_t!\n");
        goto error;
        break;
    }
    return o;

error:
    return 0;
};

long getNumpyL(PyObject *obj) {
    short s;
    int i;
    long l;
    long long ll;
    unsigned short us;
    unsigned int ui;
    unsigned long ul;
    unsigned long long ull;
    
    if(!PyArray_IsIntegerScalar(obj)) {
        PyErr_SetString(PyExc_RuntimeError, "Received non-Integer scalar type for conversion to long!\n");
        return 0;
    }

    if(PyArray_IsScalar(obj, Short)) {
        s = ((PyShortScalarObject *)obj)->obval;
        l = s;
    } else if(PyArray_IsScalar(obj, Int)) {
        i = ((PyLongScalarObject *)obj)->obval;
        l = i;
    } else if(PyArray_IsScalar(obj, Long)) {
        l = ((PyLongScalarObject *)obj)->obval;
    } else if(PyArray_IsScalar(obj, LongLong)) {
        ll = ((PyLongScalarObject *)obj)->obval;
        l = ll;
    } else if(PyArray_IsScalar(obj, UShort)) {
        us = ((PyLongScalarObject *)obj)->obval;
        l = us;
    } else if(PyArray_IsScalar(obj, UInt)) {
        ui = ((PyLongScalarObject *)obj)->obval;
        l = ui;
    } else if(PyArray_IsScalar(obj, ULong)) {
        ul = ((PyLongScalarObject *)obj)->obval;
        l = ul;
    } else if(PyArray_IsScalar(obj, ULongLong)) {
        ull = ((PyLongScalarObject *)obj)->obval;
        l = ull;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "Received unknown scalar type for conversion to long!\n");
        return 0;
    }

    return l;
}

//Raises an exception on error, which should be checked
float getNumpyF(PyArrayObject *obj, Py_ssize_t i) {
    int dtype;
    char *p;
    float o = 0.0;
    npy_intp stride;

    //Get the dtype
    dtype = PyArray_TYPE(obj);
    //Get the stride
    stride = PyArray_STRIDE(obj, 0);
    p = PyArray_BYTES(obj) + i*stride;

    switch(dtype) {
    case NPY_FLOAT16:
        return npy_half_to_float(((npy_half*)p)[0]);
    case NPY_FLOAT32:
        return ((float*)p)[0];
    case NPY_FLOAT64:
        if(((double*)p)[0] > FLT_MAX) {
            PyErr_SetString(PyExc_RuntimeError, "Received a floating point value greater than possible for a 32-bit float!\n");
            goto error;
        }
        if(((double*)p)[0] < -FLT_MAX) {
            PyErr_SetString(PyExc_RuntimeError, "Received a floating point value less than possible for a 32-bit float!\n");
            goto error;
        }
        o += ((double*)p)[0];
        return o;
    default:
        PyErr_SetString(PyExc_RuntimeError, "Received unknown data type for conversion to float!\n");
        goto error;
        break;
    }
    return o;

error:
    return 0;
}

//The calling function needs to free the result
char *getNumpyStr(PyArrayObject *obj, Py_ssize_t i) {
    char *p , *o = NULL;
    npy_intp stride, j;
    int dtype;

    //Get the dtype
    dtype = PyArray_TYPE(obj);
    //Get the stride
    stride = PyArray_STRIDE(obj, 0);
    p = PyArray_BYTES(obj) + i*stride;

    switch(dtype) {
    case NPY_STRING:
        o = calloc(1, stride + 1);
        strncpy(o, p, stride);
        return o;
    case NPY_UNICODE:
        o = calloc(1, stride/4 + 1);
        for(j=0; j<stride/4; j++) o[j] = (char) ((uint32_t*)p)[j];
        return o;
    default:
        PyErr_SetString(PyExc_RuntimeError, "Received unknown data type!\n");
        break;
    }
    return NULL;
}
#endif

//Return 1 if there are any entries at all
int hasEntries(bigWigFile_t *bw) {
    if(bw->hdr->nBasesCovered > 0) return 1;
    return 0;
}

PyObject* pyBwEnter(pyBigWigFile_t*self, PyObject *args) {
    bigWigFile_t *bw = self->bw;

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not opened!");
        return NULL;
    }

    Py_INCREF(self);

    return (PyObject*) self;
}

PyObject* pyBwOpen(PyObject *self, PyObject *pyFname) {
    char *fname = NULL;
    char *mode = "r";
    pyBigWigFile_t *pybw;
    bigWigFile_t *bw = NULL;

    if(!PyArg_ParseTuple(pyFname, "s|s", &fname, &mode)) goto error;

    //Open the local/remote file
    if(strchr(mode, 'w') != NULL || bwIsBigWig(fname, NULL)) {
        bw = bwOpen(fname, NULL, mode);
    } else {
        bw = bbOpen(fname, NULL);
    }
    if(!bw) {
        fprintf(stderr, "[pyBwOpen] bw is NULL!\n");
        goto error;
    }
    if(!mode || !strchr(mode, 'w')) {
        if(!bw->cl) goto error;
    }

    pybw = PyObject_New(pyBigWigFile_t, &bigWigFile);
    if(!pybw) {
        fprintf(stderr, "[pyBwOpen] PyObject_New() returned NULL (out of memory?)!\n");
        goto error;
    }
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

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not opened!");
        return NULL;
    }

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

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not opened!");
        return NULL;
    }

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
    if(strcmp(s, "std") == 0) return stdev;
    if(strcmp(s, "dev") == 0) return dev;
    if(strcmp(s, "max") == 0) return max;
    if(strcmp(s, "min") == 0) return min;
    if(strcmp(s, "cov") == 0) return cov;
    if(strcmp(s, "coverage") == 0) return cov;
    if(strcmp(s, "sum") == 0) return sum;
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
    PyObject *ret, *exact = Py_False, *starto = NULL, *endo = NULL;
    int i, nBins = 1;
    errno = 0; //In the off-chance that something elsewhere got an error and didn't clear it...

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not open!");
        return NULL;
    }

    if(bw->type == 1) {
        PyErr_SetString(PyExc_RuntimeError, "bigBed files have no statistics!");
        return NULL;
    }

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|OOsiO", kwd_list, &chrom, &starto, &endo, &type, &nBins, &exact)) {
        PyErr_SetString(PyExc_RuntimeError, "You must supply at least a chromosome!");
        return NULL;
    }

    //Check inputs, reset to defaults if nothing was input
    if(!nBins) nBins = 1; //For some reason, not specifying this overrides the default!
    if(!type) type = "mean";
    tid = bwGetTid(bw, chrom);

    if(starto) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(starto, Integer)) {
            startl = (long) getNumpyL(starto);
        } else 
#endif
        if(PyLong_Check(starto)) {
            startl = PyLong_AsLong(starto);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(starto)) {
            startl = PyInt_AsLong(starto);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The start coordinate must be a number!");
            return NULL;
        }
    }

    if(endo) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(endo, Integer)) {
            endl = (long) getNumpyL(endo);
        } else 
#endif
        if(PyLong_Check(endo)) {
            endl = PyLong_AsLong(endo);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(endo)) {
            endl = PyInt_AsLong(endo);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The end coordinate must be a number!");
            return NULL;
        }
    }

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

    //Return a list of None if there are no entries at all
    if(!hasEntries(bw)) {
        ret = PyList_New(nBins);
        for(i=0; i<nBins; i++) {
            Py_INCREF(Py_None);
            PyList_SetItem(ret, i, Py_None);
        }
        return ret;
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
#ifdef WITHNUMPY
static PyObject *pyBwGetValues(pyBigWigFile_t *self, PyObject *args, PyObject *kwds) {
#else
static PyObject *pyBwGetValues(pyBigWigFile_t *self, PyObject *args) {
#endif
    bigWigFile_t *bw = self->bw;
    int i;
    uint32_t start, end = -1, tid;
    unsigned long startl, endl;
    char *chrom;
    PyObject *ret, *starto = NULL, *endo = NULL;
    bwOverlappingIntervals_t *o;

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not open!");
        return NULL;
    }

    if(bw->type == 1) {
        PyErr_SetString(PyExc_RuntimeError, "bigBed files have no values! Use 'entries' instead.");
        return NULL;
    }

#ifdef WITHNUMPY
    static char *kwd_list[] = {"chrom", "start", "end", "numpy", NULL};
    PyObject *outputNumpy = Py_False;

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "sOO|O", kwd_list, &chrom, &starto, &endo, &outputNumpy)) {
#else
    if(!PyArg_ParseTuple(args, "sOO", &chrom, &starto, &endo)) {
#endif
        PyErr_SetString(PyExc_RuntimeError, "You must supply a chromosome, start and end position.\n");
        return NULL;
    }

    tid = bwGetTid(bw, chrom);

#ifdef WITHNUMPY
    if(PyArray_IsScalar(starto, Integer)) {
        startl = (long) getNumpyL(starto);
    } else
#endif
    if(PyLong_Check(starto)) {
        startl = PyLong_AsLong(starto);
#if PY_MAJOR_VERSION < 3
    } else if(PyInt_Check(starto)) {
        startl = PyInt_AsLong(starto);
#endif
    } else {
        PyErr_SetString(PyExc_RuntimeError, "The start coordinate must be a number!");
        return NULL;
    }

#ifdef WITHNUMPY
    if(PyArray_IsScalar(endo, Integer)) {
        endl = (long) getNumpyL(endo);
    } else
#endif
    if(PyLong_Check(endo)) {
        endl = PyLong_AsLong(endo);
#if PY_MAJOR_VERSION < 3
    } else if(PyInt_Check(endo)) {
        endl = PyInt_AsLong(endo);
#endif
    } else {
        PyErr_SetString(PyExc_RuntimeError, "The end coordinate must be a number!");
        return NULL;
    }

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

    if(!hasEntries(self->bw)) {
#ifdef WITHNUMPY
        if(outputNumpy == Py_True) {
            return PyArray_SimpleNew(0, NULL, NPY_FLOAT);
        } else {
#endif
            return PyList_New(0);
#ifdef WITHNUMPY
        }
#endif
    }

    o = bwGetValues(self->bw, chrom, start, end, 1);
    if(!o) {
        PyErr_SetString(PyExc_RuntimeError, "An error occurred while fetching values!");
        return NULL;
    }

#ifdef WITHNUMPY
    if(outputNumpy == Py_True) {
        npy_intp len = end - start;
        ret = PyArray_SimpleNewFromData(1, &len, NPY_FLOAT, (void *) o->value);
        //This will break if numpy ever stops using malloc!
        PyArray_ENABLEFLAGS((PyArrayObject*) ret, NPY_ARRAY_OWNDATA);
        free(o->start);
        free(o->end);
        free(o);
    } else {
#endif
        ret = PyList_New(end-start);
        for(i=0; i<(int) o->l; i++) PyList_SetItem(ret, i, PyFloat_FromDouble(o->value[i]));
        bwDestroyOverlappingIntervals(o);
#ifdef WITHNUMPY
    }
#endif

    return ret;
}

static PyObject *pyBwGetIntervals(pyBigWigFile_t *self, PyObject *args, PyObject *kwds) {
    bigWigFile_t *bw = self->bw;
    uint32_t start, end = -1, tid, i;
    unsigned long startl = 0, endl = -1;
    static char *kwd_list[] = {"chrom", "start", "end", NULL};
    bwOverlappingIntervals_t *intervals = NULL;
    char *chrom;
    PyObject *ret, *starto = NULL, *endo = NULL;

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not opened!");
        return NULL;
    }

    if(bw->type == 1) {
        PyErr_SetString(PyExc_RuntimeError, "bigBed files have no intervals! Use 'entries()' instead.");
        return NULL;
    }

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "s|OO", kwd_list, &chrom, &starto, &endo)) {
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

    if(starto) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(starto, Integer)) {
            startl = (long) getNumpyL(starto);
        } else
#endif
        if(PyLong_Check(starto)) {
            startl = PyLong_AsLong(starto);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(starto)) {
            startl = PyInt_AsLong(starto);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The start coordinate must be a number!");
            return NULL;
        }
    }

    if(endo) {
#ifdef WITHNUMPY
        if(PyArray_IsScalar(endo, Integer)) {
            endl = (long) getNumpyL(endo);
        } else
#endif
        if(PyLong_Check(endo)) {
            endl = PyLong_AsLong(endo);
#if PY_MAJOR_VERSION < 3
        } else if(PyInt_Check(endo)) {
            endl = PyInt_AsLong(endo);
#endif
        } else {
            PyErr_SetString(PyExc_RuntimeError, "The end coordinate must be a number!");
            return NULL;
        }
    }

    start = (uint32_t) startl;
    end = (uint32_t) endl;
    if(end <= start || end > bw->cl->len[tid] || start >= end) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid interval bounds!");
        return NULL;
    }

    //Check for empty files
    if(!hasEntries(bw)) {
        Py_INCREF(Py_None);
        return Py_None;
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
#ifdef WITHNUMPY
    if(PyArray_IsScalar(obj, Integer)) return 1;
#endif
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

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not open!");
        return NULL;
    }

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

    if(lengths) free(lengths);
    if(chroms) free(chroms);

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
    Py_ssize_t i, sz = 0;
    PyObject *tmp;

    if(!PyList_Check(chroms)
#ifdef WITHNUMPY
        && !PyArray_Check(chroms)
#endif
        ) return rv;
    if(!PyList_Check(starts)
#ifdef WITHNUMPY
        && !PyArray_Check(starts)
#endif
        ) return rv;
    if(!PyList_Check(ends)
#ifdef WITHNUMPY
        && !PyArray_Check(ends)
#endif
        ) return rv;
    if(!PyList_Check(values)
#ifdef WITHNUMPY
        && !PyArray_Check(values)
#endif
        ) return rv;
    if(PyList_Check(chroms)) sz = PyList_Size(chroms);
#ifdef WITHNUMPY
    if(PyArray_Check(chroms)) sz += PyArray_Size(chroms);
#endif

    if(PyList_Check(starts)) {
        if(sz != PyList_Size(starts)) return rv;
#ifdef WITHNUMPY
    } else {
        if(sz != PyArray_Size(starts)) return rv;
#endif
    }
    if(PyList_Check(ends)) {
        if(sz != PyList_Size(ends)) return rv;
#ifdef WITHNUMPY
    } else {
        if(sz != PyArray_Size(ends)) return rv;
#endif
    }
    if(PyList_Check(values)) {
        if(sz != PyList_Size(values)) return rv;
#ifdef WITHNUMPY
    } else {
        if(sz != PyArray_Size(values)) return rv;
#endif
    }

    //Ensure chroms contains strings, etc.
    if(PyList_Check(chroms)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(chroms, i);
            if(!PyString_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISSTRING( (PyArrayObject*) chroms)) return rv;
#endif
    }
    if(PyList_Check(starts)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(starts, i);
            if(!isNumeric(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISINTEGER( (PyArrayObject*) starts)) return rv;
#endif
    }
    if(PyList_Check(ends)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(ends, i);
            if(!isNumeric(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISINTEGER( (PyArrayObject*) ends)) return rv;
#endif
    }
    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISFLOAT((PyArrayObject*) values)) return rv;
#endif
    }
    return 1;
}

//single chrom, multiple starts, single span
int isType1(PyObject *chroms, PyObject *starts, PyObject *values, PyObject *span) {
    int rv = 0;
    Py_ssize_t i, sz = 0;
    PyObject *tmp;

    if(!PyString_Check(chroms)) return rv;
    if(!PyList_Check(starts)
#ifdef WITHNUMPY
        && !PyArray_Check(starts)
#endif
        ) return rv;
    if(!PyList_Check(values)
#ifdef WITHNUMPY
        && !PyArray_Check(values)
#endif
        ) return rv;
    if(!isNumeric(span)) return rv;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif

    if(PyList_Check(values)) if(sz != PyList_Size(values)) return rv;
#ifdef WITHNUMPY
    if(PyArray_Check(values)) if(sz != PyArray_Size(values)) return rv;
#endif

    if(PyList_Check(starts)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(starts, i);
            if(!isNumeric(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISINTEGER( (PyArrayObject*) starts)) return rv;
#endif
    }
    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISFLOAT( (PyArrayObject*) values)) return rv;
#endif
    }
    return 1;
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

    if(PyList_Check(values)) {
        sz = PyList_Size(values);
        for(i=0; i<sz; i++) {
            tmp = PyList_GetItem(values, i);
            if(!PyFloat_Check(tmp)) return rv;
        }
#ifdef WITHNUMPY
    } else {
        if(!PyArray_ISFLOAT( (PyArrayObject*) values)) return rv;
#endif
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
    Py_ssize_t i, sz = 0;
    uint32_t tid, uspan, ustep, ustart;
    PyObject *tmp;
#ifdef WITHNUMPY
    char *chrom;
#endif

    if(self->lastType == -1) return 0;
    if(self->lastTid == -1) return 0;
    if(self->lastType != desiredType) return 0;

    //We can only append if (A) we have the same type or (B) the same chromosome (and compatible span/step/starts
    if(desiredType == 0) {
        //We need (A) chrom == lastTid and (B) all chroms to be the same
        if(PyList_Check(chroms)) sz = PyList_Size(chroms);
#ifdef WITHNUMPY
        if(PyArray_Check(chroms)) sz = PyArray_Size(chroms);
#endif

        for(i=0; i<sz; i++) {
#ifdef WITHNUMPY
            if(PyArray_Check(chroms)) {
                chrom = getNumpyStr((PyArrayObject*)chroms, i);
                tid = bwGetTid(bw, chrom);
                free(chrom);
            } else {
#endif
                tmp = PyList_GetItem(chroms, i);
                tid = bwGetTid(bw, PyString_AsString(tmp));
#ifdef WITHNUMPY
            }
#endif
            if(tid != (uint32_t) self->lastTid) return 0;
        }

#ifdef WITHNUMPY
        if(PyArray_Check(starts)) {
            ustart = getNumpyU32((PyArrayObject*)starts, 0);
        } else {
#endif
            ustart = Numeric2Uint(PyList_GetItem(starts, 0));
#ifdef WITHNUMPY
        }
#endif
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

#ifdef WITHNUMPY
        if(PyList_Check(starts)) ustart = Numeric2Uint(PyList_GetItem(starts, 0));
        else ustart = getNumpyU32((PyArrayObject*) starts, 0);
#else
        ustart = Numeric2Uint(PyList_GetItem(starts, 0));
#endif
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
    Py_ssize_t i, sz = 0;
    char **cchroms = NULL;
    uint32_t n, *ustarts = NULL, *uends = NULL;
    float *fvalues = NULL;
    int rv;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
    n = (uint32_t) sz;

    //Allocate space
    cchroms = calloc(n, sizeof(char*));
    ustarts = calloc(n, sizeof(uint32_t));
    uends = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!cchroms || !ustarts || !uends || !fvalues) goto error;

    for(i=0; i<sz; i++) {
        if(PyList_Check(chroms)) {
            cchroms[i] = PyString_AsString(PyList_GetItem(chroms, i));
#ifdef WITHNUMPY
        } else {
            cchroms[i] = getNumpyStr((PyArrayObject*)chroms, i);
#endif
        }
        if(PyList_Check(starts)) {
            ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
        } else {
            ustarts[i] = getNumpyU32((PyArrayObject*)starts, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
        if(PyList_Check(ends)) {
            uends[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(ends, i));
#ifdef WITHNUMPY
        } else {
            uends[i] = getNumpyU32((PyArrayObject*)ends, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
        if(PyList_Check(values)) {
            fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
#ifdef WITHNUMPY
        } else {
            fvalues[i] = getNumpyF((PyArrayObject*)values, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
    }

    rv = bwAddIntervals(bw, cchroms, ustarts, uends, fvalues, n);
    if(!rv) {
        self->lastTid = bwGetTid(bw, cchroms[n-1]);
        self->lastStart = uends[n-1];
    }
    if(!PyList_Check(chroms)) {
        for(i=0; i<n; i++) free(cchroms[i]);
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
    Py_ssize_t i, sz = 0;
    uint32_t n, *ustarts = NULL, *uends = NULL;
    float *fvalues = NULL;
    int rv;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
    n = (uint32_t) sz;

    //Allocate space
    ustarts = calloc(n, sizeof(uint32_t));
    uends = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!ustarts || !uends || !fvalues) goto error;

    for(i=0; i<sz; i++) {
        if(PyList_Check(starts)) {
            ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
        } else {
            ustarts[i] = getNumpyU32((PyArrayObject*) starts, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
        if(PyList_Check(ends)) {
            uends[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(ends, i));
#ifdef WITHNUMPY
        } else {
            uends[i] = getNumpyU32((PyArrayObject*) ends, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
        if(PyList_Check(values)) {
            fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
#ifdef WITHNUMPY
        } else {
            fvalues[i] = getNumpyF((PyArrayObject*) values, i);
#endif
        }
        if(PyErr_Occurred()) goto error;
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
    Py_ssize_t i, sz = 0;
    char *cchroms = NULL;
    uint32_t n, *ustarts = NULL, uspan;
    float *fvalues = NULL;
    int rv;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    else if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
    n = (uint32_t) sz;

    //Allocate space
    ustarts = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!ustarts || !fvalues) goto error;
    uspan = (uint32_t) PyLong_AsLong(span);
    cchroms = PyString_AsString(chroms);

    if(PyList_Check(starts)) {
        for(i=0; i<sz; i++) {
            ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
            if(PyErr_Occurred()) goto error;
        }
#ifdef WITHNUMPY
    } else {
        for(i=0; i<sz; i++) {
            ustarts[i] = getNumpyU32((PyArrayObject*) starts, i);
            if(PyErr_Occurred()) goto error;
        }
#endif
    }
    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) {
            fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
            if(PyErr_Occurred()) goto error;
        }
#ifdef WITHNUMPY
    } else {
        for(i=0; i<sz; i++) {
            fvalues[i] = getNumpyF((PyArrayObject*) values, i);
            if(PyErr_Occurred()) goto error;
        }
#endif
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
    Py_ssize_t i, sz = 0;
    uint32_t n, *ustarts = NULL;
    float *fvalues = NULL;
    int rv;

    if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
    else if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
    n = (uint32_t) sz;

    //Allocate space
    ustarts = calloc(n, sizeof(uint32_t));
    fvalues = calloc(n, sizeof(float));
    if(!ustarts || !fvalues) goto error;

    if(PyList_Check(starts)) {
        for(i=0; i<sz; i++) {
            ustarts[i] = (uint32_t) PyLong_AsLong(PyList_GetItem(starts, i));
            if(PyErr_Occurred()) goto error;
        }
#ifdef WITHNUMPY
    } else {
        for(i=0; i<sz; i++) {
            ustarts[i] = getNumpyU32((PyArrayObject*) starts, i);
            if(PyErr_Occurred()) goto error;
        }
#endif
    }
    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) {
            fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
            if(PyErr_Occurred()) goto error;
        }
#ifdef WITHNUMPY
    } else {
        for(i=0; i<sz; i++) {
            fvalues[i] = getNumpyF((PyArrayObject*) values, i);
            if(PyErr_Occurred()) goto error;
        }
#endif
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
    Py_ssize_t i, sz = 0;
    char *cchrom = NULL;
    uint32_t n, ustarts, uspan, ustep;
    float *fvalues = NULL;
    int rv;

    if(PyList_Check(values)) sz = PyList_Size(values);
#ifdef WITHNUMPY
    else if(PyArray_Check(values)) sz += PyArray_Size(values);
#endif
    n = (uint32_t) sz;

    //Allocate space
    fvalues = calloc(n, sizeof(float));
    if(!fvalues) goto error;
    uspan = (uint32_t) PyLong_AsLong(span);
    ustep = (uint32_t) PyLong_AsLong(step);
    ustarts = (uint32_t) PyLong_AsLong(starts);
    cchrom = PyString_AsString(chroms);

    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
#ifdef WITHNUMPY
    } else {
        for(i=0; i<sz; i++) {
            fvalues[i] = getNumpyF((PyArrayObject*) values, i);
            if(PyErr_Occurred()) goto error;
        }
#endif
    }

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
    Py_ssize_t i, sz = 0;
    uint32_t n;
    float *fvalues = NULL;
    int rv;

    if(PyList_Check(values)) sz = PyList_Size(values);
#ifdef WITHNUMPY
    else if(PyArray_Check(values)) sz += PyArray_Size(values);
#endif
    n = (uint32_t) sz;

    //Allocate space
    fvalues = calloc(n, sizeof(float));
    if(!fvalues) goto error;

    if(PyList_Check(values)) {
        for(i=0; i<sz; i++) fvalues[i] = (float) PyFloat_AsDouble(PyList_GetItem(values, i));
#ifdef WITHNUMPY
    } else {
        for(i=0; i<sz; i++) {
            fvalues[i] = getNumpyF((PyArrayObject*) values, i);
            if(PyErr_Occurred()) goto error;
        }
#endif
    }

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
    Py_ssize_t i, sz = 0;
    PyObject *tmp;
#ifdef WITHNUMPY
    char *tmpStr;
#endif

    if(type == 0) {
        //Each chrom:start-end needs to be properly formed and come after prior entries
        if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
        if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
        if(sz == 0) return 0;
        for(i=0; i<sz; i++) {
#ifdef WITHNUMPY
            if(PyArray_Check(chroms)) {
                tmpStr = getNumpyStr((PyArrayObject*)chroms, i);
                cTid = bwGetTid(self->bw, tmpStr);
                free(tmpStr);
            } else {
#endif
                tmp = PyList_GetItem(chroms, i);
                cTid = bwGetTid(self->bw, PyString_AsString(tmp));
#ifdef WITHNUMPY
            }
#endif
            if(PyErr_Occurred()) return 0;
            if(cTid == (uint32_t) -1) return 0;

#ifdef WITHNUMPY
            if(PyArray_Check(starts)) {
                ustart = getNumpyU32((PyArrayObject*)starts, i);
            } else {
#endif
                ustart = Numeric2Uint(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
            }
#endif
            if(PyErr_Occurred()) return 0;
#ifdef WITHNUMPY
            if(PyArray_Check(ends)) {
                uend = getNumpyU32((PyArrayObject*) ends, i);
            } else {
#endif
                uend = Numeric2Uint(PyList_GetItem(ends, i));
#ifdef WITHNUMPY
            }
#endif
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
        if(!PyList_Check(starts)
#ifdef WITHNUMPY
            && !PyArray_Check(starts)
#endif
        ) return 0;
        if(PyList_Check(starts)) sz = PyList_Size(starts);
#ifdef WITHNUMPY
        else if(PyArray_Check(starts)) sz += PyArray_Size(starts);
#endif
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
#ifdef WITHNUMPY
            if(PyArray_Check(starts)) {
                ustart = getNumpyU32((PyArrayObject*)starts, i);
            } else {
#endif
                ustart = Numeric2Uint(PyList_GetItem(starts, i));
#ifdef WITHNUMPY
            }
#endif
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

    if(!self->bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigWig file handle is not open!");
        return NULL;
    }

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
    return NULL;
}

/**************************************************************
*
* BigBed functions, added in 0.3.0
*
**************************************************************/

static PyObject *pyBBGetEntries(pyBigWigFile_t *self, PyObject *args, PyObject *kwds) {
    bigWigFile_t *bw = self->bw;
    uint32_t i;
    uint32_t start, end = -1, tid;
    unsigned long startl, endl;
    char *chrom;
    static char *kwd_list[] = {"chrom", "start", "end", "withString", NULL};
    PyObject *ret, *t, *starto = NULL, *endo = NULL;
    PyObject *withStringPy = Py_True;
    int withString = 1;
    bbOverlappingEntries_t *o;

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigBed file handle is not open!");
        return NULL;
    }

    if(bw->type == 0) {
        PyErr_SetString(PyExc_RuntimeError, "bigWig files have no entries! Use 'intervals' or 'values' instead.");
        return NULL;  
    }

    if(!PyArg_ParseTupleAndKeywords(args, kwds, "sOO|O", kwd_list, &chrom, &starto, &endo, &withStringPy)) {
        PyErr_SetString(PyExc_RuntimeError, "You must supply a chromosome, start and end position.\n");
        return NULL;
    }

    tid = bwGetTid(bw, chrom);

#ifdef WITHNUMPY
    if(PyArray_IsScalar(starto, Integer)) {
        startl = (long) getNumpyL(starto);
    } else
#endif
    if(PyLong_Check(starto)) {
        startl = PyLong_AsLong(starto);
#if PY_MAJOR_VERSION < 3
    } else if(PyInt_Check(starto)) {
        startl = PyInt_AsLong(starto);
#endif
    } else {
        PyErr_SetString(PyExc_RuntimeError, "The start coordinate must be a number!");
        return NULL;
    }

#ifdef WITHNUMPY
    if(PyArray_IsScalar(endo, Integer)) {
        endl = (long) getNumpyL(endo);
    } else
#endif
    if(PyLong_Check(endo)) {
        endl = PyLong_AsLong(endo);
#if PY_MAJOR_VERSION < 3
    } else if(PyInt_Check(endo)) {
        endl = PyInt_AsLong(endo);
#endif
    } else {
        PyErr_SetString(PyExc_RuntimeError, "The end coordinate must be a number!");
        return NULL;
    }

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

    if(withStringPy == Py_False) withString = 0;

    o = bbGetOverlappingEntries(bw, chrom, start, end, withString);
    if(!o) {
        PyErr_SetString(PyExc_RuntimeError, "An error occurred while fetching the overlapping entries!\n");
        return NULL;
    }
    if(!o->l) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    ret = PyList_New(o->l);
    if(!ret) goto error;

    for(i=0; i<o->l; i++) {
        if(withString) {
            t = Py_BuildValue("(iis)", o->start[i], o->end[i], o->str[i]);
        } else {
            t = Py_BuildValue("(ii)", o->start[i], o->end[i]);
        }
        if(!t) goto error;
        PyList_SetItem(ret, i, t);
    }

    bbDestroyOverlappingEntries(o);
    return ret;

error:
    Py_DECREF(ret);
    bbDestroyOverlappingEntries(o);
    PyErr_SetString(PyExc_RuntimeError, "An error occurred while constructing the output list and tuple!");
    return NULL;
}

static PyObject *pyBBGetSQL(pyBigWigFile_t *self, PyObject *args) {
    bigWigFile_t *bw = self->bw;
    char *str = bbGetSQL(bw);
    size_t len = 0;
    PyObject *o = NULL;

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigBed file handle is not open!");
        return NULL;
    }

    if(!str) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    len = strlen(str);

#if PY_MAJOR_VERSION >= 3
    o = PyBytes_FromStringAndSize(str, len);
#else
    o = PyString_FromStringAndSize(str, len);
#endif
    if(str) free(str);

    return o;
}

static PyObject *pyIsBigWig(pyBigWigFile_t *self, PyObject *args) {
    bigWigFile_t *bw = self->bw;
    if(bw->type == 0) {
        Py_INCREF(Py_True);
        return Py_True;
    }

    Py_INCREF(Py_False);
    return Py_False;
}

static PyObject *pyIsBigBed(pyBigWigFile_t *self, PyObject *args) {
    bigWigFile_t *bw = self->bw;

    if(!bw) {
        PyErr_SetString(PyExc_RuntimeError, "The bigBed file handle is not open!");
        return NULL;
    }

    if(bw->type == 1) {
        Py_INCREF(Py_True);
        return Py_True;
    }

    Py_INCREF(Py_False);
    return Py_False;
}

/**************************************************************
*
* End of bigBed functions
*
**************************************************************/

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pyBigWig(void) {
#else
PyMODINIT_FUNC initpyBigWig(void) {
#endif
    PyObject *res;
    errno = 0; //just in case

#if PY_MAJOR_VERSION >= 3
    if(Py_AtExit(bwCleanup)) return NULL;
    if(PyType_Ready(&bigWigFile) < 0) return NULL;
    if(bwInit(128000)) return NULL;
    res = PyModule_Create(&pyBigWigmodule);
    if(!res) return NULL;
#else
    if(Py_AtExit(bwCleanup)) return;
    if(PyType_Ready(&bigWigFile) < 0) return;
    if(bwInit(128000)) return;
    res = Py_InitModule3("pyBigWig", bwMethods, "A module for handling bigWig files");
#endif

    Py_INCREF(&bigWigFile);
    PyModule_AddObject(res, "pyBigWig", (PyObject *) &bigWigFile);

#ifdef WITHNUMPY
    //Add the numpy constant
    import_array(); //Needed for numpy stuff to work
    PyModule_AddIntConstant(res, "numpy", 1);
#else
    PyModule_AddIntConstant(res, "numpy", 0);
#endif
#ifdef NOCURL
    PyModule_AddIntConstant(res, "remote", 0);
#else
    PyModule_AddIntConstant(res, "remote", 1);
#endif
    PyModule_AddStringConstant(res, "__version__", pyBigWigVersion);

#if PY_MAJOR_VERSION >= 3
    return res;
#endif
}
