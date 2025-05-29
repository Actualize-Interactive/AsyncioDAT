#pragma once

#include "DAT_CPlusPlusBase.h"
#include <cstdint>

#ifdef _WIN32
	#include <Python.h>
	#include <structmember.h>
	#include <modsupport.h>
#else
	#include <Python.h>
	#include <structmember.h>
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
PyObject* py_pollEventLoop(PyObject* self, PyObject* args);
PyObject* py_getEventLoop(PyObject* self, PyObject* args);
PyObject* py_addAsyncTask(PyObject* self, PyObject* args);
PyObject* py_createAsyncTask(PyObject* self, PyObject* args);
PyObject* py_runCoroutine(PyObject* self, PyObject* args);
PyObject* py_isAsyncioRunning(PyObject* self, PyObject* args);
PyObject* py_getCallbackCount(PyObject* self, PyObject* args);

// Property getters/setters
PyObject* py_getLoopRunning(PyObject* self, void* closure);
PyObject* py_getAsyncioInitialized(PyObject* self, void* closure);

// Plugins
PyObject* py_setPlugin(PyObject* self, PyObject* args);
PyObject* py_getPlugin(PyObject* self, PyObject* args);
PyObject* py_delPlugin(PyObject* self, PyObject* args);
PyObject* py_hasPlugin(PyObject* self, PyObject* args);
PyObject* py_clearPlugins(PyObject* self, PyObject* args);
PyObject* py_getPluginNames(PyObject* self, void* closure);
PyObject* py_getPlugins(PyObject* self, void* closure);

} // extern "C"