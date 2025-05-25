#include "py_bindings.h"
#include "asyncio_dat.h"

namespace py
{

PyMethodDef methods[] =
{
    {"initialize_asyncio", py::initializeAsyncio, METH_NOARGS, "Initialize the asyncio event loop"},
    {"shutdown_asyncio", py::shutdownAsyncio, METH_NOARGS, "Shutdown the asyncio event loop"},
    {"process_events", py::processAsyncioEvents, METH_NOARGS, "Process asyncio events for one frame"},
    {"get_event_loop", py::getEventLoop, METH_NOARGS, "Get the current event loop"},
    {"add_task", py::addAsyncTask, METH_VARARGS, "Add a coroutine as a task to the event loop"},    
    {"create_task", py::createAsyncTask, METH_VARARGS, "Create a task from a coroutine"},
    {"run_coroutine", py::runCoroutine, METH_VARARGS, "Run a coroutine until complete"},
    {"is_running", py::isAsyncioRunning, METH_NOARGS, "Check if asyncio is running"},
    {"get_callback_count", py::getCallbackCount, METH_NOARGS, "Get number of ready callbacks in the event loop"},
    {0}	// Sentinel
};

PyGetSetDef getSets[] =
{
    {"loop_running", py::getLoopRunning, nullptr, "Whether the event loop is running", nullptr},
    {"asyncio_initialized", py::getAsyncioInitialized, nullptr, "Whether asyncio is initialized", nullptr},
    {0}	// Sentinel
};

const char* pythonCallbacksDATStubs =
R"(# AsyncioDAT Python Callbacks

def on_initialize(asyncioDat, success):
    """Called when the AsyncioDAT is initialized."""
    pass

def on_startup(asyncioDat, success):
    """Called when the AsyncioDAT is started up."""
    pass

def on_pre_shutdown(asyncioDat, success, info):
    """Called when the AsyncioDAT is shutdown."""
    pass

def on_post_shutdown(asyncioDat, success, info):
    """Called after the AsyncioDAT has been shutdown."""
    pass

)";

// Global active instance
AsyncioDAT* g_activeAsyncioInstance = nullptr;

void setActiveAsyncioInstance(AsyncioDAT* instance)
{
    g_activeAsyncioInstance = instance;
}

AsyncioDAT* getActiveAsyncioInstance()
{
    return g_activeAsyncioInstance;
}

// Python function implementations that forward to active instance
PyObject* initializeAsyncio(PyObject* self, PyObject* args)
{
    if (g_activeAsyncioInstance) {
        auto success = g_activeAsyncioInstance->initializeAsyncio();
        return PyBool_FromLong(success ? 1 : 0);
    }
    PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
    return nullptr;
}

PyObject* shutdownAsyncio(PyObject* self, PyObject* args)
{
    // if ((AsyncioDAT*)self != g_activeAsyncioInstance) {
    //     PyErr_SetString(PyExc_RuntimeError, "This method can only be called from the active AsyncioDAT instance");
    //     return nullptr;
    // }
    if (g_activeAsyncioInstance) {
        g_activeAsyncioInstance->shutdownAsyncio();
    }
    Py_RETURN_NONE;
}

PyObject* processAsyncioEvents(PyObject* self, PyObject* args)
{
    // if ((AsyncioDAT*)self != g_activeAsyncioInstance) {
    //     PyErr_SetString(PyExc_RuntimeError, "This method can only be called from the active AsyncioDAT instance");
    //     return nullptr;
    // }

    if (g_activeAsyncioInstance) {
        bool success = g_activeAsyncioInstance->processAsyncioEvents();
        return PyBool_FromLong(success ? 1 : 0);
    }
    Py_RETURN_FALSE;
}

PyObject* getEventLoop(PyObject* self, PyObject* args)
{
    if (g_activeAsyncioInstance) {
        return g_activeAsyncioInstance->getEventLoop();
    }
    Py_RETURN_NONE;
}

PyObject* addAsyncTask(PyObject* self, PyObject* args)
{
    // if ((AsyncioDAT*)self != g_activeAsyncioInstance) {
    //     PyErr_SetString(PyExc_RuntimeError, "This method can only be called from the active AsyncioDAT instance");
    //     return nullptr;
    // }

    PyObject* coro = nullptr;
    if (!PyArg_ParseTuple(args, "O", &coro)) {
        PyErr_SetString(PyExc_TypeError, "Expected a coroutine object");
        return nullptr;
    }
    
    if (g_activeAsyncioInstance) {
        bool success = g_activeAsyncioInstance->addTask(coro);
        return PyBool_FromLong(success ? 1 : 0);
    }
    
    PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
    return nullptr;
}

PyObject* createAsyncTask(PyObject* self, PyObject* args)
{
    // if ((AsyncioDAT*)self != g_activeAsyncioInstance) {
    //     PyErr_SetString(PyExc_RuntimeError, "This method can only be called from the active AsyncioDAT instance");
    //     return nullptr;
    // }

    PyObject* coro = nullptr;
    if (!PyArg_ParseTuple(args, "O", &coro)) {
        PyErr_SetString(PyExc_TypeError, "Expected a coroutine object");
        return nullptr;
    }
    
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    return g_activeAsyncioInstance->createAsyncTask(coro);
}

PyObject* runCoroutine(PyObject* self, PyObject* args)
{
    // if ((AsyncioDAT*)self != g_activeAsyncioInstance) {
    //     PyErr_SetString(PyExc_RuntimeError, "This method can only be called from the active AsyncioDAT instance");
    //     return nullptr;
    // }

    PyObject* coro = nullptr;
    if (!PyArg_ParseTuple(args, "O", &coro)) {
        PyErr_SetString(PyExc_TypeError, "Expected a coroutine object");
        return nullptr;
    }
    
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    return g_activeAsyncioInstance->runCoroutine(coro);
}

PyObject* isAsyncioRunning(PyObject* self, PyObject* args)
{
    bool running = g_activeAsyncioInstance && g_activeAsyncioInstance->isAsyncioInitialized();
    return PyBool_FromLong(running ? 1 : 0);
}

PyObject* getCallbackCount(PyObject* self, PyObject* args)
{
    if (g_activeAsyncioInstance) {
        int count = g_activeAsyncioInstance->getReadyCallbackCount();
        return PyLong_FromLong(count);
    }
    return PyLong_FromLong(-1);
}

PyObject* getLoopRunning(PyObject* self, void* closure)
{
    bool running = g_activeAsyncioInstance && g_activeAsyncioInstance->isLoopRunning();
    return PyBool_FromLong(running ? 1 : 0);
}

PyObject* getAsyncioInitialized(PyObject* self, void* closure)
{
    bool initialized = g_activeAsyncioInstance && g_activeAsyncioInstance->isAsyncioInitialized();
    return PyBool_FromLong(initialized ? 1 : 0);
}

} // namespace py
