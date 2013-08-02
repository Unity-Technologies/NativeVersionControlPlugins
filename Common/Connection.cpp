#include "Connection.h"
#include "Utility.h"
#include "FileSystem.h"

using namespace std;

const char* DATA_PREFIX = "o";
const char* VERBOSE_PREFIX = "v";
const char* ERROR_PREFIX = "e";
const char* WARNING_PREFIX = "w";
const char* INFO_PREFIX = "i";
const char* COMMAND_PREFIX = "c";
const char* PROGRESS_PREFIX = "p";

const size_t MAX_LOG_FILE_SIZE = 2000000; 

Connection::Connection(const string& logPath) 
	: m_Log(NULL), m_Pipe(NULL) 
{ 
	// Rotate log file if too large
	if (PathExists(logPath) && GetFileLength(logPath) > MAX_LOG_FILE_SIZE)
	{
		string prevPath("prev");
		prevPath += logPath;
		if (PathExists(prevPath))
			DeleteRecursive(prevPath);
		if (PathExists(logPath))
			MoveAFile(logPath, prevPath);
	}
	m_Log = new LogStream(logPath);
}

Connection::~Connection()
{
	delete m_Pipe;
	m_Pipe = NULL;
}

void Connection::Connect()
{
	m_Pipe = new Pipe();
}

// read a command from stdin
UnityCommand Connection::ReadCommand(CommandArgs& args)
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
	ReadLine(read);

	if (read.empty())
	{
		// broken pipe -> error.
		if (!m_Pipe->IsEOF())
		{
			ErrorLine("Read empty command from connection");
			return UCOM_Invalid;
		}
			
		Log().Info() << "End of pipe" << Endl;
		return UCOM_Shutdown;
	}
	
	// Skip non-command lines if present
	size_t stopCounter = 1000;
	while (read.substr(0, 2) != "c:" && stopCounter--)
	{
		ReadLine(read);
	}

	if (!stopCounter)
	{
		ErrorLine("Too many bogus lines");
		return UCOM_Invalid;
	}

	string command = read.substr(2);

	if (Tokenize(args, command) == 0)
	{
		ErrorLine(string("invalid formatted - '") + command + "'");
		return UCOM_Invalid;
	}
	return StringToUnityCommand(args[0].c_str());
}

LogStream& Connection::Log()
{
	return *m_Log;
}

bool Connection::IsConnected() const
{
	return m_Pipe != NULL;
}

/*
Pipe& Connection::GetPipe()
{
	return *m_Pipe;
}
*/

void Connection::Flush()
{
	m_Log->Flush();
	m_Pipe->Flush();
}

Connection& Connection::BeginList()
{
	// Start sending list of unknown size
	DataLine("-1");
	return *this;
}

Connection& Connection::EndList()
{
	// d is list delimiter
	WriteLine("d1:end of list", m_Log->Debug());
	return *this;
}

Connection& Connection::EndResponse()
{
	WriteLine("r1:end of response", m_Log->Debug());
	m_Log->Debug() << "\n--------------------------\n";
	m_Log->Flush();
	return *this;
}

Connection& Connection::Command(const std::string& cmd, MessageArea ma)
{
	WritePrefixLine(COMMAND_PREFIX, ma, cmd, m_Log->Debug());
	return *this;
}


static void DecodeString(string& target)
{
	std::string::size_type len = target.length();
	std::string::size_type n1 = 0;
	std::string::size_type n2 = 0;
	
	while ( n1 < len && (n2 = target.find('\\', n1)) != std::string::npos &&
			n2+1 < len )
	{
		char c = target[n2+1];
		if ( c == '\\' )
		{
			target.replace(n2, 2, "\\");
			len--;
		}
		else if ( c == 'n')
		{
			target.replace(n2, 2, "\n");
			len--;
		}
		n1 = n2 + 1;
	}
}

std::string& Connection::ReadLine(std::string& target)
{
	m_Pipe->ReadLine(target);
	DecodeString(target);
	if (StartsWith(target, "c:pluginConfig vcPerforcePassword"))
		m_Log->Debug() << "UNITY > [password data stripped]" << Endl;
	else
		m_Log->Debug() << "UNITY > " << target << Endl;
	return target;
}

std::string& Connection::PeekLine(std::string& target)
{
	m_Pipe->PeekLine(target);
	DecodeString(target);
	return target;
}

// Params: -1 means not specified
Connection& Connection::Progress(int pct, time_t timeSoFar, const std::string& message, MessageArea ma)
{
	string msg = IntToString(pct) + " " + IntToString((int)timeSoFar) + " " + message;
	WritePrefixLine(PROGRESS_PREFIX, ma, msg, m_Log->Notice());
	return *this;
}

Connection& Connection::operator<<(const std::vector<std::string>& v)
{
	DataLine(v.size());
	for (std::vector<std::string>::const_iterator i = v.begin(); i != v.end(); ++i)
		WriteLine(*i, m_Log->Debug());
	return *this;
}

Connection& Connection::WritePrefix(const char* prefix, MessageArea ma, LogWriter& log)
{
	Write(prefix, log);
	Write(ma, log);
	Write(":", log);
	return *this;
}


// Encode newlines in strings
Connection& Connection::Write(const std::string& v, LogWriter& log)
{
	std::string tmp = Replace(v, "\\", "\\\\");
	tmp = Replace(tmp, "\n", "\\n");
	log << tmp;
	m_Pipe->Write(tmp);
	return *this;
}

// Encode newlines in strings
Connection& Connection::Write(const char* v, LogWriter& log)
{
	return Write(std::string(v), log);
}

Connection& Connection::WriteEndl(LogWriter& log)
{
	log << "\n";
	m_Pipe->Write("\n");
	return *this;
}

