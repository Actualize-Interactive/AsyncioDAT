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

namespace py
{
extern PyMethodDef methods[];
extern PyGetSetDef getSets[];
extern const char* pythonCallbacksDATStubs;

// Asyncio Event Loop Management
class AsyncioManager {
public:
    AsyncioManager();
    ~AsyncioManager();
    bool initialize(const TD::OP_NodeInfo* nodeInfo = nullptr);
    void shutdown();
    bool processEvents();
    PyObject* getLoop();
    bool addTask(PyObject* coro);
    bool isRunning() const { return m_loopRunning; }
    int getReadyCallbackCount();
	bool createExceptionHandler();

    uint32_t getProcessEventsCount() const { return m_processEventsCount; }
    double getLastProcessEventsTime() const { return m_lastProcessEventsTime; }

private:
    const TD::OP_NodeInfo* m_nodeInfo;

    PyObject* m_asyncioModule;
    PyObject* m_eventLoop;
    PyObject* m_newEventLoop;
    PyObject* m_setEventLoop;
    PyObject* m_getEventLoop;
    PyObject* m_runUntilComplete;
    PyObject* m_createTask;
    PyObject* m_sleep;
    PyObject* m_callSoon;
    PyObject* m_stop;
    PyObject* m_runForever;
	PyObject* m_exceptionHandler; 
    bool      m_loopRunning;
    bool      m_initialized;

    uint32_t  m_processEventsCount;
    double    m_lastProcessEventsTime;
};

// Global asyncio manager instance
void acquireSharedAsyncioManager();
void releaseSharedAsyncioManager();
int getAsyncioRefCount();
extern AsyncioManager* g_asyncioManager;

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

// Utility functions
bool ensureAsyncioInitialized();
void cleanupAsyncio();

// info
uint32_t getProcessEventsCount();
double getLastProcessEventsTime();

} // namespace py



