#include "UnityConnection.h"
#include "Utility.h"

using namespace std;
using namespace unityplugin;

UnityConnection::UnityConnection(const string& logPath) 
	: m_Log(logPath), m_UnityPipe(NULL) { }

UnityConnection::~UnityConnection()
{
	delete m_UnityPipe;
	m_UnityPipe = NULL;
}

UnityPipe* UnityConnection::Connect()
{
	m_UnityPipe = new UnityPipe(m_Log);
	return m_UnityPipe;
}

// read a command from stdin
UnityCommand UnityConnection::ReadCommand(CommandArgs& args)
{
	try
	{
		if (!IsConnected())
			Connect();
	}
	catch (exception& e)
	{
		Log().Notice() << "While reading command: " << e.what() << Endl;
		return UCOM_Invalid;
	}
	
	args.clear();
    string read;
	Pipe().ReadLine(read);

	if (read.empty())
	{
		// broken pipe -> error.
		if (!Pipe().IsEOF())
		{
			Pipe().ErrorLine("Read empty command from connection");
			return UCOM_Invalid;
		}
			
		Log().Info() << "End of pipe" << Endl;
		return UCOM_Shutdown;
	}
	
	// Skip non-command lines if present
	size_t stopCounter = 1000;
	while (read.substr(0, 2) != "c:" && stopCounter--)
	{
		Pipe().ReadLine(read);
	}

	if (!stopCounter)
	{
		Pipe().ErrorLine("Too many bogus lines");
		return UCOM_Invalid;
	}

	string command = read.substr(2);

	if (Tokenize(args, command) == 0)
	{
		Pipe().ErrorLine(string("invalid formatted - '") + command + "'");
		return UCOM_Invalid;
	}
	return StringToUnityCommand(args[0].c_str());
}

unityplugin::LogStream& UnityConnection::Log()
{
	return m_Log;
}

bool UnityConnection::IsConnected() const
{
	return m_UnityPipe != NULL;
}

UnityPipe& UnityConnection::Pipe()
{
	return *m_UnityPipe;
}
