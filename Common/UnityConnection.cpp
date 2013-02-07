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
UnityCommand UnityConnection::ReadCommand(vector<string>& args)
{
	args.clear();
    string read;
	Pipe().ReadLine(read);

	if (read.empty())
	{
		// broken pipe -> error.
		Enforce<CommandException>(Pipe().IsEOF(), UCOM_Invalid, "empty command");

		Log() << "End of pipe" << Endl;
		return UCOM_Shutdown;
	}
	
	// Skip non-command lines if present
	size_t stopCounter = 1000;
	while (read.substr(0, 2) != "c:" && stopCounter--)
	{
		Pipe().ReadLine(read);
	}

	if (!stopCounter)
		throw CommandException(UCOM_Invalid, "too many bogus lines");
	
	string command = read.substr(2);

	if (Tokenize(args, command) == 0)
		throw CommandException(UCOM_Invalid, 
							   string("invalid formatted - ") + command);

	return StringToUnityCommand(args[0].c_str());
}

unityplugin::LogStream& UnityConnection::Log()
{
	return m_Log;
}

bool UnityConnection::IsConnected() const
{
	return m_UnityPipe;
}

UnityPipe& UnityConnection::Pipe()
{
	return *m_UnityPipe;
}
