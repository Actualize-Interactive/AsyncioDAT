#include "DAT_CPlusPlusBase.h"
#include <string>
#include <deque>

#ifdef _WIN32
	#include <Python.h>
	#include <structmember.h>
	#include <modsupport.h>
#else
	#include <Python/Python.h>
	#include <Python/structmember.h>
#endif

using namespace TD;

class AsyncioDAT : public DAT_CPlusPlusBase
{
public:
	AsyncioDAT(const OP_NodeInfo* info);
	virtual ~AsyncioDAT();

	virtual void			 getGeneralInfo(DAT_GeneralInfo*, const OP_Inputs*, void* reserved1) override;
	virtual void			 execute(DAT_Output*, const OP_Inputs*, void* reserved) override;
	virtual int32_t			 getNumInfoCHOPChans(void* reserved1) override;
	virtual void			 getInfoCHOPChan(int index, OP_InfoCHOPChan* chan, void* reserved1) override;
	virtual bool			 getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1) override;
	virtual void			 getInfoDATEntries(int32_t index, int32_t nEntries, OP_InfoDATEntries* entries, void* reserved1) override;
	virtual void			 setupParameters(OP_ParameterManager* manager, void* reserved1) override;
	virtual void			 pulsePressed(const char* name, void* reserved1) override;
	virtual void			 getWarningString(OP_String* warning, void* reserved1) override;
	virtual void			 getErrorString(OP_String* error, void* reserved1) override;

	// Public methods for Python integration
	bool					 initializeAsyncio();
	void					 shutdownAsyncio();
	bool					 pollEventLoop();
	void 				   	 cancelAllTasks();
	PyObject*				 getEventLoop();
	bool					 addTask(PyObject* coro);
	PyObject*				 createAsyncTask(PyObject* coro);
	PyObject*				 runCoroutine(PyObject* coro);
	bool					 isAsyncioInitialized() const { return m_asyncioInitialized; }
	bool					 isLoopRunning() const { return m_loopRunning; }
	int						 getReadyCallbackCount();
	uint32_t				 getPollEventLoopCount() const { return m_pollEventLoopCount; }
	double					 getLastPollEventLoopDuration() const { return m_pollEventLoopDuration; }

	bool 					 addPlugin(const char* name, PyObject* obj);
	bool 					 removePlugin(const char* name);
	PyObject* 				 getPlugin(const char* name) const;
	PyObject* 				 getPluginNames() const;
	void					 clearPlugins();
	bool					 hasPlugin(const char* name) const;
	PyObject*				 getPluginsDict() const { return m_plugins; }

	bool					 executeOnCreateCallback(const std::string& filepath);
	PyObject*				 createAsyncioDATInterface();

private:
	const OP_NodeInfo*		 m_nodeInfo;
	const char* 			 m_warning;
	const char* 			 m_error;
	std::deque<std::string>  m_status_messages;

	inline static bool       s_called_on_startup = false;
	bool      				 m_autoPoll;
	bool      				 m_asyncioInitialized;
	int32_t    				 m_maxstatusrows;	
	bool 					 m_add_stop_task;

	PyObject* 				 m_asyncioModule;
	PyObject* 				 m_eventLoop;
	PyObject* 				 m_newEventLoop;
	PyObject* 				 m_setEventLoop;
	PyObject* 				 m_getEventLoop;
	PyObject* 				 m_runUntilComplete;
	PyObject* 				 m_createTask;
	PyObject* 				 m_sleep;
	PyObject* 				 m_callSoon;
	PyObject* 				 m_stop;
	PyObject* 				 m_runForever;
	PyObject* 				 m_exceptionHandler;
	PyObject* 				 m_plugins;

	bool     				 m_loopRunning;
	uint32_t 				 m_pollEventLoopCount;
	double    				 m_pollEventLoopDuration;



	void					 makeTable(DAT_Output* output);
	void					 addStatusMessage(const std::string& message);
	bool					 createExceptionHandler();
	bool					 tryBecomeActiveInstance();
	void					 releaseActiveInstance();

	void					 prependPath(const std::string& filepath);

};

