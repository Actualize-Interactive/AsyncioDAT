#pragma once

#include "DAT_CPlusPlusBase.h"

#ifdef _WIN32
	#include <Python.h>
	#include <structmember.h>
	#include <modsupport.h>
#else
	#include <Python/Python.h>
	#include <Python/structmember.h>
#endif

// Forward declaration
class AsyncioDAT;


extern PyMethodDef py_methods[];
extern PyGetSetDef py_getSets[];
extern const char* py_callbacksDATStubs;

// Global active instance management
extern AsyncioDAT* g_activeAsyncioInstance;

extern "C" {

// Python C API Method Implementations
PyObject* py_initializeAsyncio(PyObject* self, PyObject* args);
PyObject* py_shutdownAsyncio(PyObject* self, PyObject* args);
PyObject* py_processAsyncioEvents(PyObject* self, PyObject* args);
PyObject* py_getEventLoop(PyObject* self, PyObject* args);
PyObject* py_addAsyncTask(PyObject* self, PyObject* args);
PyObject* py_createAsyncTask(PyObject* self, PyObject* args);
PyObject* py_runCoroutine(PyObject* self, PyObject* args);
PyObject* py_isAsyncioRunning(PyObject* self, PyObject* args);
PyObject* py_getCallbackCount(PyObject* self, PyObject* args);

// Property getters/setters
PyObject* py_getLoopRunning(PyObject* self, void* closure);
PyObject* py_getAsyncioInitialized(PyObject* self, void* closure);

} // extern "C"