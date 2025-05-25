#include "asyncio_dat.h"
#include "py_bindings.h"
#include <iostream>
#include <chrono>

extern "C"
{

DLLEXPORT
void
FillDATPluginInfo(DAT_PluginInfo *info)
{
	// Always return DAT_CPLUSPLUS_API_VERSION in this function.
	info->apiVersion = DATCPlusPlusAPIVersion;

	// The opType is the unique name for this TOP. It must start with a
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Asynciodat");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("Asyncio DAT");

	// Will be turned into a 3 letter icon on the nodes
	info->customOPInfo.opIcon->setString("ASY");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("Keith Lostracco");
	info->customOPInfo.authorEmail->setString("keith@actualize.vision");

	// This DAT works with 0 or 1 inputs
	info->customOPInfo.minInputs = 0;
	info->customOPInfo.maxInputs = 1;

	info->customOPInfo.pythonVersion->setString(PY_VERSION);
	info->customOPInfo.pythonMethods = py_methods;
	info->customOPInfo.pythonGetSets = py_getSets;
	info->customOPInfo.pythonCallbacksDAT = py_callbacksDATStubs;

}

DLLEXPORT
DAT_CPlusPlusBase*
CreateDATInstance(const OP_NodeInfo* info)
{
	return new AsyncioDAT(info);
}

DLLEXPORT
void
DestroyDATInstance(DAT_CPlusPlusBase* instance)
{
	delete (AsyncioDAT*)instance;
}

}; // extern "C"


void setActiveAsyncioInstance(AsyncioDAT* instance)
{
    g_activeAsyncioInstance = instance;
}

AsyncioDAT* getActiveAsyncioInstance()
{
    return g_activeAsyncioInstance;
}

AsyncioDAT::AsyncioDAT(const OP_NodeInfo* info) 
	: m_nodeInfo(info)
	, m_warning(nullptr)
	, m_error(nullptr)
	, m_asyncioInitialized(false)
	, m_autoProcess(true)
	, m_asyncioModule(nullptr)
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
	, m_exceptionHandler(nullptr)
	, m_loopRunning(false)
	, m_processEventsCount(0)
	, m_lastProcessEventsTime(0.0)
	, m_plugins(nullptr)
{
	// Try to become the active instance on creation
	if (tryBecomeActiveInstance()) {
		initializeAsyncio();
	} else {
		addStatusMessage("Another AsyncioDAT instance is managing the event loop. This instance is inactive.");
	}
}

AsyncioDAT::~AsyncioDAT()
{
	shutdownAsyncio();
}

void
AsyncioDAT::getGeneralInfo(DAT_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	// We want to cook every frame to process asyncio events
	ginfo->cookEveryFrameIfAsked = true;
	ginfo->cookEveryFrame = m_autoProcess;
}

void
AsyncioDAT::makeTable(DAT_Output* output)
{
	output->setOutputDataType(DAT_OutDataType::Table);
	output->setTableSize(static_cast<int32_t>(m_status_messages.size()), 1);

	for (int32_t i = 0; i < m_status_messages.size(); i++) {
		output->setCellString(static_cast<int32_t>(m_status_messages.size() - i - 1)
			, 0
			, m_status_messages[i].c_str()
		);
	}
}

void
AsyncioDAT::addStatusMessage(const std::string& message)
{
	if (m_maxstatusrows > 0) {
		m_status_messages.push_back(message);
	}
    while (m_status_messages.size() > m_maxstatusrows) {
        m_status_messages.pop_front();
    }
}

void
AsyncioDAT::execute(DAT_Output* output, const OP_Inputs* inputs, void* reserved1)
{
	if (!output)
		return;

	if (!s_called_on_startup) {
		PyObject* callback_args = m_nodeInfo->context->createArgumentsTuple(1, nullptr);
		PyTuple_SET_ITEM(callback_args, 1, PyBool_FromLong(1));

		PyObject *result = m_nodeInfo->context->callPythonCallback("on_startup", callback_args, nullptr, nullptr);
		Py_DECREF(callback_args);

		if (result) {
			Py_DECREF(result);
		}
		s_called_on_startup = true;
	}

	m_warning = nullptr;
	m_error = nullptr;

	auto active = static_cast<bool>(inputs->getParInt("Active"));

	if (active && !m_asyncioInitialized) {
		if (!initializeAsyncio()) {
			m_error = "Another AsyncioDAT instance is managing the event loop. This instance cannot initialize.";
			return;
		}
	} else if (!active && m_asyncioInitialized) {
		shutdownAsyncio();
	}
	if (!m_asyncioInitialized) {
		output->setOutputDataType(DAT_OutDataType::Text);
		output->setText("Asyncio event loop is not initialized.");
		return;
	}

	m_maxstatusrows = inputs->getParInt("Maxstatusrows");

	if (m_autoProcess && m_asyncioInitialized) {
		processAsyncioEvents();
	}

	makeTable(output);
}

int32_t
AsyncioDAT::getNumInfoCHOPChans(void* reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 4;
}

void
AsyncioDAT::getInfoCHOPChan(int32_t index, OP_InfoCHOPChan* chan, void* reserved1)
{
	if (index == 0) {
		chan->name->setString("event_loop_active");
		chan->value = static_cast<float>(m_asyncioInitialized ? 1.0f : 0.0f);
	} else if (index == 1) {
		chan->name->setString("event_loop_auto_process");
		chan->value = static_cast<float>(m_autoProcess ? 1.0f : 0.0f);
	} else if (index == 2) {
		chan->name->setString("event_loop_process_events_count");
		chan->value = static_cast<float>(m_processEventsCount);
	} else {
		chan->name->setString("event_loop_last_process_time");
		chan->value = static_cast<float>(m_lastProcessEventsTime);
	}
}

bool
AsyncioDAT::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	return false; // We are not returning any Info DAT
}

void
AsyncioDAT::getInfoDATEntries(int32_t index, int32_t nEntries, OP_InfoDATEntries* entries, void* reserved1)
{
}

void
AsyncioDAT::setupParameters(OP_ParameterManager* manager, void* reserved1)
{
	{
		OP_NumericParameter	np;
		np.page = "Asyncio";
		np.name = "Active";
		np.label = "Active";
		np.defaultValues[0] = 1; // Default to active

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter	np;
		np.page = "Asyncio";
		np.name = "Reset";
		np.label = "Reset Event Loop";
		np.defaultValues[0] = 0;

		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter np;
		np.page = "Asyncio";
		np.name = "Autoprocess";
		np.label = "Auto Process";
		np.defaultValues[0] = 1; // Default to enabled

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter	np;
		np.page = "Asyncio";
		np.name = "Maxstatusrows";
		np.label = "Max Status Rows";
		np.defaultValues[0] = 10;
		np.minValues[0] = 0;
		np.maxValues[0] = 10;
		np.clampMins[0] = true;
		np.clampMaxes[0] = false;
		OP_ParAppendResult res = manager->appendInt(np);
		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter	np;
		np.page = "Asyncio";
		np.name = "Clearstatus";
		np.label = "Clear Status";
		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}
}

void
AsyncioDAT::pulsePressed(const char* name, void* reserved1)
{
	if (!strcmp(name, "Reset")) {
		if (getActiveAsyncioInstance() == this) {
			shutdownAsyncio();
			initializeAsyncio();
		} else {
			addStatusMessage("Cannot reset: This instance is not managing the event loop");
		}
	} else if (!strcmp(name, "Clearstatus")) {
		m_status_messages.clear();
	}
}

void
AsyncioDAT::getWarningString(OP_String* warning, void* reserved1)
{
	if (m_warning) {
		warning->setString(m_warning);
	}
}

void
AsyncioDAT::getErrorString(OP_String* error, void* reserved1)
{
	if (m_error) {
		error->setString(m_error);
	}
}

bool
AsyncioDAT::tryBecomeActiveInstance()
{
	if (getActiveAsyncioInstance() == nullptr) {
		setActiveAsyncioInstance(this);
		std::cout << "AsyncioDAT instance is now the active instance." << std::endl;
		return true;
	}
	return getActiveAsyncioInstance() == this;
}

void
AsyncioDAT::releaseActiveInstance()
{
	if (getActiveAsyncioInstance() == this) {
		setActiveAsyncioInstance(nullptr);
	}
}

bool
AsyncioDAT::initializeAsyncio()
{
	if (!tryBecomeActiveInstance()) {
		addStatusMessage("Cannot initialize: Another AsyncioDAT instance is managing the event loop");
		m_error = "Another AsyncioDAT instance is managing the event loop";
		return false;
	}

	if (m_asyncioInitialized) {
		return true;
	}

	// Initialize plugins dictionary only when this instance becomes active
	if (!m_plugins) {
		m_plugins = PyDict_New();
		if (!m_plugins) {
			m_error = "Failed to create plugins dictionary";
			addStatusMessage(m_error);
			return false;
		}
	}

	// Import asyncio module
	m_asyncioModule = PyImport_ImportModule("asyncio");
	if (!m_asyncioModule) {
		PyErr_Print();
		m_error = "Failed to import asyncio module";
		addStatusMessage(m_error);
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
		m_error = "Failed to get asyncio functions";
		addStatusMessage(m_error);
		shutdownAsyncio();
		return false;
	}

	// Create new event loop
	PyObject* args = PyTuple_New(0);
	m_eventLoop = PyObject_CallObject(m_newEventLoop, args);
	Py_DECREF(args);

	if (!m_eventLoop) {
		PyErr_Print();
		m_error = "Failed to create event loop";
		addStatusMessage(m_error);
		shutdownAsyncio();
		return false;
	}

	// Get loop methods
	m_runUntilComplete = PyObject_GetAttrString(m_eventLoop, "run_until_complete");
	m_callSoon = PyObject_GetAttrString(m_eventLoop, "call_soon");
	m_stop = PyObject_GetAttrString(m_eventLoop, "stop");
	m_runForever = PyObject_GetAttrString(m_eventLoop, "run_forever");

	if (!m_runUntilComplete || !m_callSoon || !m_stop || !m_runForever) {
		PyErr_Print();
		m_error = "Failed to get loop methods";
		addStatusMessage(m_error);
		shutdownAsyncio();
		return false;
	}

	// Set as current event loop
	PyObject* setArgs = PyTuple_Pack(1, m_eventLoop);
	PyObject* result = PyObject_CallObject(m_setEventLoop, setArgs);
	Py_DECREF(setArgs);
	
	if (!result) {
		PyErr_Print();
		m_error = "Failed to set event loop";
		addStatusMessage(m_error);
		shutdownAsyncio();
		return false;
	}
	Py_DECREF(result);

	if (!createExceptionHandler()) {
		PyErr_Print();
		m_warning = "Failed to create exception handler";
		addStatusMessage(m_warning);
	}

	m_asyncioInitialized = true;
	m_loopRunning = true;
	
	addStatusMessage("Asyncio event loop initialized successfully");

	// Call Python callback
	if (m_nodeInfo) {
		PyObject* callback_args = m_nodeInfo->context->createArgumentsTuple(1, nullptr);
		PyTuple_SET_ITEM(callback_args, 1, PyBool_FromLong(1));

		result = m_nodeInfo->context->callPythonCallback("on_initialize", callback_args, nullptr, nullptr);
		Py_DECREF(callback_args);

		if (result) {
			Py_DECREF(result);
		}
	}
	std::cout << "Asyncio event loop initialized successfully" << std::endl;



	return true;
}

void
AsyncioDAT::shutdownAsyncio()
{
	// Clean up plugins dictionary
	clearPlugins();
	Py_XDECREF(m_plugins);
	m_plugins = nullptr;

	if (m_loopRunning && m_eventLoop && m_stop) {
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

	if (m_asyncioInitialized) {
		m_asyncioInitialized = false;
		releaseActiveInstance();
		addStatusMessage("Asyncio event loop stopped");
	}
}

bool
AsyncioDAT::processAsyncioEvents()
{
	if (!m_asyncioInitialized || !m_eventLoop) {
		return false;
	}

	try {
		auto start = std::chrono::high_resolution_clock::now();

		// Schedule the loop to stop after processing ready tasks
		PyObject* stopArgs = PyTuple_Pack(1, m_stop);
		PyObject* callSoonResult = PyObject_CallObject(m_callSoon, stopArgs);
		Py_DECREF(stopArgs);
		
		if (!callSoonResult) {
			PyErr_Clear();
			return false;
		}
		Py_DECREF(callSoonResult);
		
		// Run the event loop - it will process ready tasks then stop
		PyObject* runForeverResult = PyObject_CallObject(m_runForever, nullptr);
		if (!runForeverResult) {
			PyErr_Clear();
			return false;
		}
		Py_DECREF(runForeverResult);
		
		// Process any remaining tasks with sleep(0)
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
		m_lastProcessEventsTime = elapsed.count();
		return true;
	}
	catch (...) {
		PyErr_Clear();
		return false;
	}
}

PyObject*
AsyncioDAT::getEventLoop()
{
	if (!m_asyncioInitialized || !m_eventLoop) {
		Py_RETURN_NONE;
	}
	
	Py_INCREF(m_eventLoop);
	return m_eventLoop;
}

bool
AsyncioDAT::addTask(PyObject* coro)
{
	if (!m_asyncioInitialized || !m_eventLoop || !coro) {
		return false;
	}

	try {
		PyObject* ensureFuture = PyObject_GetAttrString(m_asyncioModule, "ensure_future");
		if (!ensureFuture) {
			PyErr_Print();
			return false;
		}

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

		Py_DECREF(task);
		return true;
	}
	catch (...) {
		return false;
	}
}

PyObject*
AsyncioDAT::createAsyncTask(PyObject* coro)
{
	if (!m_asyncioInitialized || !m_eventLoop) {
		PyErr_SetString(PyExc_RuntimeError, "Asyncio not initialized");
		return nullptr;
	}
	
	PyObject* createTask = PyObject_GetAttrString(m_eventLoop, "create_task");
	if (!createTask) {
		PyErr_SetString(PyExc_RuntimeError, "Failed to get create_task method");
		return nullptr;
	}
	
	PyObject* taskArgs = PyTuple_Pack(1, coro);
	PyObject* task = PyObject_CallObject(createTask, taskArgs);
	Py_DECREF(taskArgs);
	Py_DECREF(createTask);
	
	return task;
}

PyObject*
AsyncioDAT::runCoroutine(PyObject* coro)
{
	if (!m_asyncioInitialized || !m_runUntilComplete) {
		PyErr_SetString(PyExc_RuntimeError, "Asyncio not initialized");
		return nullptr;
	}
	
	PyObject* runArgs = PyTuple_Pack(1, coro);
	PyObject* result = PyObject_CallObject(m_runUntilComplete, runArgs);
	Py_DECREF(runArgs);
	
	return result;
}

bool
AsyncioDAT::createExceptionHandler()
{
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

int
AsyncioDAT::getReadyCallbackCount()
{
	if (!m_asyncioInitialized || !m_eventLoop) {
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

bool
AsyncioDAT::addPlugin(const char* name, PyObject* obj)
{
    if (!m_plugins || !name || !obj) {
        return false;
    }
    
    // Validate name is not empty
    if (strlen(name) == 0) {
        addStatusMessage("Plugin name cannot be empty");
        return false;
    }
    
    // Check if name already exists and remove old object if it does
    PyObject* nameKey = PyUnicode_FromString(name);
    if (PyDict_Contains(m_plugins, nameKey)) {
        removePlugin(name);
    }
    Py_DECREF(nameKey);

    // Add the new object (this will increment its reference count)
    int result = PyDict_SetItemString(m_plugins, name, obj);
    
    if (result == 0) {
        addStatusMessage(std::string("Added plugin: ") + name);
        return true;
    } else {
        addStatusMessage(std::string("Failed to add plugin: ") + name);
        return false;
    }
}

bool
AsyncioDAT::removePlugin(const char* name)
{
	if (!m_plugins || !name) {
		return false;
	}

	// Remove the object associated with the name
	int result = PyDict_DelItemString(m_plugins, name);
	
	if (result == 0) {
		addStatusMessage(std::string("Removed plugin: ") + name);
		return true;
	} else {
		addStatusMessage(std::string("Failed to remove plugin: ") + name);
		return false;
	}
}

PyObject*
AsyncioDAT::getPlugin(const char* name) const
{
	if (!m_plugins || !name) {
		Py_RETURN_NONE;
	}

	PyObject* obj = PyDict_GetItemString(m_plugins, name);
	if (obj) {
		Py_INCREF(obj); // Increment reference count before returning
		return obj;
	}
	Py_RETURN_NONE;
}

PyObject*
AsyncioDAT::getPluginNames() const
{
	if (!m_plugins) {
		return PyList_New(0);
	}

	PyObject* names = PyDict_Keys(m_plugins);
	if (names) {
		Py_INCREF(names); // Increment reference count before returning
		return names;
	}
	return PyList_New(0);
}

void
AsyncioDAT::clearPlugins()
{
	if (m_plugins) {
		PyDict_Clear(m_plugins);
		addStatusMessage("Cleared all plugins");
	} else {
		addStatusMessage("No plugins to clear");
	}
}

bool
AsyncioDAT::hasPlugin(const char* name) const
{
	if (!m_plugins || !name) {
		return false;
	}

	PyObject* nameKey = PyUnicode_FromString(name);
	int exists = PyDict_Contains(m_plugins, nameKey);
	Py_DECREF(nameKey);
	
	return exists == 1;
}
