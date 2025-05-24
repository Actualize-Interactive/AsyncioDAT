
#include "DAT_CPlusPlusBase.h"
#include <string>
#include <deque>

using namespace TD;

class AsyncioDAT : public DAT_CPlusPlusBase
{
public:
	AsyncioDAT(const OP_NodeInfo* info);
	virtual ~AsyncioDAT();

	virtual void		getGeneralInfo(DAT_GeneralInfo*, const OP_Inputs*, void* reserved1) override;

	virtual void		execute(DAT_Output*,
								const OP_Inputs*,
								void* reserved) override;


	virtual int32_t		getNumInfoCHOPChans(void* reserved1) override;
	virtual void		getInfoCHOPChan(int index,
										OP_InfoCHOPChan* chan, 
										void* reserved1) override;

	virtual bool		getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1) override;
	virtual void		getInfoDATEntries(int32_t index,
											int32_t nEntries,
											OP_InfoDATEntries* entries,
											void* reserved1) override;

	virtual void		setupParameters(OP_ParameterManager* manager, void* reserved1) override;
	virtual void		pulsePressed(const char* name, void* reserved1) override;

private:
	const OP_NodeInfo*		 m_nodeInfo;
	const char* 			 m_warning;
	const char* 			 m_error;
	uint32_t			     m_executeCount;
	std::deque<std::string> m_status_messages;

	bool m_parActive;
	bool m_parAutoProcess;
	bool m_parReset;
	bool m_parClearStatus;
	uint64_t m_parMaxstatusrows;

	bool					 m_asyncioInitialized;



	void				makeTable(DAT_Output* output);
	void				addStatusMessage(const std::string& message);


	void				initializeAsyncio();
	void				shutdownAsyncio();
	void				processAsyncioEvents();

};
