#pragma once
#include <string>
#include <vector>
#include <set>
#include "Log.h"
#include "Pipe.h"
#include "Command.h"

enum MessageArea
{
	MAGeneral = 1,
	MASystem = 2,        // Error on local system e.g. cannot create dir
	MAPlugin = 4,        // Error in protocol while communicating with the plugin executable
	MAConfig = 8,        // Error in configuration e.g. config value not set
	MAConnect = 16,      // Error during connect to remote server
	MAProtocol = 32,     // Error in protocol while communicating with server
	MARemote = 64,       // Error on remote server
	MAInvalid = 128      // Must alway be the last entry
};

extern const char* DATA_PREFIX;
extern const char* VERBOSE_PREFIX;
extern const char* ERROR_PREFIX;
extern const char* WARNING_PREFIX;
extern const char* INFO_PREFIX;
extern const char* COMMAND_PREFIX;
extern const char* PROGRESS_PREFIX;

class Connection
{
public:
	Connection(const std::string& logPath);
	~Connection();

	// Connect to Unity
	void Connect();
	bool IsConnected() const;

	// Read a command from unity
	UnityCommand ReadCommand(CommandArgs& args);

	// Get the log stream
	LogStream& Log();

	// Get the raw pipe to Unity. 
	// Make sure IsConnected() is true before using.
	//	Pipe& GetPipe();

	void Flush();

	Connection& BeginList();
	Connection& EndList();
	Connection& EndResponse();
	Connection& Command(const std::string& cmd, MessageArea ma = MAGeneral);

	std::string& ReadLine(std::string& target);
	std::string& PeekLine(std::string& target);

	template <typename T>
	Connection& DataLine(const T& msg, MessageArea ma = MAGeneral)
	{
		WritePrefixLine(DATA_PREFIX, ma, msg, m_Log->Debug());
		return *this;
	}

	template <typename T>
	Connection& VerboseLine(const T& msg, MessageArea ma = MAGeneral)
	{
		WritePrefixLine(VERBOSE_PREFIX, ma, msg, m_Log->Debug());
		return *this;
	}

	template <typename T>
	Connection& ErrorLine(const T& msg, MessageArea ma = MAGeneral)
	{
		WritePrefixLine(ERROR_PREFIX, ma, msg, m_Log->Notice());
		return *this;
	}

	template <typename T>
	Connection& WarnLine(const T& msg, MessageArea ma = MAGeneral)
	{
		WritePrefixLine(WARNING_PREFIX, ma, msg, m_Log->Notice());
		return *this;
	}

	template <typename T>
	Connection& InfoLine(const T& msg, MessageArea ma = MAGeneral)
	{
		WritePrefixLine(INFO_PREFIX, ma, msg, m_Log->Debug());
		return *this;
	}

	// Params: -1 means not specified
	Connection& Progress(int pct = -1, time_t timeSoFar = -1, const std::string& message = "", MessageArea ma = MAGeneral);

	Connection& operator<<(const std::vector<std::string>& v);

private:

	Connection& WritePrefix(const char* prefix, MessageArea ma, LogWriter& log);

	template <typename T>
	Connection& Write(const T& v, LogWriter& log)
	{
		log << v;
		m_Pipe->Write(v);
		return *this;
	}
	
	// Encode newlines in strings
	Connection& Write(const std::string& v, LogWriter& log);
	Connection& Write(const char* v, LogWriter& log);

	Connection& WriteEndl(LogWriter& log);

	template <typename T>
	Connection& WriteLine(const T& v, LogWriter& log)
	{
		Write(v, log);
		WriteEndl(log);
		Flush();
		return *this;
	}

	template <typename T>
	Connection& WritePrefixLine(const char* prefix, MessageArea ma, const T& v, LogWriter& log)
	{
		WritePrefix(prefix, ma, log);
		Write(v, log);
		WriteEndl(log);
		Flush();
		return *this;
	}

	LogStream* m_Log;
	Pipe* m_Pipe;
};


// Declare primary template to catch wrong specializations
template <typename T>
Connection& operator<<(Connection& p, const T& v);

template <typename T>
Connection& operator<<(Connection& p, const std::vector<T>& v)
{
	p.DataLine(v.size());
	for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); ++i)
		p << *i;
	return p;
}

template <typename T>
Connection& operator<<(Connection& p, const std::set<T>& v)
{
	p.DataLine(v.size());
	for (typename std::set<T>::const_iterator i = v.begin(); i != v.end(); ++i)
		p << *i;
	return p;
}

template <typename T>
Connection& operator>>(Connection& conn, std::vector<T>& v)
{
	std::string line;
	conn.ReadLine(line);
	int count = atoi(line.c_str());
	T t;
	if (count >= 0)
	{
		while (count--)
		{
			conn >> t;
			v.push_back(t);
		}
	}
	else 
	{
		// TODO: Remove
		// Newline delimited list
		while (!conn.PeekLine(line).empty())
		{
			conn >> t;
			v.push_back(t);
		}
		conn.ReadLine(line);
	}
	return conn;
}

template <typename T>
Connection& operator>>(Connection& conn, std::set<T>& v)
{
	std::string line;
	conn.ReadLine(line);
	int count = atoi(line.c_str());
	T t;
	if (count >= 0)
	{
		while (count--)
		{
			conn >> t;
			v.insert(t);
		}
	}
	else 
	{
		// TODO: Remove
		// Newline delimited list
		while (!conn.PeekLine(line).empty())
		{
			conn >> t;
			v.insert(t);
		}
		conn.ReadLine(line);
	}
	return conn;
}
