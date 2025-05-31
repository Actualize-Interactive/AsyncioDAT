#include "asyncio_dat.h"
#include "py_bindings.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <sstream>



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
	info->customOPInfo.opType->setString("Asyncio");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("Asyncio");

	// Will be turned into a 3 letter icon on the nodes
	info->customOPInfo.opIcon->setString("AIO");

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
	, m_autoPoll(true)
	, m_maxstatusrows(10)
	, m_add_stop_task(false)
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
	, m_plugins(nullptr)
	, m_loopRunning(false)
	, m_pollEventLoopCount(0)
	, m_pollEventLoopDuration(0.0)
{
	prependPath("prepend_to_path.txt");
	if (tryBecomeActiveInstance()) {
		initializeAsyncio();
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
	ginfo->cookEveryFrame = m_autoPoll;
}

void
AsyncioDAT::makeTable(DAT_Output* output)
{
	output->setOutputDataType(DAT_OutDataType::Table);
	output->setTableSize(static_cast<int32_t>(m_status_messages.size()), 1);

	for (int32_t i = 0; i < m_status_messages.size(); i++) {
		output->setCellString(static_cast<int32_t>(m_status_messages.size() - i - 1), 0, m_status_messages[i].c_str());
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
	std::cout << "AsyncioDAT Status: " << message << std::endl;
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
	m_maxstatusrows = inputs->getParInt("Maxstatusrows");

	auto active = static_cast<bool>(inputs->getParInt("Active"));

	if (active && !m_asyncioInitialized) {
		if (!initializeAsyncio()) {
			if (m_status_messages.empty() || strcmp(m_warning, m_status_messages.back().c_str()) != 0) {
				addStatusMessage(m_warning);
			}
		}
	} else if (!active && m_asyncioInitialized) {
		shutdownAsyncio();
	} 
	
	auto autoPoll = static_cast<bool>(inputs->getParInt("Autopoll"));
	if (m_autoPoll != autoPoll) {
		m_autoPoll = autoPoll;
		addStatusMessage("Auto process set to " + std::string(m_autoPoll ? "true" : "false"));
	}

	m_add_stop_task = static_cast<bool>(inputs->getParInt("Addstoptask"));

	if (m_autoPoll && m_asyncioInitialized) {
		pollEventLoop();
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
		chan->name->setString("event_loop_auto_poll");
		chan->value = static_cast<float>(m_autoPoll ? 1.0f : 0.0f);
	} else if (index == 2) {
		chan->name->setString("event_loop_poll_count");
		chan->value = static_cast<float>(m_pollEventLoopCount);
	} else {
		chan->name->setString("event_loop_poll_duration");
		chan->value = static_cast<float>(m_pollEventLoopDuration);
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
		np.name = "Autopoll";
		np.label = "Auto Poll";
		np.defaultValues[0] = 1;

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter np;
		np.page = "Asyncio";
		np.name = "Addstoptask";
		np.label = "Add Stop Task";
		np.defaultValues[0] = 0; // Default to not adding stop task

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
		std::ostringstream oss;
		oss << "AsyncioDAT instance " << static_cast<void*>(this) << " is now the active instance.";
		addStatusMessage(oss.str());
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
		m_warning = "Cannot initialize: Another AsyncioDAT instance is managing the event loop";
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
	

	// Execute on_create callback from Python file on first startup
	static bool s_first_init = true;
	if (s_first_init) {
		executeOnCreateCallback("on_asyncio_create.py");
		s_first_init = false;
	}


	// Call Python callback
	if (m_nodeInfo) {
		PyObject* callback_args = m_nodeInfo->context->createArgumentsTuple(1, nullptr);
		PyTuple_SET_ITEM(callback_args, 1, PyBool_FromLong(1));

		// This won't work when called from the constructor because the callbacksDAT
		// is not yet initialized... Bypassing since we don't really need to cook before
		// calling the callback.
		if (false) {
			// first argument self
			PyObject* self = PyTuple_GetItem(callback_args, 0);
			if (!self) {
				PyErr_Print();
				m_error = "Failed to get self from callback arguments";
				addStatusMessage(m_error);
				Py_DECREF(callback_args);
				return false;
			}
			Py_INCREF(self);

			PY_Struct* me = (PY_Struct*)self;
			PY_GetInfo info;
			info.autoCook = true;
			me->context->getNodeInstance(info); // force the node to cook
			Py_DECREF(self); // Decrement reference count for self
		}

		result = m_nodeInfo->context->callPythonCallback("on_initialize", callback_args, nullptr, nullptr);
		Py_DECREF(callback_args);
		if (result) {
			Py_DECREF(result);
		}
	}

	addStatusMessage("Asyncio event loop initialized successfully");
	return true;
}

void
AsyncioDAT::shutdownAsyncio()
{
	// Clean up plugins dictionary
   	clearPlugins();
	Py_XDECREF(m_plugins);
	m_plugins = nullptr;

    if (m_loopRunning && m_eventLoop) {
        // Try to schedule stop on the loop instead of calling it directly
        if (m_callSoon && m_stop) {
            PyObject* stopArgs = PyTuple_Pack(1, m_stop);
            PyObject* callSoonResult = PyObject_CallObject(m_callSoon, stopArgs);
            Py_DECREF(stopArgs);
            if (callSoonResult) {
                Py_DECREF(callSoonResult);
            }
            
            // Give the loop a chance to process the stop callback
            if (m_runForever) {
                PyObject* runResult = PyObject_CallObject(m_runForever, nullptr);
                if (runResult) {
                    Py_DECREF(runResult);
                }
            }
        }
        
        // If that didn't work, try the more aggressive approach
        if (m_loopRunning && m_stop) {
            // Set a timeout to prevent indefinite hanging
            PyObject* args = PyTuple_New(0);
            PyObject* result = PyObject_CallObject(m_stop, args);
            Py_DECREF(args);
            if (result) {
                Py_DECREF(result);
            }
        }
        
        m_loopRunning = false;
    }

    // Cancel any pending tasks before cleanup
    if (m_eventLoop) {
        cancelAllTasks();
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
AsyncioDAT::pollEventLoop()
{
	if (!m_asyncioInitialized || !m_eventLoop) {
		return false;
	}

	try {
		auto start = std::chrono::high_resolution_clock::now();

		if (m_add_stop_task)
		{
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
		}
		

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
		
		m_pollEventLoopCount++;
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		m_pollEventLoopDuration = elapsed.count() * 1000.0; // Convert to milliseconds
		return true;
	}
	catch (...) {
		PyErr_Clear();
		return false;
	}
}

void
AsyncioDAT::cancelAllTasks()
{
    if (!m_eventLoop) {
        return;
    }

    try {
        // Get all tasks from the event loop
        PyObject* asyncioModule = PyImport_ImportModule("asyncio");
        if (asyncioModule) {
            PyObject* allTasks = PyObject_GetAttrString(asyncioModule, "all_tasks");
            if (allTasks) {
                PyObject* loopArg = PyTuple_Pack(1, m_eventLoop);
                PyObject* tasks = PyObject_CallObject(allTasks, loopArg);
                Py_DECREF(loopArg);
                
                if (tasks && PySet_Check(tasks)) {
                    PyObject* iterator = PyObject_GetIter(tasks);
                    if (iterator) {
                        PyObject* task;
                        while ((task = PyIter_Next(iterator))) {
                            // Cancel each task
                            PyObject* cancel = PyObject_GetAttrString(task, "cancel");
                            if (cancel) {
                                PyObject* cancelResult = PyObject_CallObject(cancel, nullptr);
                                Py_XDECREF(cancelResult);
                                Py_DECREF(cancel);
                            }
                            Py_DECREF(task);
                        }
                        Py_DECREF(iterator);
                    }
                }
                Py_XDECREF(tasks);
                Py_DECREF(allTasks);
            }
            Py_DECREF(asyncioModule);
        }
        PyErr_Clear(); // Clear any errors from the cancellation process
    }
    catch (...) {
        PyErr_Clear();
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

void
AsyncioDAT::prependPath(const std::string& filepath)
{
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        addStatusMessage("Could not open " + filepath);
        return;
    }

    std::vector<std::string> pathEntries;
    std::string line;
    
    // Read all lines from the file
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));

        // Skip lines that start with '#' or are empty
        if (line.empty() || line[0] == '#') {
            continue;
        }

        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (!line.empty()) {
            pathEntries.push_back(line);
        }
    }
    file.close();

    if (pathEntries.empty()) {
        addStatusMessage("No valid path entries found in prepend_to_path.txt");
        return;
    }

    // Get sys module
    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        addStatusMessage("Failed to import sys module");
        PyErr_Clear();
        return;
    }

    // Get sys.path list
    PyObject* sysPath = PyObject_GetAttrString(sysModule, "path");
    if (!sysPath || !PyList_Check(sysPath)) {
        addStatusMessage("Failed to get sys.path");
        Py_DECREF(sysModule);
        PyErr_Clear();
        return;
    }

    // Prepend each path entry to sys.path if not already present
    for (const auto& entry : pathEntries) {
        PyObject* pathStr = PyUnicode_FromString(entry.c_str());
        if (!pathStr) {
            addStatusMessage("Failed to create string for path: " + entry);
            continue;
        }

        // Check if path already exists in sys.path
        int contains = PySequence_Contains(sysPath, pathStr);
        if (contains == -1) {
            // Error occurred
            addStatusMessage("Error checking if path exists: " + entry);
            Py_DECREF(pathStr);
            PyErr_Clear();
            continue;
        }

        if (contains == 0) {
            // Path doesn't exist, prepend it (insert at index 0)
            if (PyList_Insert(sysPath, 0, pathStr) == 0) {
                addStatusMessage("Prepended to sys.path: " + entry);
            } else {
                addStatusMessage("Failed to prepend to sys.path: " + entry);
                PyErr_Clear();
            }
        } else {
            addStatusMessage("Path already in sys.path: " + entry);
        }

        Py_DECREF(pathStr);
    }

    Py_DECREF(sysPath);
    Py_DECREF(sysModule);
}

bool
AsyncioDAT::executeOnCreateCallback(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) {
		addStatusMessage("Could not find startup Python file: " + filepath + " (this is optional)");
		return false;
	}

	// Read the entire file content
	std::string content((std::istreambuf_iterator<char>(file)),
						std::istreambuf_iterator<char>());
	file.close();

	if (content.empty()) {
		addStatusMessage("Startup Python file is empty: " + filepath);
		return false;
	}

	// Create globals dictionarie
	PyObject* globals = PyDict_New();

	// Add builtins to globals
	PyObject* builtins = PyImport_ImportModule("builtins");
	if (builtins) {
		PyDict_SetItemString(globals, "__builtins__", builtins);
		Py_DECREF(builtins);
	}

	// Add __name__ and __file__ to globals for proper module context
	PyDict_SetItemString(globals, "__name__", PyUnicode_FromString("__main__"));
	PyDict_SetItemString(globals, "__file__", PyUnicode_FromString(filepath.c_str()));

	// Compile and execute the Python code to define functions and imports
	PyObject* compiled = Py_CompileString(content.c_str(), filepath.c_str(), Py_file_input);
	if (!compiled) {
		addStatusMessage("Failed to compile startup Python file: " + filepath);
		PyErr_Print();
		Py_DECREF(globals);
		return false;
	}

	// Execute in globals so module-level imports are available
	PyObject* result = PyEval_EvalCode(compiled, globals, globals);
	Py_DECREF(compiled);

	if (!result) {
		addStatusMessage("Failed to execute startup Python file: " + filepath);
		PyErr_Print();
		Py_DECREF(globals);
		return false;
	}
	Py_DECREF(result);

	// Look for the on_create function in globals (where it was executed)
	PyObject* onCreateFunc = PyDict_GetItemString(globals, "on_create");
	if (!onCreateFunc || !PyCallable_Check(onCreateFunc)) {
		addStatusMessage("No callable 'on_create' function found in: " + filepath);
		Py_DECREF(globals);
		return false;
	}

	// Create a Python wrapper object with plugin methods
	PyObject* wrapper = createAsyncioDATInterface();
	if (!wrapper) {
		addStatusMessage("Failed to create Python wrapper for on_create callback");
		Py_DECREF(globals);
		return false;
	}

	// Call the on_create function with the wrapper object
	PyObject* args = PyTuple_Pack(1, wrapper);
	PyObject* callResult = PyObject_CallObject(onCreateFunc, args);
	Py_DECREF(args);
	Py_DECREF(wrapper);

	if (!callResult) {
		addStatusMessage("Error calling on_create function from: " + filepath);
		PyErr_Print();
		Py_DECREF(globals);
		return false;
	}

	Py_DECREF(callResult);
	Py_DECREF(globals);

	addStatusMessage("Successfully called on_create from: " + filepath);
	return true;
}

PyObject*
AsyncioDAT::createAsyncioDATInterface()
{
	// Create a simple wrapper class that calls the global Python functions
	const char* wrapperCode = 
R"(
class AsyncioDATInterface:
    """Simple wrapper that provides access to AsyncioDAT plugin methods"""
    
    def __init__(self, instance_ptr):
        # Store the C++ instance pointer (not used directly, just for reference)
        self._instance_ptr = instance_ptr
    
    def add_plugin(self, name, obj):
        """Add a plugin to the AsyncioDAT instance"""
        # Call the global Python function that delegates to C++
        # Pass arguments as separate parameters, not as a tuple
        return py_setPlugin(name, obj)
    
    def get_plugin(self, name):
        """Get a plugin from the AsyncioDAT instance"""
        return py_getPlugin(name)
    
    def remove_plugin(self, name):
        """Remove a plugin from the AsyncioDAT instance"""
        return py_delPlugin(name)
    
    def has_plugin(self, name):
        """Check if a plugin exists in the AsyncioDAT instance"""
        return py_hasPlugin(name)
    
    def clear_plugins(self):
        """Clear all plugins from the AsyncioDAT instance"""
        return py_clearPlugins()

    def get_plugin_names(self):
        """Get names of all plugins"""
        names = py_getPluginNames()
        return list(names) if names else []
    
    def get_event_loop(self):
        """Get the asyncio event loop"""
        return py_getEventLoop()

    def add_task(self, coro):
        """Add a coroutine as a task to the event loop"""
        return py_addAsyncTask(coro)

    def create_task(self, coro):
        """Create a task from a coroutine"""
        return py_createAsyncTask(coro)

    def is_running(self):
        """Check if asyncio is running"""
        return py_isAsyncioRunning()

AsyncioDATInterface
)";

	PyObject* globals = PyDict_New();
	PyObject* locals = PyDict_New();

	// Add builtins to globals
	PyObject* builtins = PyImport_ImportModule("builtins");
	if (builtins) {
		PyDict_SetItemString(globals, "__builtins__", builtins);
		Py_DECREF(builtins);
	}

	// Add the Python binding functions to the globals so they can be called
	// Find functions by name from the py_methods array
	for (int i = 0; py_methods[i].ml_name != nullptr; i++) {
		const char* name = py_methods[i].ml_name;
		
		if (strcmp(name, "set_plugin") == 0) {
			PyDict_SetItemString(globals, "py_setPlugin", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "get_plugin") == 0) {
			PyDict_SetItemString(globals, "py_getPlugin", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "del_plugin") == 0) {
			PyDict_SetItemString(globals, "py_delPlugin", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "has_plugin") == 0) {
			PyDict_SetItemString(globals, "py_hasPlugin", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "clear_plugins") == 0) {
			PyDict_SetItemString(globals, "py_clearPlugins", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "get_event_loop") == 0) {
			PyDict_SetItemString(globals, "py_getEventLoop", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "add_task") == 0) {
			PyDict_SetItemString(globals, "py_addAsyncTask", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "create_task") == 0) {
			PyDict_SetItemString(globals, "py_createAsyncTask", PyCFunction_New(&py_methods[i], nullptr));
		} else if (strcmp(name, "is_running") == 0) {
			PyDict_SetItemString(globals, "py_isAsyncioRunning", PyCFunction_New(&py_methods[i], nullptr));
		}
	}

	// Add the getter function for plugin names - create a proper static method def
	static PyMethodDef getterMethodDef = {"py_getPluginNames", (PyCFunction)py_getPluginNames, METH_NOARGS, "Get plugin names"};
	PyDict_SetItemString(globals, "py_getPluginNames", PyCFunction_New(&getterMethodDef, nullptr));
	
	PyObject* compiled = Py_CompileString(wrapperCode, "<asyncio_wrapper>", Py_file_input);
	if (!compiled) {
		addStatusMessage("Failed to compile wrapper code");
		PyErr_Print();
		Py_DECREF(globals);
		Py_DECREF(locals);
		return nullptr;
	}

	PyObject* result = PyEval_EvalCode(compiled, globals, locals);
	Py_DECREF(compiled);

	if (!result) {
		addStatusMessage("Failed to execute wrapper code");
		PyErr_Print();
		Py_DECREF(globals);
		Py_DECREF(locals);
		return nullptr;
	}
	Py_DECREF(result);

	// Get the wrapper class
	PyObject* wrapperClass = PyDict_GetItemString(locals, "AsyncioDATInterface");
	if (!wrapperClass || !PyCallable_Check(wrapperClass)) {
		addStatusMessage("Failed to get AsyncioDATInterface class");
		Py_DECREF(globals);
		Py_DECREF(locals);
		return nullptr;
	}

	// Create the wrapper instance with a dummy pointer
	PyObject* instancePtr = PyLong_FromVoidPtr(this);
	PyObject* args = PyTuple_Pack(1, instancePtr);
	PyObject* wrapper = PyObject_CallObject(wrapperClass, args);
	
	// Clean up
	Py_DECREF(args);
	Py_DECREF(instancePtr);
	Py_DECREF(globals);
	Py_DECREF(locals);

	if (!wrapper) {
		addStatusMessage("Failed to create AsyncioDATInterface instance");
		PyErr_Print();
		return nullptr;
	}

	addStatusMessage("Created wrapper successfully");
	return wrapper;
}