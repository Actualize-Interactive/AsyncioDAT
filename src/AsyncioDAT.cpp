#include "AsyncioDAT.h"
#include "py_bindings.h"
#include <iostream>

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
	info->customOPInfo.pythonMethods = py::methods;
	info->customOPInfo.pythonGetSets = py::getSets;
	info->customOPInfo.pythonCallbacksDAT = py::pythonCallbacksDATStubs;

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

AsyncioDAT::AsyncioDAT(const OP_NodeInfo* info) 
	:	m_nodeInfo(info)
	,	m_warning(nullptr)
	,	m_error(nullptr)
	,	m_asyncioInitialized(false)
	,	m_executeCount(0)
	,	m_parAutoProcess(true)
{
	initializeAsyncio();
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
	ginfo->cookEveryFrame = m_parAutoProcess;
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
	if (m_parMaxstatusrows > 0) {
		m_status_messages.push_back(message);
	}
    while (m_status_messages.size() > m_parMaxstatusrows) {
        m_status_messages.pop_front();
    }
}

void
AsyncioDAT::execute(DAT_Output* output,
							const OP_Inputs* inputs,
							void* reserved1)
{
	if (!output)
		return;

	m_executeCount++;
	m_warning = nullptr;
	m_error = nullptr;

	auto parActive = inputs->getParInt("Active") > 0;
	if (parActive != m_parActive) {
		m_parActive = parActive;
		if (m_parActive) {
			initializeAsyncio();
			addStatusMessage("Asyncio event loop started");
		} else {
			shutdownAsyncio();
			addStatusMessage("Asyncio event loop stopped");
		}
	}

	auto parAutoProcess = inputs->getParInt("Autoprocess") > 0;
	if (parAutoProcess != m_parAutoProcess) {
		m_parAutoProcess = parAutoProcess;
		if (m_parAutoProcess) {
			addStatusMessage("Asyncio event loop auto processing started");
		} else {
			addStatusMessage("Asyncio event loop auto processing stopped");
		}
	}
	m_parMaxstatusrows = inputs->getParInt("Maxstatusrows");

	if (m_parActive && m_parAutoProcess && m_asyncioInitialized) {
		processAsyncioEvents();
	}

	makeTable(output);
}

int32_t
AsyncioDAT::getNumInfoCHOPChans(void* reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 1;
}

void
AsyncioDAT::getInfoCHOPChan(int32_t index,
									OP_InfoCHOPChan* chan, void* reserved1)
{
	if (index == 0) {
		chan->name->setString("execute_count");
		chan->value = static_cast<float>(m_executeCount);
	} else {
		chan->name->setString("chan");
		chan->value = 0;
	}

}

bool
AsyncioDAT::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	return false; // We are not returning any Info DAT
}

void
AsyncioDAT::getInfoDATEntries(int32_t index,
								int32_t nEntries,
								OP_InfoDATEntries* entries,
								void* reserved1)
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
		np.defaultValues[0] = 1;

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter	np;
		np.page = "Asyncio";
		np.name = "Reset";
		np.label = "Reset";
		np.defaultValues[0] = 0;

		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter	np;
		np.page = "Asyncio";
		np.name = "Autoprocess";
		np.label = "Auto Process Events";
		np.defaultValues[0] = 1;

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
		shutdownAsyncio();
		initializeAsyncio();
		addStatusMessage("Asyncio event loop reset");
	} else if (!strcmp(name, "Clearstatus")) {
		m_status_messages.clear();
	} else {
		std::cout << "AsyncioDAT: Unknown pulse pressed: " << name << std::endl;
	}
}

void
AsyncioDAT::initializeAsyncio()
{
	if (!m_asyncioInitialized) {
		m_asyncioInitialized = py::ensureAsyncioInitialized();
		if (m_asyncioInitialized) {
			std::cout << "AsyncioDAT: Event loop initialized successfully" << std::endl;
		} else {
			std::cout << "AsyncioDAT: Failed to initialize event loop" << std::endl;
			m_error = "Failed to initialize asyncio event loop";
		}
	}
}

void
AsyncioDAT::shutdownAsyncio()
{
	if (m_asyncioInitialized) {
		py::cleanupAsyncio();
		m_asyncioInitialized = false;
		std::cout << "AsyncioDAT: Event loop shutdown" << std::endl;
	}
}

void
AsyncioDAT::processAsyncioEvents()
{
	if (m_asyncioInitialized && py::g_asyncioManager) {
		if (!py::g_asyncioManager->processEvents()) {
			m_warning = "Failed to process asyncio events, reinitializing...";
			shutdownAsyncio();
			initializeAsyncio();
		}
	}
}
