#include <Python.h>
#include <iostream>
#include "theano_mod_helper.h"
#include <math.h>
#include <numpy/arrayobject.h>
#include <numpy/arrayscalars.h>
#include <iostream>
#include <cmath>
//////////////////////
////  Support Code
//////////////////////


    namespace {
    struct __struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a {
        PyObject* __ERROR;

        PyObject* storage_V3;
PyObject* storage_V5;
PyObject* storage_V7;
PyObject* storage_V9;
PyObject* storage_V11;
PyObject* storage_V1;
        

        __struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a() {
            // This is only somewhat safe because we:
            //  1) Are not a virtual class
            //  2) Do not use any virtual classes in the members
            //  3) Deal with mostly POD and pointers

            // If this changes, we would have to revise this, but for
            // now I am tired of chasing segfaults because
            // initialization code had an error and some pointer has
            // a junk value.
            memset(this, 0, sizeof(*this));
        }
        ~__struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a(void) {
            cleanup();
        }

        int init(PyObject* __ERROR, PyObject* storage_V3, PyObject* storage_V5, PyObject* storage_V7, PyObject* storage_V9, PyObject* storage_V11, PyObject* storage_V1) {
            Py_XINCREF(storage_V3);
Py_XINCREF(storage_V5);
Py_XINCREF(storage_V7);
Py_XINCREF(storage_V9);
Py_XINCREF(storage_V11);
Py_XINCREF(storage_V1);
            this->storage_V3 = storage_V3;
this->storage_V5 = storage_V5;
this->storage_V7 = storage_V7;
this->storage_V9 = storage_V9;
this->storage_V11 = storage_V11;
this->storage_V1 = storage_V1;
            







            this->__ERROR = __ERROR;
            return 0;
        }
        void cleanup(void) {
            __label_1:

double __DUMMY_1;
__label_3:

double __DUMMY_3;
__label_5:

double __DUMMY_5;
__label_7:

double __DUMMY_7;
__label_9:

double __DUMMY_9;
__label_11:

double __DUMMY_11;
__label_14:

double __DUMMY_14;

            Py_XDECREF(this->storage_V3);
Py_XDECREF(this->storage_V5);
Py_XDECREF(this->storage_V7);
Py_XDECREF(this->storage_V9);
Py_XDECREF(this->storage_V11);
Py_XDECREF(this->storage_V1);
        }
        int run(void) {
            int __failure = 0;
            
    PyObject* py_V1;
    
        PyArrayObject* V1;
        
            typedef npy_int32 dtype_V1;
            
    PyObject* py_V3;
    
        PyArrayObject* V3;
        
            typedef npy_float32 dtype_V3;
            
    PyObject* py_V5;
    
        PyArrayObject* V5;
        
            typedef npy_float32 dtype_V5;
            
    PyObject* py_V7;
    
        PyArrayObject* V7;
        
            typedef npy_int32 dtype_V7;
            
    PyObject* py_V9;
    
        PyArrayObject* V9;
        
            typedef npy_float32 dtype_V9;
            
    PyObject* py_V11;
    
        PyArrayObject* V11;
        
            typedef npy_float32 dtype_V11;
            
{

    py_V1 = PyList_GET_ITEM(storage_V1, 0);
    {Py_XINCREF(py_V1);}
    
        if (py_V1 == Py_None)
        {
            
        V1 = NULL;
        
        }
        else
        {
            
            V1 = NULL;
            if (py_V1 == Py_None) {
                // We can either fail here or set V1 to NULL and rely on Ops
                // using tensors to handle the NULL case, but if they fail to do so
                // they'll end up with nasty segfaults, so this is public service.
                PyErr_SetString(PyExc_ValueError, "expected an ndarray, not None");
                {
        __failure = 2;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_2;}
            }
            if (!PyArray_Check(py_V1)) {
                PyErr_SetString(PyExc_ValueError, "expected an ndarray");
                {
        __failure = 2;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_2;}
            }
            // We expect NPY_INT32
            if (!PyArray_ISALIGNED((PyArrayObject*) py_V1)) {
                PyArrayObject * tmp = (PyArrayObject*) py_V1;
                PyErr_Format(PyExc_NotImplementedError,
                             "expected an aligned array of type %ld "
                             "(NPY_INT32), got non-aligned array of type %ld"
                             " with %ld dimensions, with 3 last dims "
                             "%ld, %ld, %ld"
                             " and 3 last strides %ld %ld, %ld.",
                             (long int) NPY_INT32,
                             (long int) PyArray_TYPE((PyArrayObject*) py_V1),
                             (long int) PyArray_NDIM(tmp),
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-1] : -1,
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-1] : -1
            );
                {
        __failure = 2;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_2;}
            }
            // This is a TypeError to be consistent with DEBUG_MODE
            // Note: DEBUG_MODE also tells the name of the container
            if (PyArray_TYPE((PyArrayObject*) py_V1) != NPY_INT32) {
                PyErr_Format(PyExc_TypeError,
                             "expected type_num %d (NPY_INT32) got %d",
                             NPY_INT32, PyArray_TYPE((PyArrayObject*) py_V1));
                {
        __failure = 2;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_2;}
            }
            
        V1 = (PyArrayObject*)(py_V1);
        Py_XINCREF(V1);
        
        }
        
{

    py_V3 = PyList_GET_ITEM(storage_V3, 0);
    {Py_XINCREF(py_V3);}
    
            V3 = NULL;
            if (py_V3 == Py_None) {
                // We can either fail here or set V3 to NULL and rely on Ops
                // using tensors to handle the NULL case, but if they fail to do so
                // they'll end up with nasty segfaults, so this is public service.
                PyErr_SetString(PyExc_ValueError, "expected an ndarray, not None");
                {
        __failure = 4;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_4;}
            }
            if (!PyArray_Check(py_V3)) {
                PyErr_SetString(PyExc_ValueError, "expected an ndarray");
                {
        __failure = 4;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_4;}
            }
            // We expect NPY_FLOAT32
            if (!PyArray_ISALIGNED((PyArrayObject*) py_V3)) {
                PyArrayObject * tmp = (PyArrayObject*) py_V3;
                PyErr_Format(PyExc_NotImplementedError,
                             "expected an aligned array of type %ld "
                             "(NPY_FLOAT32), got non-aligned array of type %ld"
                             " with %ld dimensions, with 3 last dims "
                             "%ld, %ld, %ld"
                             " and 3 last strides %ld %ld, %ld.",
                             (long int) NPY_FLOAT32,
                             (long int) PyArray_TYPE((PyArrayObject*) py_V3),
                             (long int) PyArray_NDIM(tmp),
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-1] : -1,
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-1] : -1
            );
                {
        __failure = 4;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_4;}
            }
            // This is a TypeError to be consistent with DEBUG_MODE
            // Note: DEBUG_MODE also tells the name of the container
            if (PyArray_TYPE((PyArrayObject*) py_V3) != NPY_FLOAT32) {
                PyErr_Format(PyExc_TypeError,
                             "expected type_num %d (NPY_FLOAT32) got %d",
                             NPY_FLOAT32, PyArray_TYPE((PyArrayObject*) py_V3));
                {
        __failure = 4;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_4;}
            }
            
        V3 = (PyArrayObject*)(py_V3);
        Py_XINCREF(V3);
        
{

    py_V5 = PyList_GET_ITEM(storage_V5, 0);
    {Py_XINCREF(py_V5);}
    
            V5 = NULL;
            if (py_V5 == Py_None) {
                // We can either fail here or set V5 to NULL and rely on Ops
                // using tensors to handle the NULL case, but if they fail to do so
                // they'll end up with nasty segfaults, so this is public service.
                PyErr_SetString(PyExc_ValueError, "expected an ndarray, not None");
                {
        __failure = 6;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_6;}
            }
            if (!PyArray_Check(py_V5)) {
                PyErr_SetString(PyExc_ValueError, "expected an ndarray");
                {
        __failure = 6;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_6;}
            }
            // We expect NPY_FLOAT32
            if (!PyArray_ISALIGNED((PyArrayObject*) py_V5)) {
                PyArrayObject * tmp = (PyArrayObject*) py_V5;
                PyErr_Format(PyExc_NotImplementedError,
                             "expected an aligned array of type %ld "
                             "(NPY_FLOAT32), got non-aligned array of type %ld"
                             " with %ld dimensions, with 3 last dims "
                             "%ld, %ld, %ld"
                             " and 3 last strides %ld %ld, %ld.",
                             (long int) NPY_FLOAT32,
                             (long int) PyArray_TYPE((PyArrayObject*) py_V5),
                             (long int) PyArray_NDIM(tmp),
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-1] : -1,
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-1] : -1
            );
                {
        __failure = 6;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_6;}
            }
            // This is a TypeError to be consistent with DEBUG_MODE
            // Note: DEBUG_MODE also tells the name of the container
            if (PyArray_TYPE((PyArrayObject*) py_V5) != NPY_FLOAT32) {
                PyErr_Format(PyExc_TypeError,
                             "expected type_num %d (NPY_FLOAT32) got %d",
                             NPY_FLOAT32, PyArray_TYPE((PyArrayObject*) py_V5));
                {
        __failure = 6;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_6;}
            }
            
        V5 = (PyArrayObject*)(py_V5);
        Py_XINCREF(V5);
        
{

    py_V7 = PyList_GET_ITEM(storage_V7, 0);
    {Py_XINCREF(py_V7);}
    
            V7 = NULL;
            if (py_V7 == Py_None) {
                // We can either fail here or set V7 to NULL and rely on Ops
                // using tensors to handle the NULL case, but if they fail to do so
                // they'll end up with nasty segfaults, so this is public service.
                PyErr_SetString(PyExc_ValueError, "expected an ndarray, not None");
                {
        __failure = 8;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_8;}
            }
            if (!PyArray_Check(py_V7)) {
                PyErr_SetString(PyExc_ValueError, "expected an ndarray");
                {
        __failure = 8;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_8;}
            }
            // We expect NPY_INT32
            if (!PyArray_ISALIGNED((PyArrayObject*) py_V7)) {
                PyArrayObject * tmp = (PyArrayObject*) py_V7;
                PyErr_Format(PyExc_NotImplementedError,
                             "expected an aligned array of type %ld "
                             "(NPY_INT32), got non-aligned array of type %ld"
                             " with %ld dimensions, with 3 last dims "
                             "%ld, %ld, %ld"
                             " and 3 last strides %ld %ld, %ld.",
                             (long int) NPY_INT32,
                             (long int) PyArray_TYPE((PyArrayObject*) py_V7),
                             (long int) PyArray_NDIM(tmp),
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-1] : -1,
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-1] : -1
            );
                {
        __failure = 8;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_8;}
            }
            // This is a TypeError to be consistent with DEBUG_MODE
            // Note: DEBUG_MODE also tells the name of the container
            if (PyArray_TYPE((PyArrayObject*) py_V7) != NPY_INT32) {
                PyErr_Format(PyExc_TypeError,
                             "expected type_num %d (NPY_INT32) got %d",
                             NPY_INT32, PyArray_TYPE((PyArrayObject*) py_V7));
                {
        __failure = 8;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_8;}
            }
            
        V7 = (PyArrayObject*)(py_V7);
        Py_XINCREF(V7);
        
{

    py_V9 = PyList_GET_ITEM(storage_V9, 0);
    {Py_XINCREF(py_V9);}
    
        if (py_V9 == Py_None)
        {
            
        V9 = NULL;
        
        }
        else
        {
            
            V9 = NULL;
            if (py_V9 == Py_None) {
                // We can either fail here or set V9 to NULL and rely on Ops
                // using tensors to handle the NULL case, but if they fail to do so
                // they'll end up with nasty segfaults, so this is public service.
                PyErr_SetString(PyExc_ValueError, "expected an ndarray, not None");
                {
        __failure = 10;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_10;}
            }
            if (!PyArray_Check(py_V9)) {
                PyErr_SetString(PyExc_ValueError, "expected an ndarray");
                {
        __failure = 10;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_10;}
            }
            // We expect NPY_FLOAT32
            if (!PyArray_ISALIGNED((PyArrayObject*) py_V9)) {
                PyArrayObject * tmp = (PyArrayObject*) py_V9;
                PyErr_Format(PyExc_NotImplementedError,
                             "expected an aligned array of type %ld "
                             "(NPY_FLOAT32), got non-aligned array of type %ld"
                             " with %ld dimensions, with 3 last dims "
                             "%ld, %ld, %ld"
                             " and 3 last strides %ld %ld, %ld.",
                             (long int) NPY_FLOAT32,
                             (long int) PyArray_TYPE((PyArrayObject*) py_V9),
                             (long int) PyArray_NDIM(tmp),
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-1] : -1,
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-1] : -1
            );
                {
        __failure = 10;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_10;}
            }
            // This is a TypeError to be consistent with DEBUG_MODE
            // Note: DEBUG_MODE also tells the name of the container
            if (PyArray_TYPE((PyArrayObject*) py_V9) != NPY_FLOAT32) {
                PyErr_Format(PyExc_TypeError,
                             "expected type_num %d (NPY_FLOAT32) got %d",
                             NPY_FLOAT32, PyArray_TYPE((PyArrayObject*) py_V9));
                {
        __failure = 10;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_10;}
            }
            
        V9 = (PyArrayObject*)(py_V9);
        Py_XINCREF(V9);
        
        }
        
{

    py_V11 = PyList_GET_ITEM(storage_V11, 0);
    {Py_XINCREF(py_V11);}
    
        if (py_V11 == Py_None)
        {
            
        V11 = NULL;
        
        }
        else
        {
            
            V11 = NULL;
            if (py_V11 == Py_None) {
                // We can either fail here or set V11 to NULL and rely on Ops
                // using tensors to handle the NULL case, but if they fail to do so
                // they'll end up with nasty segfaults, so this is public service.
                PyErr_SetString(PyExc_ValueError, "expected an ndarray, not None");
                {
        __failure = 12;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_12;}
            }
            if (!PyArray_Check(py_V11)) {
                PyErr_SetString(PyExc_ValueError, "expected an ndarray");
                {
        __failure = 12;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_12;}
            }
            // We expect NPY_FLOAT32
            if (!PyArray_ISALIGNED((PyArrayObject*) py_V11)) {
                PyArrayObject * tmp = (PyArrayObject*) py_V11;
                PyErr_Format(PyExc_NotImplementedError,
                             "expected an aligned array of type %ld "
                             "(NPY_FLOAT32), got non-aligned array of type %ld"
                             " with %ld dimensions, with 3 last dims "
                             "%ld, %ld, %ld"
                             " and 3 last strides %ld %ld, %ld.",
                             (long int) NPY_FLOAT32,
                             (long int) PyArray_TYPE((PyArrayObject*) py_V11),
                             (long int) PyArray_NDIM(tmp),
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_DIMS(tmp)[PyArray_NDIM(tmp)-1] : -1,
                             (long int) PyArray_NDIM(tmp) >= 3 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-3] : -1,
                             (long int) PyArray_NDIM(tmp) >= 2 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-2] : -1,
                             (long int) PyArray_NDIM(tmp) >= 1 ?
            PyArray_STRIDES(tmp)[PyArray_NDIM(tmp)-1] : -1
            );
                {
        __failure = 12;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_12;}
            }
            // This is a TypeError to be consistent with DEBUG_MODE
            // Note: DEBUG_MODE also tells the name of the container
            if (PyArray_TYPE((PyArrayObject*) py_V11) != NPY_FLOAT32) {
                PyErr_Format(PyExc_TypeError,
                             "expected type_num %d (NPY_FLOAT32) got %d",
                             NPY_FLOAT32, PyArray_TYPE((PyArrayObject*) py_V11));
                {
        __failure = 12;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_12;}
            }
            
        V11 = (PyArrayObject*)(py_V11);
        Py_XINCREF(V11);
        
        }
        
{
// Op class CrossentropySoftmaxArgmax1HotWithBias

        npy_intp* Nx = PyArray_DIMS(V3);
        npy_intp Sx = 0;
        npy_intp Sb = 0;
        npy_intp Ssm = 0;


        if (PyArray_NDIM(V3) != 2)
        {
            PyErr_SetString(PyExc_ValueError, "not a 2d tensor");
            {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
        }
        if (PyArray_NDIM(V5) != 1)
        {
            PyErr_SetString(PyExc_ValueError, "b not 1d tensor");
            {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
        }
        if ((PyArray_TYPE(V3) != NPY_DOUBLE) &&
            (PyArray_TYPE(V3) != NPY_FLOAT))
        {
            PyErr_SetString(PyExc_TypeError, "not a float");
            {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
        }
        if ((PyArray_TYPE(V5) != NPY_DOUBLE) &&
            (PyArray_TYPE(V5) != NPY_FLOAT))
        {
            PyErr_SetString(PyExc_TypeError, "b not float");
            {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
        }
        if ((PyArray_DIMS(V3)[1] != PyArray_DIMS(V5)[0]))
        {
            PyErr_Format(PyExc_ValueError,
                         "number of columns in x (%ld) does not match length of b (%ld)",
                (long int)PyArray_DIMS(V3)[1], (long int)PyArray_DIMS(V5)[0]);
            {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
        }

        if ((NULL == V11)
            || (PyArray_DIMS(V11)[0] != PyArray_DIMS(V3)[0])
            || (PyArray_DIMS(V11)[1] != PyArray_DIMS(V3)[1]))
        {
            if (NULL != V11) Py_XDECREF(V11);
            V11 = (PyArrayObject*)PyArray_SimpleNew(2, PyArray_DIMS(V3),
                                                       PyArray_TYPE(V3));
            if(!V11) {
                PyErr_SetString(PyExc_MemoryError,
                     "failed to alloc sm output");
                {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;}
            }
        }
        Sx = PyArray_STRIDES(V3)[1]/sizeof(dtype_V3);
        Sb = PyArray_STRIDES(V5)[0]/sizeof(dtype_V5);
        Ssm = PyArray_STRIDES(V11)[1]/sizeof(dtype_V11);

        
        if (PyArray_NDIM(V7) != 1)
        {
            PyErr_SetString(PyExc_ValueError, "y_idx not 1d tensor");
            {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
        }
        if (PyArray_DIMS(V3)[0] != PyArray_DIMS(V7)[0])
        {
            PyErr_Format(PyExc_ValueError,
                "number of rows in x (%ld) does not match length of y (%ld)",
                (long int)PyArray_DIMS(V3)[0],
                (long int)PyArray_DIMS(V7)[0]);
            {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
        }

        if ((NULL == V9) //initial condition
            || (PyArray_DIMS(V9)[0] != PyArray_DIMS(V7)[0]))
        {
            if (NULL != V9) Py_XDECREF(V9);
            V9 = (PyArrayObject*)PyArray_SimpleNew(1,
                PyArray_DIMS(V7), PyArray_TYPE(V3));
            if(!V9)
            {
                PyErr_SetString(PyExc_MemoryError,
                     "failed to alloc nll output");
                {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
            }
        }
        if ((NULL == V1)
            || (PyArray_DIMS(V1)[0] != PyArray_DIMS(V7)[0]))
        {
            Py_XDECREF(V1);
            V1 = (PyArrayObject*) PyArray_SimpleNew(1,
                PyArray_DIMS(V7), PyArray_TYPE(V7));
            if(!V1)
            {
                PyErr_SetString(PyExc_MemoryError,
                     "failed to alloc am output");
                {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
            }
        }
                
        for (size_t i = 0; i < Nx[0]; ++i)
        {
            size_t j;
            double sum = 0.0;

            const dtype_V3* __restrict__ x_i = (dtype_V3*)(PyArray_BYTES(V3) + PyArray_STRIDES(V3)[0] * i);
            const dtype_V5* __restrict__ b_i = (dtype_V5*)(PyArray_BYTES(V5));
            dtype_V11* __restrict__ sm_i = (dtype_V11*)(PyArray_BYTES(V11) + PyArray_STRIDES(V11)[0] * i);

            npy_intp Sx = PyArray_STRIDES(V3)[1]/sizeof(dtype_V3);
            npy_intp Sb = PyArray_STRIDES(V5)[0]/sizeof(dtype_V5);
            npy_intp Ssm = PyArray_STRIDES(V11)[1]/sizeof(dtype_V11);

            size_t row_max_j=0;
            dtype_V11 row_max = x_i[0] + b_i[0];
            //std::cout << "0 " << row_max << "\n";
            // Get the maximum value of the row
            for (j = 1; j < Nx[1]; ++j)
            {
                dtype_V11 row_ij = x_i[j * Sx] +  b_i[j * Sb];
                //std::cout << "1 " << row_ij << "\n";
                row_max_j = (row_ij > row_max) ? j : row_max_j;
                row_max   = (row_ij > row_max) ? row_ij : row_max;
            }

        
            const npy_int32 y_i = ((npy_int32*)(PyArray_BYTES(V7) + PyArray_STRIDES(V7)[0] * i))[0];
            dtype_V9* __restrict__ nll_i = (dtype_V9*)(PyArray_BYTES(V9) + PyArray_STRIDES(V9)[0] * i);
            npy_int32* __restrict__ am_i = (npy_int32*) (PyArray_BYTES(V1) + PyArray_STRIDES(V1)[0] * i);
                
            for (j = 0; j < Nx[1]; ++j)
            {
                dtype_V11 row_ij = x_i[j * Sx] +  b_i[j * Sb];
                //std::cout << "2 " << j << " " << row_ij << " " << row_max << "\n";
                dtype_V11 sm_ij = exp(row_ij - row_max);
                //std::cout << "3 " << j << " " << sm_ij << "\n";
                sum += sm_ij;
                sm_i[j * Ssm] = sm_ij;
            }

            //cblas_dscal(x.N, 1.0 / sum, &mat_at(s,i,0), s.n);
            double sum_inv = 1.0 / sum;
            for (j = 0; j < Nx[1]; ++j)
            {
                sm_i[j * Ssm] *= sum_inv;
            }

        
            if ((y_i >= PyArray_DIMS(V3)[1]) || (y_i < 0))
            {
                PyErr_SetString(PyExc_ValueError, "y_i value out of bounds");
                {
        __failure = 13;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_13;};
            }
            nll_i[0] = - x_i[y_i*Sx]
                       - b_i[y_i*Sb]
                       + row_max
                       + log(sum);
            am_i[0] = row_max_j;
                
        }
        __label_13:

double __DUMMY_13;

}
__label_12:

    if (!__failure) {
      
        {Py_XDECREF(py_V11);}
        if (!V11) {
            Py_INCREF(Py_None);
            py_V11 = Py_None;
        }
        else if ((void*)py_V11 != (void*)V11) {
            py_V11 = (PyObject*)V11;
        }

        {Py_XINCREF(py_V11);}

        if (V11 && !PyArray_ISALIGNED((PyArrayObject*) py_V11)) {
            PyErr_Format(PyExc_NotImplementedError,
                         "c_sync: expected an aligned array, got non-aligned array of type %ld"
                         " with %ld dimensions, with 3 last dims "
                         "%ld, %ld, %ld"
                         " and 3 last strides %ld %ld, %ld.",
                         (long int) PyArray_TYPE((PyArrayObject*) py_V11),
                         (long int) PyArray_NDIM(V11),
                         (long int) PyArray_NDIM(V11) >= 3 ?
        PyArray_DIMS(V11)[PyArray_NDIM(V11)-3] : -1,
                         (long int) PyArray_NDIM(V11) >= 2 ?
        PyArray_DIMS(V11)[PyArray_NDIM(V11)-2] : -1,
                         (long int) PyArray_NDIM(V11) >= 1 ?
        PyArray_DIMS(V11)[PyArray_NDIM(V11)-1] : -1,
                         (long int) PyArray_NDIM(V11) >= 3 ?
        PyArray_STRIDES(V11)[PyArray_NDIM(V11)-3] : -1,
                         (long int) PyArray_NDIM(V11) >= 2 ?
        PyArray_STRIDES(V11)[PyArray_NDIM(V11)-2] : -1,
                         (long int) PyArray_NDIM(V11) >= 1 ?
        PyArray_STRIDES(V11)[PyArray_NDIM(V11)-1] : -1
        );
            {
        __failure = 12;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_12;}
        }
        
      PyObject* old = PyList_GET_ITEM(storage_V11, 0);
      {Py_XINCREF(py_V11);}
      PyList_SET_ITEM(storage_V11, 0, py_V11);
      {Py_XDECREF(old);}
    }
    
        if (V11) {
            Py_XDECREF(V11);
        }
        
    {Py_XDECREF(py_V11);}
    
double __DUMMY_12;

}
__label_10:

    if (!__failure) {
      
        {Py_XDECREF(py_V9);}
        if (!V9) {
            Py_INCREF(Py_None);
            py_V9 = Py_None;
        }
        else if ((void*)py_V9 != (void*)V9) {
            py_V9 = (PyObject*)V9;
        }

        {Py_XINCREF(py_V9);}

        if (V9 && !PyArray_ISALIGNED((PyArrayObject*) py_V9)) {
            PyErr_Format(PyExc_NotImplementedError,
                         "c_sync: expected an aligned array, got non-aligned array of type %ld"
                         " with %ld dimensions, with 3 last dims "
                         "%ld, %ld, %ld"
                         " and 3 last strides %ld %ld, %ld.",
                         (long int) PyArray_TYPE((PyArrayObject*) py_V9),
                         (long int) PyArray_NDIM(V9),
                         (long int) PyArray_NDIM(V9) >= 3 ?
        PyArray_DIMS(V9)[PyArray_NDIM(V9)-3] : -1,
                         (long int) PyArray_NDIM(V9) >= 2 ?
        PyArray_DIMS(V9)[PyArray_NDIM(V9)-2] : -1,
                         (long int) PyArray_NDIM(V9) >= 1 ?
        PyArray_DIMS(V9)[PyArray_NDIM(V9)-1] : -1,
                         (long int) PyArray_NDIM(V9) >= 3 ?
        PyArray_STRIDES(V9)[PyArray_NDIM(V9)-3] : -1,
                         (long int) PyArray_NDIM(V9) >= 2 ?
        PyArray_STRIDES(V9)[PyArray_NDIM(V9)-2] : -1,
                         (long int) PyArray_NDIM(V9) >= 1 ?
        PyArray_STRIDES(V9)[PyArray_NDIM(V9)-1] : -1
        );
            {
        __failure = 10;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_10;}
        }
        
      PyObject* old = PyList_GET_ITEM(storage_V9, 0);
      {Py_XINCREF(py_V9);}
      PyList_SET_ITEM(storage_V9, 0, py_V9);
      {Py_XDECREF(old);}
    }
    
        if (V9) {
            Py_XDECREF(V9);
        }
        
    {Py_XDECREF(py_V9);}
    
double __DUMMY_10;

}
__label_8:

        if (V7) {
            Py_XDECREF(V7);
        }
        
    {Py_XDECREF(py_V7);}
    
double __DUMMY_8;

}
__label_6:

        if (V5) {
            Py_XDECREF(V5);
        }
        
    {Py_XDECREF(py_V5);}
    
double __DUMMY_6;

}
__label_4:

        if (V3) {
            Py_XDECREF(V3);
        }
        
    {Py_XDECREF(py_V3);}
    
double __DUMMY_4;

}
__label_2:

    if (!__failure) {
      
        {Py_XDECREF(py_V1);}
        if (!V1) {
            Py_INCREF(Py_None);
            py_V1 = Py_None;
        }
        else if ((void*)py_V1 != (void*)V1) {
            py_V1 = (PyObject*)V1;
        }

        {Py_XINCREF(py_V1);}

        if (V1 && !PyArray_ISALIGNED((PyArrayObject*) py_V1)) {
            PyErr_Format(PyExc_NotImplementedError,
                         "c_sync: expected an aligned array, got non-aligned array of type %ld"
                         " with %ld dimensions, with 3 last dims "
                         "%ld, %ld, %ld"
                         " and 3 last strides %ld %ld, %ld.",
                         (long int) PyArray_TYPE((PyArrayObject*) py_V1),
                         (long int) PyArray_NDIM(V1),
                         (long int) PyArray_NDIM(V1) >= 3 ?
        PyArray_DIMS(V1)[PyArray_NDIM(V1)-3] : -1,
                         (long int) PyArray_NDIM(V1) >= 2 ?
        PyArray_DIMS(V1)[PyArray_NDIM(V1)-2] : -1,
                         (long int) PyArray_NDIM(V1) >= 1 ?
        PyArray_DIMS(V1)[PyArray_NDIM(V1)-1] : -1,
                         (long int) PyArray_NDIM(V1) >= 3 ?
        PyArray_STRIDES(V1)[PyArray_NDIM(V1)-3] : -1,
                         (long int) PyArray_NDIM(V1) >= 2 ?
        PyArray_STRIDES(V1)[PyArray_NDIM(V1)-2] : -1,
                         (long int) PyArray_NDIM(V1) >= 1 ?
        PyArray_STRIDES(V1)[PyArray_NDIM(V1)-1] : -1
        );
            {
        __failure = 2;
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Unexpected error in an Op's C code. "
                "No Python exception was set.");
            }
        goto __label_2;}
        }
        
      PyObject* old = PyList_GET_ITEM(storage_V1, 0);
      {Py_XINCREF(py_V1);}
      PyList_SET_ITEM(storage_V1, 0, py_V1);
      {Py_XDECREF(old);}
    }
    
        if (V1) {
            Py_XDECREF(V1);
        }
        
    {Py_XDECREF(py_V1);}
    
double __DUMMY_2;

}

            
        if (__failure) {
            // When there is a failure, this code puts the exception
            // in __ERROR.
            PyObject* err_type = NULL;
            PyObject* err_msg = NULL;
            PyObject* err_traceback = NULL;
            PyErr_Fetch(&err_type, &err_msg, &err_traceback);
            if (!err_type) {err_type = Py_None;Py_INCREF(Py_None);}
            if (!err_msg) {err_msg = Py_None; Py_INCREF(Py_None);}
            if (!err_traceback) {err_traceback = Py_None; Py_INCREF(Py_None);}
            PyObject* old_err_type = PyList_GET_ITEM(__ERROR, 0);
            PyObject* old_err_msg = PyList_GET_ITEM(__ERROR, 1);
            PyObject* old_err_traceback = PyList_GET_ITEM(__ERROR, 2);
            PyList_SET_ITEM(__ERROR, 0, err_type);
            PyList_SET_ITEM(__ERROR, 1, err_msg);
            PyList_SET_ITEM(__ERROR, 2, err_traceback);
            {Py_XDECREF(old_err_type);}
            {Py_XDECREF(old_err_msg);}
            {Py_XDECREF(old_err_traceback);}
        }
        // The failure code is returned to index what code block failed.
        return __failure;
        
        }
    };
    }
    

        static int __struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a_executor(__struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a* self) {
            return self->run();
        }

        static void __struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a_destructor(void* executor, void* self) {
            delete ((__struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a*)self);
        }
        
//////////////////////
////  Functions
//////////////////////
static PyObject * instantiate(PyObject * self, PyObject *argtuple) {
  assert(PyTuple_Check(argtuple));
  if (7 != PyTuple_Size(argtuple)){ 
     PyErr_Format(PyExc_TypeError, "Wrong number of arguments, expected 7, got %i", (int)PyTuple_Size(argtuple));
     return NULL;
  }
  __struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a* struct_ptr = new __struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a();
  if (struct_ptr->init( PyTuple_GET_ITEM(argtuple, 0),PyTuple_GET_ITEM(argtuple, 1),PyTuple_GET_ITEM(argtuple, 2),PyTuple_GET_ITEM(argtuple, 3),PyTuple_GET_ITEM(argtuple, 4),PyTuple_GET_ITEM(argtuple, 5),PyTuple_GET_ITEM(argtuple, 6) ) != 0) {
    delete struct_ptr;
    return NULL;
  }
  PyObject* thunk = PyCObject_FromVoidPtrAndDesc((void*)(&__struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a_executor), struct_ptr, __struct_compiled_op_6fe68fbfaebddd390ea8a9ce5e4a9c3a_destructor);
  return thunk; }

//////////////////////
////  Module init
//////////////////////
static PyMethodDef MyMethods[] = {
	{"instantiate", instantiate, METH_VARARGS, "undocumented"} ,
	{NULL, NULL, 0, NULL}
};
PyMODINIT_FUNC init6fe68fbfaebddd390ea8a9ce5e4a9c3a(void){
   import_array();
   (void) Py_InitModule("6fe68fbfaebddd390ea8a9ce5e4a9c3a", MyMethods);
}
