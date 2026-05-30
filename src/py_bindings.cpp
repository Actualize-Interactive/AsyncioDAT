#include "py_bindings.h"
#include "asyncio_dat.h"

PyMethodDef py_methods[] =
{
    {"initialize_asyncio", py_initializeAsyncio, METH_NOARGS, "Initialize the asyncio event loop"},
    {"shutdown_asyncio", py_shutdownAsyncio, METH_NOARGS, "Shutdown the asyncio event loop"},
    {"poll_event_loop", py_pollEventLoop, METH_NOARGS, "Process asyncio events ready in the event loop"},
    {"get_event_loop", py_getEventLoop, METH_NOARGS, "Get the current event loop"},
    {"add_task", py_addAsyncTask, METH_VARARGS, "Add a coroutine as a task to the event loop"},    
    {"create_task", py_createAsyncTask, METH_VARARGS, "Create a task from a coroutine"},
    {"run_coroutine", py_runCoroutine, METH_VARARGS, "Run a coroutine until complete"},
    {"is_running", py_isAsyncioRunning, METH_NOARGS, "Check if asyncio is running"},
    {"get_callback_count", py_getCallbackCount, METH_NOARGS, "Get number of ready callbacks in the event loop"},
    {"set_plugin", py_setPlugin, METH_VARARGS, "Set a plugin"},
    {"get_plugin", py_getPlugin, METH_VARARGS, "Get a plugin"},
    {"del_plugin", py_delPlugin, METH_VARARGS, "Delete a plugin"},
    {"has_plugin", py_hasPlugin, METH_VARARGS, "Check if a plugin exists"},
    {"clear_plugins", py_clearPlugins, METH_NOARGS, "Clear all plugins"},
    {0}	// Sentinel
};

PyGetSetDef py_getSets[] =
{
    {"loop_running", py_getLoopRunning, nullptr, "Whether the event loop is running", nullptr},
    {"asyncio_initialized", py_getAsyncioInitialized, nullptr, "Whether asyncio is initialized", nullptr},
    {"plugin_names", py_getPluginNames, nullptr, "Get names of all plugins", nullptr},
    {"plugins", py_getPlugins, nullptr, "Plugin accessor", nullptr},
    {0}	// Sentinel
};

const char* py_callbacksDATStubs =
R"(# AsyncioDAT Python Callbacks

def on_initialized(asyncio_dat):
    """Called after the asyncio is initialized."""
    pass

def on_poll_begin(asyncio_dat):
    """Called at the beginning of polling the event loop."""
    pass

def on_poll_end(asyncio_dat):
    """Called at the end of polling the event loop."""
    pass

def on_shutdown_begin(asyncio_dat):
    """Called when the asyncio is shutting down."""
    pass

def on_shutdown_complete(asyncio_dat):
    """Called after the asyncio has been shutdown."""
    pass

)";


// Global active instance
AsyncioDAT* g_activeAsyncioInstance = nullptr;

extern "C" {

// Python function implementations that forward to active instance
PyObject* py_initializeAsyncio(PyObject* self, PyObject* args)
{
    if (g_activeAsyncioInstance) {
        auto success = g_activeAsyncioInstance->initializeAsyncio();
        return PyBool_FromLong(success ? 1 : 0);
    }
    PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
    return nullptr;
}

PyObject* py_shutdownAsyncio(PyObject* self, PyObject* args)
{    if (g_activeAsyncioInstance) {
        g_activeAsyncioInstance->shutdownAsyncio();
    }
    Py_RETURN_NONE;
}

PyObject* py_pollEventLoop(PyObject* self, PyObject* args)
{
    if (g_activeAsyncioInstance) {
        bool success = g_activeAsyncioInstance->pollEventLoop();
        return PyBool_FromLong(success ? 1 : 0);
    }
    Py_RETURN_FALSE;
}

PyObject* py_getEventLoop(PyObject* self, PyObject* args)
{
    if (g_activeAsyncioInstance) {
        return g_activeAsyncioInstance->getEventLoop();
    }
    Py_RETURN_NONE;
}

PyObject* py_addAsyncTask(PyObject* self, PyObject* args)
{
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

PyObject* py_createAsyncTask(PyObject* self, PyObject* args)
{
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

PyObject* py_runCoroutine(PyObject* self, PyObject* args)
{
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

PyObject* py_isAsyncioRunning(PyObject* self, PyObject* args)
{
    bool running = g_activeAsyncioInstance && g_activeAsyncioInstance->isAsyncioInitialized();
    return PyBool_FromLong(running ? 1 : 0);
}

PyObject* py_getCallbackCount(PyObject* self, PyObject* args)
{
    if (g_activeAsyncioInstance) {
        int count = g_activeAsyncioInstance->getReadyCallbackCount();
        return PyLong_FromLong(count);
    }
    return PyLong_FromLong(-1);
}

PyObject* py_getLoopRunning(PyObject* self, void* closure)
{
    bool running = g_activeAsyncioInstance && g_activeAsyncioInstance->isLoopRunning();
    return PyBool_FromLong(running ? 1 : 0);
}

PyObject* py_getAsyncioInitialized(PyObject* self, void* closure)
{
    bool initialized = g_activeAsyncioInstance && g_activeAsyncioInstance->isAsyncioInitialized();
    return PyBool_FromLong(initialized ? 1 : 0);
}

PyObject* py_setPlugin(PyObject* self, PyObject* args)
{
    const char* name = nullptr;
    PyObject* value = nullptr;
    
    if (!PyArg_ParseTuple(args, "sO", &name, &value)) {
        PyErr_SetString(PyExc_TypeError, "Expected (string, object)");
        return nullptr;
    }
    
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    bool success = g_activeAsyncioInstance->addPlugin(name, value);
    return PyBool_FromLong(success ? 1 : 0);
}

PyObject* py_getPlugin(PyObject* self, PyObject* args)
{
    const char* name = nullptr;
    
    if (!PyArg_ParseTuple(args, "s", &name)) {
        PyErr_SetString(PyExc_TypeError, "Expected a string");
        return nullptr;
    }
    
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    return g_activeAsyncioInstance->getPlugin(name);
}

PyObject* py_delPlugin(PyObject* self, PyObject* args)
{
    const char* name = nullptr;
    
    if (!PyArg_ParseTuple(args, "s", &name)) {
        PyErr_SetString(PyExc_TypeError, "Expected a string");
        return nullptr;
    }
    
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    bool success = g_activeAsyncioInstance->removePlugin(name);
    return PyBool_FromLong(success ? 1 : 0);
}

PyObject* py_hasPlugin(PyObject* self, PyObject* args)
{
    const char* name = nullptr;
    
    if (!PyArg_ParseTuple(args, "s", &name)) {
        PyErr_SetString(PyExc_TypeError, "Expected a string");
        return nullptr;
    }
    
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    auto exists = g_activeAsyncioInstance->hasPlugin(name);
    return PyBool_FromLong(exists ? 1 : 0);
}

PyObject* py_clearPlugins(PyObject* self, PyObject* args)
{
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    g_activeAsyncioInstance->clearPlugins();
    Py_RETURN_NONE;
}

// Custom plugin attribute accessor class
static PyObject* py_createPluginAttrAccessor(PyObject* self, PyObject* args)
{
    const char* accessorCode = 
R"(
class Plugins:
    def __init__(self, asyncio_dat):
        self._asyncio_dat = asyncio_dat
    
    def __getattr__(self, name):
        obj = self._asyncio_dat.get_plugin(name)
        if obj is None:
            raise AttributeError(f"'{name}' attribute not found")
        return obj
    
    def __setattr__(self, name, value):
        if name.startswith('_'):
            super().__setattr__(name, value)
        else:
            self._asyncio_dat.set_plugin(name, value)
    
    def __delattr__(self, name):
        if not self._asyncio_dat.del_plugin(name):
            raise AttributeError(f"'{name}' attribute not found")
    
    def __contains__(self, name):
        return self._asyncio_dat.has_plugin(name)

    def __dir__(self):
        names = self._asyncio_dat.plugin_names
        return list(names) if names else []

Plugins
)";

    PyObject* globals = PyDict_New();
    PyObject* locals = PyDict_New();

    PyObject* builtins = PyImport_ImportModule("builtins");
    if (builtins) {
        PyDict_SetItemString(globals, "__builtins__", builtins);
        Py_DECREF(builtins);
    }

    PyObject* compiled = Py_CompileString(accessorCode, "<plugin_accessor>", Py_file_input);
    if (compiled) {
        PyObject* result = PyEval_EvalCode(compiled, globals, locals);
        if (result) {
            PyObject* accessorClass = PyDict_GetItemString(locals, "Plugins");
            if (accessorClass) {
                Py_INCREF(accessorClass);
                Py_DECREF(result);
                Py_DECREF(compiled);
                Py_DECREF(globals);
                Py_DECREF(locals);
                return accessorClass;
            }
            Py_DECREF(result);
        }
        Py_DECREF(compiled);
    }
    
    Py_DECREF(globals);
    Py_DECREF(locals);
    PyErr_Clear();
    Py_RETURN_NONE;
}

PyObject* py_getPluginNames (PyObject* self, void* closure)
{
    if (!g_activeAsyncioInstance) {
        return PyList_New(0);
    }
    
    return g_activeAsyncioInstance->getPluginNames();
}

PyObject* py_getPlugins(PyObject* self, void* closure)
{
    if (!g_activeAsyncioInstance) {
        PyErr_SetString(PyExc_RuntimeError, "No active AsyncioDAT instance");
        return nullptr;
    }
    
    static PyObject* accessorClass = nullptr;
    if (!accessorClass) {
        accessorClass = py_createPluginAttrAccessor(self, nullptr);
        if (!accessorClass) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to create plugin attribute accessor");
            return nullptr;
        }
    }

    PyObject* args = PyTuple_Pack(1, self);
    PyObject* accessor = PyObject_CallObject(accessorClass, args);
    Py_DECREF(args);
    if (!accessor) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create plugin attribute accessor instance");
        return nullptr;
    }
    
    return accessor;
}

} // extern "C"