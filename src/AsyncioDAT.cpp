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
	,	m_autoProcessEvents(true)
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
	ginfo->cookEveryFrame = m_autoProcessEvents;
}

void
AsyncioDAT::makeTable(DAT_Output* output, int numRows, int numCols)
{
	// output->setOutputDataType(DAT_OutDataType::Table);
	// output->setTableSize(numRows, numCols);

	// std::array<const char*, 5> data = { "this", "is", "some", "test", "data"};

	// for (int i = 0; i < numRows; i++)
	// {
	// 	for (int j = 0; j < numCols; j++)
	// 	{
	// 		int j2 = j;

	// 		// If we are asked to make more columns than we have data for
	// 		if (j2 >= data.size())
	// 			j2 = j2 % data.size();

	// 		output->setCellString(i, j, data[j2]);
	// 	}
	// }
}

void
AsyncioDAT::makeText(DAT_Output* output)
{
	// output->setOutputDataType(DAT_OutDataType::Text);
	// output->setText("This is some test data.");
}

void
AsyncioDAT::execute(DAT_Output* output,
							const OP_Inputs* inputs,
							void* reserved1)
{
	if (!output)
		return;

	m_executeCount++;

	// Update auto-process setting from parameter
	m_autoProcessEvents = inputs->getParInt("Autoprocess") != 0;

	// Process asyncio events every frame if auto-processing is enabled
	if (m_autoProcessEvents && m_asyncioInitialized) {
		processAsyncioEvents();
	}

	// Create status output as text
	output->setOutputDataType(DAT_OutDataType::Text);
	
	std::string statusText = "AsyncioDAT Status\n";
	statusText += "================\n";
	statusText += "Execute Count: " + std::to_string(m_executeCount) + "\n";
	statusText += "Asyncio Initialized: " + std::string(m_asyncioInitialized ? "Yes" : "No") + "\n";
	statusText += "Auto Process Events: " + std::string(m_autoProcessEvents ? "Yes" : "No") + "\n";
	
	if (py::g_asyncioManager) {
		statusText += "Event Loop Running: " + std::string(py::g_asyncioManager->isRunning() ? "Yes" : "No") + "\n";
	} else {
		statusText += "Event Loop Running: No\n";
	}
	
	statusText += "\nAvailable Methods:\n";
	statusText += "- initialize_asyncio()\n";
	statusText += "- shutdown_asyncio()\n";
	statusText += "- process_events()\n";
	statusText += "- get_event_loop()\n";
	statusText += "- add_task(coroutine)\n";
	statusText += "- create_task(coroutine)\n";
	statusText += "- run_coroutine(coroutine)\n";
	statusText += "- is_running()\n";
	
	output->setText(statusText.c_str());
}

int32_t
AsyncioDAT::getNumInfoCHOPChans(void* reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 4;
}

void
AsyncioDAT::getInfoCHOPChan(int32_t index,
									OP_InfoCHOPChan* chan, void* reserved1)
{

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
	// CHOP
	{
		OP_StringParameter	np;

		np.name = "Chop";
		np.label = "CHOP";

		OP_ParAppendResult res = manager->appendCHOP(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// Number of Rows
	{
		OP_NumericParameter	np;

		np.name = "Rows";
		np.label = "Rows";
		np.defaultValues[0] = 4;
		np.minSliders[0] = 0;
		np.maxSliders[0] = 10;

		OP_ParAppendResult res = manager->appendInt(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// Number of Columns
	{
		OP_NumericParameter	np;

		np.name = "Cols";
		np.label = "Cols";
		np.defaultValues[0] = 5;
		np.minSliders[0] = 0;
		np.maxSliders[0] = 10;

		OP_ParAppendResult res = manager->appendInt(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// DAT output type
	{
		OP_StringParameter	sp;

		sp.name = "Outputtype";
		sp.label = "Output Type";

		sp.defaultValue = "Table";

		const char *names[] = {"Table", "Text"};
		const char *labels[] = {"Table", "Text"};

		OP_ParAppendResult res = manager->appendMenu(sp, 2, names, labels);
		assert(res == OP_ParAppendResult::Success);
	}

	// pulse
	{
		OP_NumericParameter	np;

		np.name = "Reset";
		np.label = "Reset";

		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// Auto process events toggle
	{
		OP_NumericParameter	np;

		np.name = "Autoprocess";
		np.label = "Auto Process Events";
		np.defaultValues[0] = 1.0;

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

}

void
AsyncioDAT::pulsePressed(const char* name, void* reserved1)
{
	if (!strcmp(name, "Reset"))
	{
		// Reset asyncio - shutdown and reinitialize
		shutdownAsyncio();
		initializeAsyncio();
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
			// If processing fails, try to reinitialize
			m_warning = "Failed to process asyncio events, reinitializing...";
			shutdownAsyncio();
			initializeAsyncio();
		}
	}
}
