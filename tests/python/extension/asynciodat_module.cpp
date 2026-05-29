// Test-only CPython extension that exposes AsyncioDAT's bindings as an
// importable module, so the asyncio / plugin / config logic can be tested with
// pytest WITHOUT TouchDesigner.
//
// It compiles the real operator sources (asyncio_dat.cpp, py_bindings.cpp,
// config.cpp) and stands in for TouchDesigner with a fake OP_Context (the only
// TD interface the operator actually calls — for lifecycle callbacks). A single
// AsyncioDAT instance is created at import and registered as the active
// instance, which the py_* binding functions forward to.

#include "asyncio_dat.h"
#include "py_bindings.h"

using namespace TD;

namespace {

// Minimal OP_Context. The operator only uses createArgumentsTuple /
// callPythonCallback (for lifecycle callbacks); the rest are unused stubs.
class FakeContext : public OP_Context
{
public:
    PyObject* createArgumentsTuple(int numOtherArgs, void*) override
    {
        // By convention index 0 is 'op'; fill all slots with None.
        const int n = (numOtherArgs > 0 ? numOtherArgs : 0) + 1;
        PyObject* tuple = PyTuple_New(n);
        for (int i = 0; i < n; ++i) {
            Py_INCREF(Py_None);
            PyTuple_SET_ITEM(tuple, i, Py_None);
        }
        return tuple;
    }
    PyObject* callPythonCallback(const char*, PyObject*, PyObject*, void*) override
    {
        Py_RETURN_NONE;
    }
    bool beginCUDAOperations(void*) override { return false; }
    void endCUDAOperations(void*) override {}

protected:
    void* reservedFunc0() override { return nullptr; }
    void* reservedFunc1() override { return nullptr; }
    void* reservedFunc2() override { return nullptr; }
    void* reservedFunc3() override { return nullptr; }
    void* reservedFunc4() override { return nullptr; }
    void* reservedFunc5() override { return nullptr; }
    void* reservedFunc6() override { return nullptr; }
    void* reservedFunc7() override { return nullptr; }
    void* reservedFunc8() override { return nullptr; }
    void* reservedFunc9() override { return nullptr; }
    void* reservedFunc10() override { return nullptr; }
    void* reservedFunc11() override { return nullptr; }
    void* reservedFunc12() override { return nullptr; }
    void* reservedFunc13() override { return nullptr; }
    void* reservedFunc14() override { return nullptr; }
};

FakeContext  g_fakeContext;
OP_NodeInfo  g_fakeNodeInfo{};
AsyncioDAT*  g_testInstance = nullptr;

// Wrappers exposing the getset-style values as plain module functions.
PyObject* tm_plugin_names(PyObject*, PyObject*)        { return py_getPluginNames(nullptr, nullptr); }
PyObject* tm_loop_running(PyObject*, PyObject*)        { return py_getLoopRunning(nullptr, nullptr); }
PyObject* tm_asyncio_initialized(PyObject*, PyObject*) { return py_getAsyncioInitialized(nullptr, nullptr); }

PyMethodDef extra_methods[] = {
    {"plugin_names", tm_plugin_names, METH_NOARGS, "List of registered plugin names"},
    {"loop_running", tm_loop_running, METH_NOARGS, "Whether the event loop is running"},
    {"asyncio_initialized", tm_asyncio_initialized, METH_NOARGS, "Whether asyncio is initialized"},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef asynciodat_module = {
    PyModuleDef_HEAD_INIT,
    "asynciodat",
    "Test-only extension exposing AsyncioDAT's bindings (no TouchDesigner).",
    -1,
    py_methods,        // reuse the operator's binding table verbatim
    nullptr, nullptr, nullptr, nullptr,
};

} // namespace

PyMODINIT_FUNC
PyInit_asynciodat(void)
{
    PyObject* module = PyModule_Create(&asynciodat_module);
    if (!module) {
        return nullptr;
    }
    if (PyModule_AddFunctions(module, extra_methods) != 0) {
        Py_DECREF(module);
        return nullptr;
    }

    // Stand up a single operator instance. Its constructor registers itself as
    // the active instance and initializes the asyncio event loop, which the
    // py_* binding functions then operate on.
    g_fakeNodeInfo.opPath = "/test/asyncio1";
    g_fakeNodeInfo.opId = 1;
    g_fakeNodeInfo.pluginPath = "";
    g_fakeNodeInfo.context = &g_fakeContext;

    if (!g_testInstance) {
        g_testInstance = new AsyncioDAT(&g_fakeNodeInfo);
    }

    return module;
}
