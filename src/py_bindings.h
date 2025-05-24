#pragma once

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

PyObject* helloResponse(PyObject* self, PyObject* args);


} // namespace py



