#include "AESTask.h"
#include "AESCommand.h"

#if defined(_WINDOWS)
#include "windows.h"
#endif

using namespace std;

AESTask* AESTask::s_Singleton = NULL;

AESTask::AESTask() : m_IsConnected(false), m_URL(""), m_UserName(""), m_Password(""), m_ProjectPath(""), m_Connection(NULL), m_AES(NULL)
{
    s_Singleton = this;
}

AESTask::~AESTask()
{
    Disconnect();
}

int AESTask::Run()
{
    m_Connection = new Connection("./Library/AESplugin.log");
    
	try
	{
		UnityCommand cmd;
		vector<string> args;
		
		for ( ;; )
		{
			cmd = m_Connection->ReadCommand(args);
			
			if (cmd == UCOM_Invalid)
				return 1; // error
			
            if (cmd == UCOM_Shutdown)
			{
				m_Connection->EndResponse();
				return 0; // ok
			}
			
            if (!Dispatch(cmd, args))
				return 1; // error
		}
	}
	catch (exception& e)
	{
		m_Connection->Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
	}
    
	return 1; // error
}

int AESTask::Connect()
{
    if (IsConnected())
        return 0;
    
    m_AES = new AESConnection();
    m_IsConnected = true;
    
    return 0;
}

void AESTask::Disconnect()
{
    if (!IsConnected())
        return;
    
    delete m_AES;
    m_IsConnected = false;
}

bool AESTask::Dispatch(UnityCommand command, const vector<string>& args)
{
    vector<string> arguments = args;
    AESCommand* cmd = NULL;
    
	if (command == UCOM_CustomCommand && arguments.size() > 1)
	{
		cmd = LookupCommand(arguments[1]);
        arguments.erase(arguments.begin());
	}
    else
    {
        cmd = LookupCommand(UnityCommandToString(command));
	}
    
    if (!cmd)
	{
        string msg = "unknown command " + arguments[0];
		throw CommandException(command, msg);
	}
	
	if (!cmd->Run(*this, arguments))
		return false;
    
	return true;
}

