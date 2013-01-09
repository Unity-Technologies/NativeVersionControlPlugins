#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include "Utility.h"

#if defined(_WINDOWS)
#include <sstream>
#include <windows.h>
#endif

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

// Convenience pipe that writes to both log file and 
// stdout pipe (ie. the unity process)
struct UnityPipe {
		
	UnityPipe(const std::string& logPath) : m_Log(logPath.c_str(), std::ios_base::ate), m_LineBufferValid(false)
	{
		m_Log << "UnityPipe set up" << std::endl;
#if defined(_WINDOWS)
		m_NamedPipe = INVALID_HANDLE_VALUE;
#endif
	}

	~UnityPipe()
	{
#if defined(_WINDOWS)
		if (m_NamedPipe != INVALID_HANDLE_VALUE)
			CloseHandle(m_NamedPipe);
#endif
	}
	
	template <typename T>
	UnityPipe& __Write(const T& v)
	{
		m_Log << v;
#if defined(_WINDOWS)
		std::stringstream ss;
		ss << v;
		std::string str = ss.str();
		const CHAR* buf = str.c_str(); 
		size_t toWrite = str.length();
		DWORD written;
		BOOL success = WriteFile(m_NamedPipe,            // pipe handle 
							 buf,             // message 
							 toWrite,              // message length 
							 &written,             // bytes written 
							 NULL);                  // not overlapped 
		
		if (!success) 
		{
			Log() << TEXT("WriteFile to pipe failed. GLE=") << ErrorCodeToMsg(GetLastError()) << std::endl;
			exit(-1);
		}
#else
		std::cout << v;
#endif
		return *this;
	}

	template <typename T>
	UnityPipe& Write(const T& v)
	{
		return __Write(v);
	}
	
	UnityPipe& Write(const std::string& v)
	{
		std::string tmp = Replace(v, "\\", "\\\\");
		tmp = Replace(v, "\n", "\\n");
		__Write(tmp);
		return *this;
	}

	UnityPipe& Write(const char* v)
	{
		return Write(std::string(v));
	}

	template <typename T>
	UnityPipe& WriteLine(const T& v)
	{
		Write(v);
		__Write("\n");
#if defined(_WINDOWS)
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}
	
	std::string& ReadLine(std::string& target)
	{
		if (m_LineBufferValid)
		{
			target.swap(m_LineBuffer);
			m_LineBufferValid = false;
			return target;
		}

#if defined(_WINDOWS)

		BOOL success; 
		DWORD bytesRead;
		const size_t BUFSIZE = 4096 * 4;
		static CHAR buffer[BUFSIZE]; 
			
		// Read until a newline appears in the m_Buffer
		while (true)
		{ 
			// First check if we have already buffered lines up for grabs
			std::string::size_type i = m_Buffer.find("\n");
			if (i != std::string::npos)
			{
				success = true;
				target = m_Buffer.substr(0,i);
				++i; // Eat \n
				if (i >= m_Buffer.length()) 
					m_Buffer.clear();
				else
					m_Buffer = m_Buffer.substr(i); 
				break; // success
			}

			// Read from the pipe. 
			success = ReadFile(m_NamedPipe,    // pipe handle 
								buffer,    // buffer to receive reply 
								BUFSIZE * sizeof(CHAR),  // size of buffer 
								&bytesRead,  // number of bytes read 
								NULL);    // not overlapped 
			
			if ( !success && GetLastError() != ERROR_MORE_DATA )
				break; // error 	
				
			// Put data to the buffer
			buffer[bytesRead] = 0x0; // terminate the read string
			m_Buffer += (char*)buffer;
		}; 
			
		if (!success)
		{
				
			Log() << TEXT("ReadFile from pipe failed. GLE=%d\n") <<  ErrorCodeToMsg(GetLastError()) << std::endl;
			exit(-1);
		}
#else
		std::getline(std::cin, target);
#endif
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

		return target;
	}
	
	std::string& PeekLine(std::string& dest)
	{
		ReadLine(m_LineBuffer);
		dest = m_LineBuffer;
		m_LineBufferValid = true;
		return dest;
	}
	
	std::ofstream& Log()
	{
		return m_Log;
	}
	
	UnityPipe& BeginList()
	{
		// Start sending list of unknown size
		OkLine("-1");
		return *this;
	}
	
	UnityPipe& EndList()
	{
		// d is list delimiter
		__Write("d1:end of list\n");
#if defined(_WINDOWS)
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}
	
	UnityPipe& EndResponse()
	{
		m_Log << "\n--------------------------\n" << std::flush;
		__Write("r1:end of response\n");
#if !defined(_WINDOWS)
		std::cout << std::flush;
#else
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}
	
	UnityPipe& Ok(const std::string& msg = "", MessageArea ma = MAGeneral)
	{
		Write("o");
		Write(ma);
		Write(":");
		if (!msg.empty())
			Write(msg);
		return *this;
	}

	template <typename T>
	UnityPipe& Ok(const T& msg, MessageArea ma = MAGeneral)
	{
		Write("o");
		Write(ma);
		Write(":");
		Write(msg);
		return *this;
	}

	UnityPipe& OkLine(const std::string& msg = "", MessageArea ma = MAGeneral)
	{
		Ok(msg, ma); __Write("\n");
#if defined(_WINDOWS)
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}
	
	template <typename T>
	UnityPipe& OkLine(const T& msg, MessageArea ma = MAGeneral)
	{
		Ok(msg, ma); __Write("\n");
#if defined(_WINDOWS)
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}

	UnityPipe& Error(const std::string& message, MessageArea ma = MAGeneral)
	{
		Write("e"); 
		Write(ma);
		Write(":");
		Write(message);
		return *this;
	}

	UnityPipe& ErrorLine(const std::string& message, MessageArea ma = MAGeneral)
	{
		Error(message, ma); __Write("\n");
#if defined(_WINDOWS)
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}

	UnityPipe& Warn(const std::string& message, MessageArea ma = MAGeneral)
	{
		Write("w"); 
		Write(ma);
		Write(":");
		Write(message);
		return *this;
	}

	UnityPipe& WarnLine(const std::string& message, MessageArea ma = MAGeneral)
	{
		Warn(message, ma); __Write("\n");
#if defined(_WINDOWS)
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}
	
	UnityPipe& Info(const std::string& message, MessageArea ma = MAGeneral)
	{
		Write("i"); 
		Write(ma);
		Write(":");
		Write(message);
		return *this;
	}
	
	UnityPipe& InfoLine(const std::string& message, MessageArea ma = MAGeneral)
	{
		Info(message, ma); __Write("\n");
#if defined(_WINDOWS)
		FlushFileBuffers(m_NamedPipe);
#endif
		return *this;
	}

	UnityPipe& operator<<(const std::vector<std::string>& v)
	{
		OkLine(v.size());
		for (std::vector<std::string>::const_iterator i = v.begin(); i != v.end(); ++i)
			WriteLine(*i);
		return *this;
	}
	

#if defined(_WINDOWS)
	void SetChannel(HANDLE c)
	{
		m_NamedPipe = c;
	}
#endif
	
private:
	std::ofstream m_Log;
	bool m_LineBufferValid;
	std::string m_LineBuffer;

#if defined(_WINDOWS)
	HANDLE m_NamedPipe;
	std::string m_Buffer;
#endif
};
					

template <typename T>
UnityPipe& operator<<(UnityPipe& p, const T& v)
{
	p.Write(v);
	return p;
}

template <typename T>
UnityPipe& operator<<(UnityPipe& p, const std::vector<T>& v)
{
	p.OkLine(v.size());
	for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); ++i)
		p << *i;
	return p;
}

template <typename T>
UnityPipe& operator>>(UnityPipe& pipe, std::vector<T>& v)
{
	std::string line;
	pipe.ReadLine(line);
	int count = atoi(line.c_str());
	T t;
	if (count >= 0)
	{
		while (count--)
		{
			pipe >> t;
			v.push_back(t);
		}
	} 
	else 
	{
		// TODO: Remove
		// Newline delimited list
		while (!pipe.PeekLine(line).empty())
		{
			pipe >> t;
			v.push_back(t);
		}
		pipe.ReadLine(line);
	}
	return pipe;
}
