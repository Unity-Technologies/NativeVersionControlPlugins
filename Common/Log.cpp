#include "Log.h"
#include <time.h>

LogWriter::LogWriter(LogStream& stream, bool isOn) : m_Stream(stream), m_On(isOn) 
{
}
	
LogWriter& Flush(LogWriter& w)
{
	w.Flush();
	return w;
}

LogWriter& Endl(LogWriter& w)
{
	w.Write("\n");
	w.Flush();
	return w;
}


LogStream::LogStream(const std::string& path, LogLevel level) 
	: m_LogLevel(level),
      m_CurrLogLevel(level),
	  m_Stream(path.c_str(), std::ios_base::app),
      m_Buffer(""),
	  m_OnWriter(Self(), true), 
	  m_OffWriter(Self(), false)
{
}

LogStream& LogStream::Self(void)
{
	// Work around C4355
	return *this;
}

LogStream::~LogStream()
{
	m_Stream.flush();
	m_Stream.close();
}

void LogStream::SetLogLevel(LogLevel l) 
{ 
    m_LogLevel = l; 
}

LogLevel LogStream::GetLogLevel() const 
{ 
	return m_LogLevel; 
}

LogWriter& LogStream::Debug() 
{
    m_CurrLogLevel = LOG_DEBUG;
	return m_LogLevel <= LOG_DEBUG ? m_OnWriter : m_OffWriter; 
}

LogWriter& LogStream::Info() 
{ 
    m_CurrLogLevel = LOG_INFO;
	return m_LogLevel <= LOG_INFO ? m_OnWriter : m_OffWriter;
}

LogWriter& LogStream::Notice() 
{ 
    m_CurrLogLevel = LOG_NOTICE;
	return m_LogLevel <= LOG_NOTICE ? m_OnWriter : m_OffWriter;
}

LogWriter& LogStream::Fatal() 
{ 
    m_CurrLogLevel = LOG_FATAL;
	return m_LogLevel <= LOG_FATAL ? m_OnWriter : m_OffWriter;
}

void LogStream::WritePrefix()
{
    switch (m_CurrLogLevel)
    {
        case LOG_DEBUG:
            m_Stream << "[DBG]";
            break;
        case LOG_INFO:
            m_Stream << "[INF]";
            break;
        case LOG_NOTICE:
            m_Stream << "[NOT]";
            break;
        case LOG_FATAL:
            m_Stream << "[ERR]";
            break;
    }
    
    time_t tim;
    time(&tim);
    std::string t = std::string(ctime(&tim));
    t.resize(t.length()-1);
    
    m_Stream << "[" << t.c_str() << "] ";
}

LogStream& LogStream::Flush()
{
    if (!m_Buffer.empty())
    {
        WritePrefix();
        m_Stream << m_Buffer.c_str();
        m_Buffer.clear();
	}
	m_Stream << std::flush;
	return *this;
}

LogWriter& operator<<(LogWriter& w, LogWriter& (*pf)(LogWriter&))
{
	return pf(w);
}

LogStream& operator<<(LogStream& w, LogStream& (*pf)(LogStream&))
{
	return pf(w);
}

LogWriter& LogWriter::Flush() 
{
	if (m_On) m_Stream.Flush();
	return *this;
}
	
LogStream& Flush(LogStream& w)
{
	w.Flush();
	return w;
}

LogStream& Endl(LogStream& w)
{
	w.Write("\n");
	w.Flush();
	return w;
}

