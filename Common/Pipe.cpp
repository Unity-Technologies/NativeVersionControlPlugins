#include "Pipe.h"
#include "Utility.h"

using namespace std;

Pipe::Pipe() : m_LineBufferValid(false)
{
#if defined(_WINDOWS)
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\UnityVCS"); 
		
	// All pipe instances are busy, so wait for 2 seconds. 
	if ( ! WaitNamedPipe(lpszPipename, 2000)) 
	{ 
		string msg = "Could not open pipe: 2 second wait timed out.\n";
		msg += ErrorCodeToMsg(GetLastError());
		throw PipeException(msg);
	}
		
	m_NamedPipe = CreateFile( 
							 lpszPipename,   // pipe name 
							 GENERIC_READ |  // read and write access 
							 GENERIC_WRITE, 
							 0,              // no sharing 
							 NULL,           // default security attributes
							 OPEN_EXISTING,  // opens existing pipe
							 0,              // default attributes
							 NULL);          // no template file
		
	// Break if the pipe handle is valid. 
	if (m_NamedPipe == INVALID_HANDLE_VALUE) 
	{		
		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		string msg = "Could not open pipe. GLE=";
		msg += ErrorCodeToMsg(GetLastError());
		throw PipeException(msg);
	}
#endif
}
	
Pipe::~Pipe()
{
#if defined(_WINDOWS)
	if (m_NamedPipe != INVALID_HANDLE_VALUE)
	{
		FlushFileBuffers(m_NamedPipe);
			
		CloseHandle(m_NamedPipe);
	}
#endif
}

void Pipe::Flush()
{
#if defined(_WINDOWS)
	FlushFileBuffers(m_NamedPipe);
#else
	cout << flush;
#endif
}

Pipe& Pipe::Write(const string& str)
{
#if defined(_WINDOWS)
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
		string msg = "WriteFile to pipe failed. GLE=";
		msg += ErrorCodeToMsg(GetLastError());
		throw PipeException(msg);
	}

#else
	cout << str;
#endif
	return *this;
}


string& Pipe::ReadLine(string& target)
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
		string::size_type i = m_Buffer.find("\n");
		if (i != string::npos)
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
		m_Buffer.append(buffer, bytesRead);
	};
			
	if (!success)
	{
		string msg = "Readfile from pipe failed. GLE=";
		msg += ErrorCodeToMsg(GetLastError());
		throw PipeException(msg);
	}
#else
	getline(cin, target);
#endif
	return target;
}
	
string& Pipe::PeekLine(string& dest)
{
	ReadLine(m_LineBuffer);
	dest = m_LineBuffer;
	m_LineBufferValid = true;
	return dest;
}
	
bool Pipe::IsEOF() const
{
#if defined(_WINDOWS)
	return false; // TODO: Implement
#else
	return cin.eof();
#endif
}
