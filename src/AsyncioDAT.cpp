
#include "AsyncioDAT.h"
#include "py_bindings.h"

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
{
}

AsyncioDAT::~AsyncioDAT()
{
}

void
AsyncioDAT::getGeneralInfo(DAT_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	ginfo->cookEveryFrameIfAsked = false;
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
/*
	if (inputs->getNumInputs() > 0)
	{
		inputs->enablePar("Rows", 0);		// not used
		inputs->enablePar("Cols", 0);		// not used
		inputs->enablePar("Outputtype", 0);	// not used

		const OP_DATInput	*cinput = inputs->getInputDAT(0);

		int numRows = cinput->numRows;
		int numCols = cinput->numCols;
		bool isTable = cinput->isTable;

		if (!isTable) // is Text
		{
			const char* str = cinput->getCell(0, 0);
			output->setText(str);
		}
		else
		{
			output->setOutputDataType(DAT_OutDataType::Table);
			output->setTableSize(numRows, numCols);

			for (int i = 0; i < cinput->numRows; i++)
			{
				for (int j = 0; j < cinput->numCols; j++)
				{
					const char* str = cinput->getCell(i, j);
					output->setCellString(i, j, str);
				}
			}
		}

	}
	else // If no input is connected, lets output a custom table/text DAT
	{
		inputs->enablePar("Rows", 1);
		inputs->enablePar("Cols", 1);
		inputs->enablePar("Outputtype", 1);

		int outputDataType = inputs->getParInt("Outputtype");
		int	 numRows = inputs->getParInt("Rows");
		int	 numCols = inputs->getParInt("Cols");

		switch (outputDataType)
		{
			case 0:		// Table
				makeTable(output, numRows, numCols);
				break;

			case 1:		// Text
				makeText(output);
				break;

			default: // table
				makeTable(output, numRows, numCols);
				break;
		}

		// if there is an input chop parameter:
		const OP_CHOPInput	*cinput = inputs->getParCHOP("Chop");
		if (cinput)
		{
			int numSamples = cinput->numSamples;
			int ind = 0;
			for (int i = 0; i < cinput->numChannels; i++)
			{
				myChopChanName = std::string(cinput->getChannelName(i));
				myChop = inputs->getParString("Chop");

				static char tempBuffer[50];
				myChopChanVal = float(cinput->getChannelData(i)[ind]);

#ifdef _WIN32
				sprintf_s(tempBuffer, "%g", myChopChanVal);
#else // macOS
				snprintf(tempBuffer, sizeof(tempBuffer), "%g", myChopChanVal);
#endif
				if (numCols == 0)
					numCols = 2;
				output->setTableSize(numRows + i + 1, numCols);
				output->setCellString(numRows + i, 0, myChopChanName.c_str());
				output->setCellString(numRows + i, 1, &tempBuffer[0]);
			}

		}

	}
*/

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

}

void
AsyncioDAT::pulsePressed(const char* name, void* reserved1)
{
	if (!strcmp(name, "Reset"))
	{
	}
}
