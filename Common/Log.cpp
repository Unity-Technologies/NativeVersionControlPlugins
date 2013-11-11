#include "Log.h"
#include <iostream>

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

LogStream::LogStream(LogLevel level)
: m_LogLevel(level),
m_OnWriter(Self(), true),
m_OffWriter(Self(), false)
{
	m_Stream = &std::cout;
}

LogStream::LogStream(const std::string& path, LogLevel level)
: m_LogLevel(level),
m_FileStream(path.c_str(), std::ios_base::app),
m_OnWriter(Self(), true),
m_OffWriter(Self(), false)
{
	m_Stream = &m_FileStream;
}

LogStream& LogStream::Self(void)
{
	// Work around C4355
	return *this;
}

LogStream::~LogStream()
{
	m_Stream->flush();
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
	return m_LogLevel <= LOG_DEBUG ? m_OnWriter : m_OffWriter;
}

LogWriter& LogStream::Info()
{
	return m_LogLevel <= LOG_INFO ? m_OnWriter : m_OffWriter;
}

LogWriter& LogStream::Notice()
{
	return m_LogLevel <= LOG_NOTICE ? m_OnWriter : m_OffWriter;
}

LogWriter& LogStream::Fatal()
{
	return m_LogLevel <= LOG_FATAL ? m_OnWriter : m_OffWriter;
}

LogStream& LogStream::Flush()
{
	*m_Stream << std::flush;
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