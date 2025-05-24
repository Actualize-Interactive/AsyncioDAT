
#include "DAT_CPlusPlusBase.h"
#include <string>
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
	const OP_NodeInfo*	m_nodeInfo;
	const char* 		m_warning;
	const char* 		m_error;



	void				makeTable(DAT_Output* output, int numRows, int numCols);
	void				makeText(DAT_Output* output);

	// We don't need to store this pointer, but we do for the example.
	// The OP_NodeInfo class store information about the node that's using
	// this instance of the class (like its name).
	// const OP_NodeInfo*	myNodeInfo;

	// In this example this value will be incremented each time the execute()
	// function is called, then passes back to the DAT
	// int32_t				myExecuteCount;

	// double				myOffset;

	// std::string         myChopChanName;
	// float               myChopChanVal;
	// std::string         myChop;

	// std::string         myDat;


};
