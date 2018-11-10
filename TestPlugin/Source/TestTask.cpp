#include "TestTask.h"
#include "TestCommand.h"
#include "Connection.h"
#include "Dispatch.h"
#include "Commands/TestConfigRequest.h"
#include "Commands/TestChangeDescriptionRequest.h"
#include "Commands/TestDownloadRequest.h"
#include "Commands/TestOutgoingAssetsRequest.h"
#include "Commands/TestIncomingAssetsRequest.h"
#include "Commands/TestSubmitRequest.h"
#include "Commands/TestStatusRequest.h"
#include "Commands/TestAddRequest.h"
#include "Commands/TestCheckoutRequest.h"
#include "Commands/TestDeleteRequest.h"
#include "Commands/TestGetLatestRequest.h"
#include "Commands/TestLockRequest.h"
#include "Commands/TestUnlockRequest.h"
#include "Commands/TestResolveRequest.h"
#include "Commands/TestRevertRequest.h"
#include "Commands/TestMoveRequest.h"
#include "Commands/TestFileModeRequest.h"
#include "Commands/TestMoveChangelistRequest.h"
#include "Commands/TestIncomingRequest.h"
#include "Commands/TestOutgoiingRequest.h"

// This class essentially manages the command line interface to the API and replies.  Commands are read from stdin and results
// written to stdout and errors to stderr.  All text based communications used tags to make the parsing easier on both ends.
TestTask::TestTask()
{
}

TestTask::~TestTask()
{
}

int TestTask::Run()
{
	Connection* connection = new Connection("./Library/testplugin.log");
	try 
	{
		UnityCommand cmd;
		std::vector<std::string> args;
		
		for ( ;; )
		{
			try
			{
				cmd = connection->ReadCommand(args);
				connection->Log().Notice() << "TestTask cmd:'" << UnityCommandToString(cmd) << "'" << Endl;
				
				if (cmd == UCOM_Invalid)
				{
					return 1;
				}
				else if (cmd == UCOM_Shutdown)
				{
					connection->EndResponse();
					return 0;
				}
				else if (!Dispatch<TestCommand>(*connection, *this, cmd, args))
				{
					return 0;
				}
			}
			catch (PluginException& ex)
			{
				connection->Log().Fatal() << "PluginExeption: " << ex.what() << Endl;
				connection->ErrorLine(ex.what());
				connection->EndResponse();
				return 1;
			}
		}
	}
	catch (std::exception& e)
	{
		connection->Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
	}
	return 1;
}
