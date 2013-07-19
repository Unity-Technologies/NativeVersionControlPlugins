#pragma once
#include <fstream>

// Log levels. 
// Lower levels are included in highers levels automatically.
enum LogLevel
{
	LOG_DEBUG,  // protocol messages
	LOG_INFO,   // commands
	LOG_NOTICE, // warnings and errors
	LOG_FATAL,  // exceptions
};


class LogStream;


// A convenient way of disabling certain log levels
// in a returned object.
class LogWriter
{
public:
	LogWriter(LogStream& stream, bool isOn);
	LogWriter& Flush();
	template<typename T> LogWriter& Write(const T& v);

private:
	LogStream& m_Stream;
	bool m_On;
};

template<typename T>
LogWriter& LogWriter::Write(const T& v)
{
	if (m_On)
		m_Stream << v;
	return *this;
}
	
template<typename T>
LogWriter& operator<<(LogWriter& w, const T& v)
{
	return w.Write(v);
}

LogWriter& operator<<(LogWriter& w, LogWriter& (*pf)(LogWriter&));
LogWriter& Flush(LogWriter& w);
LogWriter& Endl(LogWriter& w);

class LogStream
{		
public:
	LogStream(const std::string& path, LogLevel level = LOG_NOTICE);
	~LogStream();
	
	LogStream& Self(void);

	void SetLogLevel(LogLevel l);
	LogLevel GetLogLevel() const;

	LogWriter& Debug();
	LogWriter& Info();
	LogWriter& Notice();
	LogWriter& Fatal();

	LogStream& Flush();

	template<typename T> LogStream& Write(const T& v);
private:
	LogLevel m_LogLevel;
	std::ofstream m_Stream;
	LogWriter m_OnWriter;  // write logs to file
	LogWriter m_OffWriter; // throw away logs
};


template<typename T>
LogStream& LogStream::Write(const T& v)
{
	m_Stream << v;
	return *this;
}

template<typename T>
LogStream& operator<<(LogStream& s, const T& v)
{
	return s.Write(v);
}

LogStream& operator<<(LogStream& w, LogStream& (*pf)(LogStream&));
LogStream& Flush(LogStream& w);
LogStream& Endl(LogStream& w);

