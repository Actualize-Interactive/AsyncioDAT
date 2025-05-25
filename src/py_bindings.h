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

namespace py
{
extern PyMethodDef methods[];
extern PyGetSetDef getSets[];
extern const char* pythonCallbacksDATStubs;

// Global active instance management
extern AsyncioDAT* g_activeAsyncioInstance;
void setActiveAsyncioInstance(AsyncioDAT* instance);
AsyncioDAT* getActiveAsyncioInstance();

// Python C API Method Implementations
PyObject* initializeAsyncio(PyObject* self, PyObject* args);
PyObject* shutdownAsyncio(PyObject* self, PyObject* args);
PyObject* processAsyncioEvents(PyObject* self, PyObject* args);
PyObject* getEventLoop(PyObject* self, PyObject* args);
PyObject* addAsyncTask(PyObject* self, PyObject* args);
PyObject* createAsyncTask(PyObject* self, PyObject* args);
PyObject* runCoroutine(PyObject* self, PyObject* args);
PyObject* isAsyncioRunning(PyObject* self, PyObject* args);
PyObject* getCallbackCount(PyObject* self, PyObject* args);

// Property getters/setters
PyObject* getLoopRunning(PyObject* self, void* closure);
PyObject* getAsyncioInitialized(PyObject* self, void* closure);

} // namespace py
