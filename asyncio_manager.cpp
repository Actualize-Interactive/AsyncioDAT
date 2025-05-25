#include "asyncio_manager.h"
#include <string>
#include <iostream>
#include <memory>
#include <chrono>

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
    \"\"\"Called when the AsyncioDAT is initialized.\"\"\"
    pass

def on_pre_shutdown(asyncioDat, success, info):
    \"\"\"Called when the AsyncioDAT is shutdown.\"\"\"
    pass

def on_post_shutdown(asyncioDat, success, info):
    \"\"\"Called after the AsyncioDAT has been shutdown.\"\"\"
    pass

)";



// Singleton manager with reference counting
class AsyncioSingleton {
private:
    static AsyncioManager* s_instance;
    static int s_refCount;

public:
    static AsyncioManager* getInstance() {
        if (!s_instance) {
            s_instance = new AsyncioManager();
        }
        s_refCount++;
        return s_instance;
    }

    static void releaseInstance() {
        s_refCount--;
        if (s_refCount <= 0) {
            if (s_instance) {
                s_instance->shutdown();
                delete s_instance;
                s_instance = nullptr;
            }
            s_refCount = 0;
        }
    }

    static int getRefCount() {
        return s_refCount;
    }
};

// Static member definitions
AsyncioManager* AsyncioSingleton::s_instance = nullptr;
int AsyncioSingleton::s_refCount = 0;
AsyncioManager* g_asyncioManager = nullptr;


void acquireSharedAsyncioManager()
{
    AsyncioSingleton::getInstance();
}

void releaseSharedAsyncioManager()
{
    AsyncioSingleton::releaseInstance();
}

int getAsyncioRefCount()
{
    return AsyncioSingleton::getRefCount();
}


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
    , m_exceptionHandler(nullptr)
{
}

AsyncioManager::~AsyncioManager()
{
    shutdown();
}

bool AsyncioManager::initialize(const TD::OP_NodeInfo* nodeInfo)
{
    if (m_initialized) {
        return true;
    }
    m_nodeInfo = nodeInfo;

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

    if (!createExceptionHandler()) {
        PyErr_Print();
        shutdown();
        return false;
    }

    m_initialized = true;
    m_loopRunning = true;  // Mark as running since we've set it as the current loop

    
    if (m_nodeInfo) {
        // We'll only be adding one extra argument
        PyObject* callback_args = m_nodeInfo->context->createArgumentsTuple(1, nullptr);
        // The first argument is already set to the 'op' variable, so we set the second argument to our speed value
        PyTuple_SET_ITEM(callback_args, 1, PyBool_FromLong(1));

        PyObject *result = m_nodeInfo->context->callPythonCallback("on_initialize", callback_args, nullptr, nullptr);
        // callPythonCallback doesn't take ownership of the args
        Py_DECREF(callback_args);

        // We own result now, so we need to Py_DECREF it unless we want to hold onto it
        if (result)
        {
            Py_DECREF(result);
        }
    }
    
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
    Py_XDECREF(m_exceptionHandler);

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
    m_exceptionHandler = nullptr;

    m_initialized = false;
    m_loopRunning = false;
}

bool AsyncioManager::processEvents()
{
    if (!m_initialized || !m_eventLoop) {
        return false;
    }

    try {
        auto start = std::chrono::high_resolution_clock::now();

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
        m_processEventsCount++;
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        // Calculate elapsed time in seconds
        // Note: This is not the same as the time taken by the event loop
        m_lastProcessEventsTime = elapsed.count();

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

        if (m_exceptionHandler) {
            PyObject* addDoneCallback = PyObject_GetAttrString(task, "add_done_callback");
            if (addDoneCallback) {
                PyObject* callbackArgs = PyTuple_Pack(1, m_exceptionHandler);
                PyObject* callbackResult = PyObject_CallObject(addDoneCallback, callbackArgs);
                Py_DECREF(callbackArgs);
                Py_XDECREF(callbackResult);
                Py_DECREF(addDoneCallback);
            }
        }

        Py_DECREF(task); // Task is now managed by the loop
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

bool AsyncioManager::createExceptionHandler()
{
    // Create a simple exception handler using Python code
    const char* handlerCode = 
        "def _task_done_callback(task):\n"
        "    try:\n"
        "        task.result()\n"
        "    except Exception as e:\n"
        "        print(f'AsyncioDAT task exception: {e}')\n"
        "\n"
        "_task_done_callback\n";
    
    PyObject* globals = PyDict_New();
    PyObject* locals = PyDict_New();
    
    // Add necessary imports to globals
    PyObject* builtins = PyImport_ImportModule("builtins");
    if (builtins) {
        PyDict_SetItemString(globals, "__builtins__", builtins);
        Py_DECREF(builtins);
    }
    
    PyObject* compiled = Py_CompileString(handlerCode, "<exception_handler>", Py_file_input);
    if (compiled) {
        PyObject* result = PyEval_EvalCode(compiled, globals, locals);
        if (result) {
            m_exceptionHandler = PyDict_GetItemString(locals, "_task_done_callback");
            if (m_exceptionHandler) {
                Py_INCREF(m_exceptionHandler);
            }
            Py_DECREF(result);
        }
        Py_DECREF(compiled);
    }
    Py_DECREF(globals);
    Py_DECREF(locals);

    return m_exceptionHandler != nullptr;
}

// Python C API Method Implementations

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
        g_asyncioManager = AsyncioSingleton::getInstance();
    }
    
    if (!g_asyncioManager->isRunning()) {
        return g_asyncioManager->initialize();
    }
    
    return true;
}

void cleanupAsyncio()
{
    if (g_asyncioManager) {
        AsyncioSingleton::releaseInstance();
        g_asyncioManager = nullptr;
    }
}

uint32_t getProcessEventsCount()
{
    if (g_asyncioManager) {
        return g_asyncioManager->getProcessEventsCount();
    }
    return 0;
}

double getLastProcessEventsTime()
{
    if (g_asyncioManager) {
        return g_asyncioManager->getLastProcessEventsTime();
    }
    return 0.0;
}

} // namespace py