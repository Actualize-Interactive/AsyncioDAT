#include "py_bindings.h"
#include <string>


namespace py
{


PyObject* helloResponse(PyObject* self, PyObject* args)
{
	// get a string and return "Hello <string>!"
	const char* str = nullptr;
	if (!PyArg_ParseTuple(args, "s", &str))
	{
		PyErr_SetString(PyExc_TypeError, "Invalid argument");
		return nullptr;
	}
	std::string result = "Hello ";
	result += str;
	result += "!";
	return PyUnicode_FromString(result.c_str());
}


PyMethodDef methods[] =
{
	{"hello", py::helloResponse, METH_VARARGS, "Say hello"},
	{0}	// Sentinel
};

PyGetSetDef getSets[] =
{
	{0}	// Sentinel
};

const char* pythonCallbacksDATStubs =
"# This is an example/empty callbacks DAT.\n"
"\n"
"\n";


} // namespace py