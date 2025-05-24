#include "py_bindings.h"
#include <string>
#include <iostream>
#include <iostream>

namespace py
{

// Global asyncio manager instance
AsyncioManager* g_asyncioManager = nullptr;

// AsyncioManager Implementation
AsyncioManager::AsyncioManager()
    : m_asyncioModule(nullptr)
    , m_eventLoop(nullptr)
    , m_newEventLoop(nullptr)
    , m_setEventLoop(nullptr)
    , m_getEventLoop(nullptr)
    , m_runUntilComplete(nullptr)
    , m_createTask(nullptr)
    , m_sleep(nullptr)
    , m_callSoon(nullptr)
    , m_stop(nullptr)
    , m_runForever(nullptr)
    , m_loopRunning(false)
    , m_initialized(false)
{
}

AsyncioManager::~AsyncioManager()
{
    shutdown();
}

bool AsyncioManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    // Import asyncio module
    m_asyncioModule = PyImport_ImportModule("asyncio");
    if (!m_asyncioModule) {
        PyErr_Print();
        return false;
    }

    // Get asyncio functions
    m_newEventLoop = PyObject_GetAttrString(m_asyncioModule, "new_event_loop");
    m_setEventLoop = PyObject_GetAttrString(m_asyncioModule, "set_event_loop");
    m_getEventLoop = PyObject_GetAttrString(m_asyncioModule, "get_event_loop");
    m_createTask = PyObject_GetAttrString(m_asyncioModule, "create_task");
    m_sleep = PyObject_GetAttrString(m_asyncioModule, "sleep");

    if (!m_newEventLoop || !m_setEventLoop || !m_getEventLoop || !m_createTask || !m_sleep) {
        PyErr_Print();
        shutdown();
        return false;
    }

    // Create new event loop
    PyObject* args = PyTuple_New(0);
    m_eventLoop = PyObject_CallObject(m_newEventLoop, args);
    Py_DECREF(args);

    if (!m_eventLoop) {
        PyErr_Print();
        shutdown();
        return false;
    }

    // Get loop methods
    m_runUntilComplete = PyObject_GetAttrString(m_eventLoop, "run_until_complete");
    m_callSoon = PyObject_GetAttrString(m_eventLoop, "call_soon");
    m_stop = PyObject_GetAttrString(m_eventLoop, "stop");
    m_runForever = PyObject_GetAttrString(m_eventLoop, "run_forever");

    if (!m_runUntilComplete || !m_callSoon || !m_stop || !m_runForever) {
        PyErr_Print();
        shutdown();
        return false;
    }

    // Set as current event loop
    PyObject* setArgs = PyTuple_Pack(1, m_eventLoop);
    PyObject* result = PyObject_CallObject(m_setEventLoop, setArgs);
    Py_DECREF(setArgs);
    
    if (!result) {
        PyErr_Print();
        shutdown();
        return false;
    }
    Py_DECREF(result);

    m_initialized = true;
    m_loopRunning = true;  // Mark as running since we've set it as the current loop
    std::cout << "Asyncio event loop initialized successfully" << std::endl;
    return true;
}

void AsyncioManager::shutdown()
{
    if (m_loopRunning && m_eventLoop && m_stop) {
        // Stop the loop if it's running
        PyObject* args = PyTuple_New(0);
        PyObject* result = PyObject_CallObject(m_stop, args);
        Py_DECREF(args);
        if (result) {
            Py_DECREF(result);
        }
        m_loopRunning = false;
    }

    // Clean up Python objects
    Py_XDECREF(m_runForever);
    Py_XDECREF(m_stop);
    Py_XDECREF(m_callSoon);
    Py_XDECREF(m_runUntilComplete);
    Py_XDECREF(m_sleep);
    Py_XDECREF(m_createTask);
    Py_XDECREF(m_getEventLoop);
    Py_XDECREF(m_setEventLoop);
    Py_XDECREF(m_newEventLoop);
    Py_XDECREF(m_eventLoop);
    Py_XDECREF(m_asyncioModule);

    m_runForever = nullptr;
    m_stop = nullptr;
    m_callSoon = nullptr;
    m_runUntilComplete = nullptr;
    m_sleep = nullptr;
    m_createTask = nullptr;
    m_getEventLoop = nullptr;
    m_setEventLoop = nullptr;
    m_newEventLoop = nullptr;
    m_eventLoop = nullptr;
    m_asyncioModule = nullptr;

    m_initialized = false;
    m_loopRunning = false;
}

bool AsyncioManager::processEvents()
{
    if (!m_initialized || !m_eventLoop) {
        return false;
    }

    try {
        // Implement the exact pattern from the working Python code:
        // 1. event_loop.call_soon(event_loop.stop)
        // 2. event_loop.run_forever()  
        // 3. event_loop.run_until_complete(asyncio.sleep(0))
        
        // Step 1: Schedule the loop to stop after processing ready tasks
        // call_soon(event_loop.stop)
        PyObject* stopArgs = PyTuple_Pack(1, m_stop);
        PyObject* callSoonResult = PyObject_CallObject(m_callSoon, stopArgs);
        Py_DECREF(stopArgs);
        
        if (!callSoonResult) {
            PyErr_Clear();
            return false;
        }
        Py_DECREF(callSoonResult);
        
        // Step 2: Run the event loop - it will process ready tasks then stop
        // event_loop.run_forever()
        PyObject* runForeverResult = PyObject_CallObject(m_runForever, nullptr);
        if (!runForeverResult) {
            PyErr_Clear();
            return false;
        }
        Py_DECREF(runForeverResult);
        
        // Step 3: Process any remaining tasks with sleep(0)
        // event_loop.run_until_complete(asyncio.sleep(0))
        PyObject* sleepArgs = PyTuple_Pack(1, PyFloat_FromDouble(0.0));
        PyObject* sleepCoro = PyObject_CallObject(m_sleep, sleepArgs);
        Py_DECREF(sleepArgs);
        
        if (!sleepCoro) {
            PyErr_Clear();
            return false;
        }
        
        PyObject* runCompleteArgs = PyTuple_Pack(1, sleepCoro);
        PyObject* runCompleteResult = PyObject_CallObject(m_runUntilComplete, runCompleteArgs);
        Py_DECREF(runCompleteArgs);
        Py_DECREF(sleepCoro);
        
        if (!runCompleteResult) {
            PyErr_Clear();
            return false;
        }
        Py_DECREF(runCompleteResult);
        
        return true;
    }
    catch (...) {
        PyErr_Clear();
        return false;
    }
}

PyObject* AsyncioManager::getLoop()
{
    if (!m_initialized || !m_eventLoop) {
        Py_RETURN_NONE;
    }
    
    Py_INCREF(m_eventLoop);
    return m_eventLoop;
}

bool AsyncioManager::addTask(PyObject* coro)
{
    if (!m_initialized || !m_eventLoop || !coro) {
        return false;
    }

    try {
        // Use asyncio.ensure_future() which works with non-running loops
        PyObject* ensureFuture = PyObject_GetAttrString(m_asyncioModule, "ensure_future");
        if (!ensureFuture) {
            PyErr_Print();
            return false;
        }

        // Call ensure_future(coro, loop=our_loop)
        PyObject* kwargs = PyDict_New();
        PyDict_SetItemString(kwargs, "loop", m_eventLoop);
        
        PyObject* args = PyTuple_Pack(1, coro);
        PyObject* task = PyObject_Call(ensureFuture, args, kwargs);
        
        Py_DECREF(args);
        Py_DECREF(kwargs);
        Py_DECREF(ensureFuture);

        if (!task) {
            PyErr_Print();
            return false;
        }

        Py_DECREF(task); // Task is now managed by the loop        std::cout << "Task added successfully using ensure_future" << std::endl;
        return true;
    }
    catch (...) {
        return false;
    }
}

int AsyncioManager::getReadyCallbackCount()
{
    if (!m_initialized || !m_eventLoop) {
        return -1;
    }

    try {
        PyObject* readyAttr = PyObject_GetAttrString(m_eventLoop, "_ready");
        if (readyAttr) {
            PyObject* lenMethod = PyObject_GetAttrString(readyAttr, "__len__");
            if (lenMethod) {
                PyObject* lenResult = PyObject_CallObject(lenMethod, nullptr);
                if (lenResult && PyLong_Check(lenResult)) {
                    long count = PyLong_AsLong(lenResult);
                    Py_DECREF(lenResult);
                    Py_DECREF(lenMethod);
                    Py_DECREF(readyAttr);
                    return (int)count;
                }
                Py_XDECREF(lenResult);
                Py_DECREF(lenMethod);
            }
            Py_DECREF(readyAttr);
        }
        PyErr_Clear();
        return 0;
    }
    catch (...) {
        PyErr_Clear();
        return -1;
    }
}

// Python C API Method Implementations

PyObject* helloResponse(PyObject* self, PyObject* args)
{
    const char* str = nullptr;
    if (!PyArg_ParseTuple(args, "s", &str)) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument");
        return nullptr;
    }
    std::string result = "Hello ";
    result += str;
    result += "!";
    return PyUnicode_FromString(result.c_str());
}

PyObject* initializeAsyncio(PyObject* self, PyObject* args)
{
    if (!ensureAsyncioInitialized()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize asyncio");
        return nullptr;
    }
    
    Py_RETURN_TRUE;
}

PyObject* shutdownAsyncio(PyObject* self, PyObject* args)
{
    if (g_asyncioManager) {
        g_asyncioManager->shutdown();
    }
    Py_RETURN_NONE;
}

PyObject* processAsyncioEvents(PyObject* self, PyObject* args)
{
    if (!ensureAsyncioInitialized()) {
        Py_RETURN_FALSE;
    }
    
    bool success = g_asyncioManager->processEvents();
    return PyBool_FromLong(success ? 1 : 0);
}

PyObject* getEventLoop(PyObject* self, PyObject* args)
{
    if (!ensureAsyncioInitialized()) {
        Py_RETURN_NONE;
    }
    
    return g_asyncioManager->getLoop();
}

PyObject* addAsyncTask(PyObject* self, PyObject* args)
{
    PyObject* coro = nullptr;
    if (!PyArg_ParseTuple(args, "O", &coro)) {
        PyErr_SetString(PyExc_TypeError, "Expected a coroutine object");
        return nullptr;
    }
    
    if (!ensureAsyncioInitialized()) {
        PyErr_SetString(PyExc_RuntimeError, "Asyncio not initialized");
        return nullptr;
    }
    
    bool success = g_asyncioManager->addTask(coro);
    return PyBool_FromLong(success ? 1 : 0);
}

PyObject* createAsyncTask(PyObject* self, PyObject* args)
{
    PyObject* coro = nullptr;
    if (!PyArg_ParseTuple(args, "O", &coro)) {
        PyErr_SetString(PyExc_TypeError, "Expected a coroutine object");
        return nullptr;
    }
    
    if (!ensureAsyncioInitialized()) {
        PyErr_SetString(PyExc_RuntimeError, "Asyncio not initialized");
        return nullptr;
    }
    
    PyObject* loop = g_asyncioManager->getLoop();
    if (!loop || loop == Py_None) {
        Py_XDECREF(loop);
        PyErr_SetString(PyExc_RuntimeError, "No event loop available");
        return nullptr;
    }
    
    PyObject* createTask = PyObject_GetAttrString(loop, "create_task");
    Py_DECREF(loop);
    
    if (!createTask) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get create_task method");
        return nullptr;
    }
    
    PyObject* taskArgs = PyTuple_Pack(1, coro);
    PyObject* task = PyObject_CallObject(createTask, taskArgs);
    Py_DECREF(taskArgs);
    Py_DECREF(createTask);
    
    return task; // Caller owns the reference
}

PyObject* runCoroutine(PyObject* self, PyObject* args)
{
    PyObject* coro = nullptr;
    if (!PyArg_ParseTuple(args, "O", &coro)) {
        PyErr_SetString(PyExc_TypeError, "Expected a coroutine object");
        return nullptr;
    }
    
    if (!ensureAsyncioInitialized()) {
        PyErr_SetString(PyExc_RuntimeError, "Asyncio not initialized");
        return nullptr;
    }
    
    PyObject* loop = g_asyncioManager->getLoop();
    if (!loop || loop == Py_None) {
        Py_XDECREF(loop);
        PyErr_SetString(PyExc_RuntimeError, "No event loop available");
        return nullptr;
    }
    
    PyObject* runUntilComplete = PyObject_GetAttrString(loop, "run_until_complete");
    Py_DECREF(loop);
    
    if (!runUntilComplete) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get run_until_complete method");
        return nullptr;
    }
    
    PyObject* runArgs = PyTuple_Pack(1, coro);
    PyObject* result = PyObject_CallObject(runUntilComplete, runArgs);
    Py_DECREF(runArgs);
    Py_DECREF(runUntilComplete);
    
    return result;
}

PyObject* isAsyncioRunning(PyObject* self, PyObject* args)
{
    bool running = g_asyncioManager && g_asyncioManager->isRunning();
    return PyBool_FromLong(running ? 1 : 0);
}

PyObject* getCallbackCount(PyObject* self, PyObject* args)
{
    if (!ensureAsyncioInitialized()) {
        return PyLong_FromLong(-1);
    }
    
    int count = g_asyncioManager->getReadyCallbackCount();
    return PyLong_FromLong(count);
}

// Property getters
PyObject* getLoopRunning(PyObject* self, void* closure)
{
    bool running = g_asyncioManager && g_asyncioManager->isRunning();
    return PyBool_FromLong(running ? 1 : 0);
}

PyObject* getAsyncioInitialized(PyObject* self, void* closure)
{
    bool initialized = g_asyncioManager != nullptr;
    return PyBool_FromLong(initialized ? 1 : 0);
}

// Utility functions
bool ensureAsyncioInitialized()
{
    if (!g_asyncioManager) {
        g_asyncioManager = new AsyncioManager();
    }
    
    if (!g_asyncioManager->isRunning()) {
        return g_asyncioManager->initialize();
    }
    
    return true;
}

void cleanupAsyncio()
{
    if (g_asyncioManager) {
        delete g_asyncioManager;
        g_asyncioManager = nullptr;
    }
}

// Method definitions for TouchDesigner
PyMethodDef methods[] =
{
    {"hello", py::helloResponse, METH_VARARGS, "Say hello"},
    {"initialize_asyncio", py::initializeAsyncio, METH_NOARGS, "Initialize the asyncio event loop"},
    {"shutdown_asyncio", py::shutdownAsyncio, METH_NOARGS, "Shutdown the asyncio event loop"},
    {"process_events", py::processAsyncioEvents, METH_NOARGS, "Process asyncio events for one frame"},
    {"get_event_loop", py::getEventLoop, METH_NOARGS, "Get the current event loop"},
    {"add_task", py::addAsyncTask, METH_VARARGS, "Add a coroutine as a task to the event loop"},    {"create_task", py::createAsyncTask, METH_VARARGS, "Create a task from a coroutine"},
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
"# AsyncioDAT Python Callbacks and Examples\n"
"# This DAT provides access to asyncio functionality within TouchDesigner\n"
"\n"
"import asyncio\n"
"import time\n"
"\n"
"# Example async function that can be run in the managed event loop\n"
"async def example_async_task(name, duration=1.0):\n"
"    \"\"\"Example async task that sleeps for a duration\"\"\"\n"
"    print(f'Starting task: {name}')\n"
"    await asyncio.sleep(duration)\n"
"    print(f'Completed task: {name}')\n"
"    return f'Result from {name}'\n"
"\n"
"# Example of how to add a task to the managed loop\n"
"def add_example_task():\n"
"    \"\"\"Add an example task to the asyncio loop\"\"\"\n"
"    coro = example_async_task('example', 2.0)\n"
"    return op('asynciodat1').add_task(coro)\n"
"\n"
"# Example async function for network requests\n"
"async def fetch_data(url):\n"
"    \"\"\"Example async function for HTTP requests\"\"\"\n"
"    import aiohttp\n"
"    async with aiohttp.ClientSession() as session:\n"
"        async with session.get(url) as response:\n"
"            return await response.text()\n"
"\n"
"# Example periodic task\n"
"async def periodic_task(interval=1.0):\n"
"    \"\"\"Example periodic task\"\"\"\n"
"    count = 0\n"
"    while True:\n"
"        print(f'Periodic task tick: {count}')\n"
"        count += 1\n"
"        await asyncio.sleep(interval)\n"
"        if count > 10:  # Stop after 10 iterations\n"
"            break\n"
"\n"
"# Convenience functions for common operations\n"
"def initialize():\n"
"    \"\"\"Initialize the asyncio event loop\"\"\"\n"
"    return op('asynciodat1').initialize_asyncio()\n"
"\n"
"def add_periodic_task(interval=1.0):\n"
"    \"\"\"Add a periodic task to the event loop\"\"\"\n"
"    coro = periodic_task(interval)\n"
"    return op('asynciodat1').add_task(coro)\n"
"\n"
"def get_loop():\n"
"    \"\"\"Get the current event loop\"\"\"\n"
"    return op('asynciodat1').get_event_loop()\n"
"\n"
"def is_running():\n"
"    \"\"\"Check if asyncio is running\"\"\"\n"
"    return op('asynciodat1').is_running()\n"
"\n"
"# Called when the DAT is first created\n"
"def onStart():\n"
"    \"\"\"Called when the operator starts\"\"\"\n"
"    print('AsyncioDAT starting...')\n"
"    initialize()\n"
"\n"
"# Called when the DAT is destroyed\n"
"def onDestroy():\n"
"    \"\"\"Called when the operator is destroyed\"\"\"\n"
"    print('AsyncioDAT shutting down...')\n"
"    op('asynciodat1').shutdown_asyncio()\n"
"\n";

} // namespace py