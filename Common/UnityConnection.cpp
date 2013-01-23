#include "UnityConnection.h"

using namespace std;

UnityConnection::UnityConnection(const string& logPath) 
	: m_Log(logPath.c_str(), ios_base::ate) { }

UnityConnection::~UnityConnection()
{
	delete m_UnityPipe;
}

UnityPipe* UnityConnection::Connect()
{
	m_UnityPipe = new UnityPipe(m_Log);
}

// read a command from stdin
UnityCommand UnityConnection::ReadCommand(vector<string>& args)
{
	args.clear();
    string read;
	Pipe().ReadLine(read);

	if (read.empty())
	{
		// broken pipe -> exit.
		throw CommandException(UCOM_Invalid, "empty command");
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

	return StringToUnityCommand(command.c_str());
}

ofstream& UnityConnection::Log()
{
	return m_Log;
}

UnityPipe& UnityConnection::Pipe()
{
	return *m_UnityPipe;
}
