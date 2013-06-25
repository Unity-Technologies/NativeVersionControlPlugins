#include "UnityPrefix.h"
#include "Editor/Platform/Interface/ExternalProcess.h"
#include "Runtime/Utilities/File.h"
#include "PlatformDependent/Win/PathUnicodeConversion.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;


ExternalProcess::ExternalProcess(const std::string& app, const std::vector<std::string>& arguments)
: m_LineBufferValid(false), m_ApplicationPath(app), m_Arguments(arguments),
  m_ReadTimeout(30.0), m_WriteTimeout(30.0)
{
	ZeroMemory( &m_Task, sizeof(m_Task) );
	m_Task.hProcess = INVALID_HANDLE_VALUE;
	m_Event = INVALID_HANDLE_VALUE;
	m_NamedPipe = INVALID_HANDLE_VALUE;
}

ExternalProcess::~ExternalProcess()
{
	Shutdown();
}

bool ExternalProcess::Launch()
{
	// Most of this is from MSDN's "Creating a Child Process with Redirected Input and Output",
	// except for the parts where the code example is wrong :)
	//
	// See http://www.codeproject.com/KB/threads/redir.aspx for details on where it's wrong...

	// Make sure the plugin is not running
	Shutdown();
	m_Buffer.clear();

	// LogString(string("Launching external process ") + m_ApplicationPath);

	// Search path if app is not absolute name
	wchar_t appWide[kDefaultPathBufferSize];
	ConvertUnityPathName( m_ApplicationPath.c_str(), appWide, kDefaultPathBufferSize );
	if( !IsAbsoluteFilePath(m_ApplicationPath) )
	{
		wchar_t pathWide[kDefaultPathBufferSize];
		memset(pathWide, 0, sizeof(pathWide));
		wchar_t* namePart;
		if( SearchPathW( NULL, appWide, NULL, kDefaultPathBufferSize, pathWide, &namePart ) > 0 )
		{
			memcpy( appWide, pathWide, sizeof(pathWide) );
		}
	}

	// Now create a child process
	STARTUPINFOW siStartInfo;
	ZeroMemory( &m_Task, sizeof(m_Task) );
	ZeroMemory( &siStartInfo, sizeof(siStartInfo) );
	siStartInfo.cb = sizeof(siStartInfo);
	siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
//	siStartInfo.hStdOutput = INVALID_HANDLE_VALUE; // m_Task_stdout_wr.handle;
//	siStartInfo.hStdInput = INVALID_HANDLE_VALUE; // m_Task_stdin_rd.handle;
	siStartInfo.wShowWindow = SW_HIDE;

	// generate an argument list
	std::wstring argumentString = std::wstring(L"\"") + appWide + L'"';
	std::wstring argWide;
	for( int i = 0; i < m_Arguments.size(); ++i ) {
		argumentString += L' ';

		std::string arg = m_Arguments[ i ];
		arg = QuoteString( arg );
		ConvertUTF8ToWideString( arg, argWide );
		argumentString += argWide;
	}

	wchar_t* argumentBuffer = new wchar_t[argumentString.size()+1];
	memcpy( argumentBuffer, argumentString.c_str(), (argumentString.size()+1)*sizeof(wchar_t) );

	// launch process
	wchar_t directoryWide[kDefaultPathBufferSize];
	ConvertUnityPathName( File::GetCurrentDirectory().c_str(), directoryWide, kDefaultPathBufferSize );

	BOOL processResult = CreateProcessW(
		appWide,		// application
		argumentBuffer, // command line
		NULL,          // process security attributes
		NULL,          // primary thread security attributes
		TRUE,          // handles are inherited
		0,             // creation flags
		NULL,          // use parent's environment
		File::GetCurrentDirectory().empty() ? NULL : directoryWide, // directory
		&siStartInfo,  // STARTUPINFO pointer
		&m_Task ); // receives PROCESS_INFORMATION

	delete[] argumentBuffer;

	if( processResult == FALSE )
	{
		LogString("Error creating external process: " + winutils::ErrorCodeToMsg(GetLastError()));
		return false;
	}

	if (!CloseHandle(m_Task.hThread))
		LogString("Error closing external process thread: " + winutils::ErrorCodeToMsg(GetLastError()));

	//
	// Setup named pipe to external process
	//
	wchar_t pipeWide[kDefaultPathBufferSize];
	ConvertUnityPathName("\\\\.\\pipe\\UnityVCS", pipeWide, kDefaultPathBufferSize);

	memset(&m_Overlapped, 0, sizeof(m_Overlapped));
	m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_Overlapped.hEvent = m_Event;
    m_NamedPipe = CreateNamedPipeW(pipeWide, 
								  /* FILE_FLAG_FIRST_PIPE_INSTANCE | */ FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX, 
								  PIPE_TYPE_BYTE |  PIPE_READMODE_BYTE | PIPE_WAIT,
								  PIPE_UNLIMITED_INSTANCES, 16384, 16384, 0, NULL);
	if (m_NamedPipe == INVALID_HANDLE_VALUE)
	{
		LogString("Error creating named pipe: " + winutils::ErrorCodeToMsg(GetLastError()));
		Cleanup();
		return false;
	}
	
    int ret = ConnectNamedPipe(m_NamedPipe, &m_Overlapped);
	
	if (ret != 0)
	{
		LogString("Error connecting named pipe: " + winutils::ErrorCodeToMsg(GetLastError()));
		Cleanup();
		return false;
	}

	switch (GetLastError()) {
		case ERROR_PIPE_CONNECTED:
			return true; // connected to external process
			break;
		case ERROR_IO_PENDING:
		{
			// Wait for connect for 30 secs
			const int waitConnectTimeoutMs = 60000;
			
			switch (WaitForSingleObject(m_Event, waitConnectTimeoutMs))
			{
				case WAIT_OBJECT_0:
				{
					DWORD dwIgnore;
					ret = GetOverlappedResult(m_NamedPipe, &m_Overlapped, &dwIgnore, FALSE);
					if (!ret)
					{
						LogString("Error waiting for named pipe connect: " + winutils::ErrorCodeToMsg(GetLastError()));
						Cleanup();
						return false;
					}
					return true; // connected to external process
				}
				case WAIT_TIMEOUT:
					LogStringMsg("Timed out connecting pipe to external process: %s", m_ApplicationPath.c_str());
					break;
				case WAIT_FAILED:
					LogString("Wait timout while connecting to named pipe:" + winutils::ErrorCodeToMsg(GetLastError()));
					break;
				default:
					LogString("Unknown error while connecting pipe to external process");
					break;
			}
			break;
		}
		default:
			LogString("Unknown state while connecting pipe to external process");
			break;
	}	
	Cleanup();
	return false;
}

bool ExternalProcess::IsRunning()
{
	if (m_Task.hProcess == INVALID_HANDLE_VALUE)
		return false;

	DWORD result = WaitForSingleObject(m_Task.hProcess, 0);
	
	if (result == WAIT_FAILED)
		LogString("Error checking run state of external process: " + winutils::ErrorCodeToMsg(GetLastError()));
	
	return result == WAIT_TIMEOUT;
}

// Copies the char array into a string, but ignores all \r
static void ConvertToString(CHAR* buffer, std::string& result)
{
	result.clear();
	
	for(int i = 0; ; ) {
		CHAR c = buffer[i];
		switch (c) {
		case '\r':
			break;
		case 0:
			return;
		default:
			result += (char)c;
		}
		i++;
	}
}

string ExternalProcess::ReadLine()
{
	if (m_LineBufferValid)
	{
		m_LineBufferValid = false;
		return m_LineBuffer;
	}

	const DWORD kBufferSize = 1024;
	static char buffer[kBufferSize + 1];

	DWORD waitReadTimeoutMs = (DWORD)(m_ReadTimeout * 1000.0);

	while (true)
	{
		if (!IsRunning())
		{
			throw ExternalProcessException(EPSTATE_NotRunning, "Trying to read from process that is not running");
		}

		std::string::size_type i = m_Buffer.find("\n");

		if ( i != std::string::npos)
		{
			std::string result(m_Buffer, 0, i);
			++i; // Eat \n
			if (i >= m_Buffer.length()) 
				m_Buffer.clear();
			else
				m_Buffer = m_Buffer.substr(i);
			return result;
		}

		DWORD bytesRead = 0;
		bool success = ReadFile( m_NamedPipe, buffer, kBufferSize, &bytesRead, &m_Overlapped );			
		if ( success )
		{
			if (bytesRead == 0)
				throw ExternalProcessException(EPSTATE_BrokenPipe, "Error reading external process - read 0 bytes");
		}
		else
		{
			switch (GetLastError())
			{
			case ERROR_IO_PENDING:	
				// Wait for data to be readable on pipe
				switch (WaitForSingleObject(m_Event, waitReadTimeoutMs))
				{
					case WAIT_OBJECT_0:
					{
						bool success = GetOverlappedResult( m_NamedPipe, // handle to pipe
															&m_Overlapped, // OVERLAPPED structure 
															&bytesRead,            // bytes transferred 
															FALSE);            // do not wait 					
						if (!success)
						{
							LogString("Error waiting for read external process: " + winutils::ErrorCodeToMsg(GetLastError()));
							throw ExternalProcessException(EPSTATE_BrokenPipe, "Error waiting for read in external process");
						}

						if (bytesRead == 0)
							throw ExternalProcessException(EPSTATE_BrokenPipe, "Error reading any data from external process");

						break;
					}
					case WAIT_TIMEOUT:
						throw ExternalProcessException(EPSTATE_TimeoutReading, "Timeout reading from external process");
					case WAIT_FAILED:
						LogString("Timout waiting for read external process: " + winutils::ErrorCodeToMsg(GetLastError()));
						throw ExternalProcessException(EPSTATE_BrokenPipe, "Failed waiting for read from external process");
					default:
						throw ExternalProcessException(EPSTATE_BrokenPipe, "Unknown error during read of external process");
				}
				break;
			default:
				LogString("Error reading externalprocess: " + winutils::ErrorCodeToMsg(GetLastError()));
				throw ExternalProcessException(EPSTATE_BrokenPipe, "Error read polling external process");
			} 
		}
			
		buffer[bytesRead] = 0;
			
		string newData;
		ConvertToString(buffer, newData);
		m_Buffer += newData;
	}

	throw ExternalProcessException(EPSTATE_TimeoutReading, "Too many retries while reading from external process. This cannot happen.");
	return string();
}

string ExternalProcess::PeekLine()
{
	m_LineBuffer = ReadLine();
	m_LineBufferValid = true;
	return m_LineBuffer;
}


bool ExternalProcess::Write(const std::string& data)
{
	const CHAR* buf = data.c_str(); 
	DWORD written = 0;
	size_t toWrite = data.length();
	DWORD waitWriteTimeoutMs = (DWORD)(m_WriteTimeout * 1000.0);
	
	while (true)
	{
		if (!IsRunning())
		{
			throw ExternalProcessException(EPSTATE_NotRunning, "Trying to write non running external process");
		}
		
		bool success = WriteFile(m_NamedPipe, buf, toWrite, &written, &m_Overlapped);

		if ( success )
		{
			if (toWrite != written)
				throw ExternalProcessException(EPSTATE_BrokenPipe, "Error writing external process - not all bytes written");
			break;
		}
		else
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				// Wait for data to be readable on pipe
				switch (WaitForSingleObject(m_Event, waitWriteTimeoutMs))
				{
					case WAIT_OBJECT_0:
					{
						bool success = GetOverlappedResult( m_NamedPipe, // handle to pipe
													   &m_Overlapped, // OVERLAPPED structure 
													   &written,            // bytes transferred 
													   FALSE);            // do not wait 					
						if (!success)
						{
							LogString("Error waiting to write external process: " + winutils::ErrorCodeToMsg(GetLastError()));
							throw ExternalProcessException(EPSTATE_BrokenPipe, "Error waiting for write in external process");
						}

						if (written != toWrite)
							throw ExternalProcessException(EPSTATE_BrokenPipe, "Error writing all data to external process");
						
						return true;
					}
					case WAIT_TIMEOUT:
						throw ExternalProcessException(EPSTATE_TimeoutReading, "Timeout writing to external process");
					case WAIT_FAILED:
						LogString("Failed waiting to write external process: " + winutils::ErrorCodeToMsg(GetLastError()));
						throw ExternalProcessException(EPSTATE_BrokenPipe, "Failed waiting for write to external process");
					default:
						throw ExternalProcessException(EPSTATE_BrokenPipe, "Unknown error during write to external process");
				}
			}
			else
			{
				LogString("Error writing external process: " + winutils::ErrorCodeToMsg(GetLastError()));
				throw ExternalProcessException(EPSTATE_BrokenPipe, "Error write polling external process");
			} 
		}
	}

	return true;
}
		
void ExternalProcess::Shutdown()
{
	// Wait until exit is needed in order for the task to properly deallocate
	if (IsRunning() && m_NamedPipe != INVALID_HANDLE_VALUE)
	{
		FlushFileBuffers(m_NamedPipe);
		DisconnectNamedPipe(m_NamedPipe);
	}

	Cleanup();
}

void ExternalProcess::SetReadTimeout(double secs)
{
	m_ReadTimeout = secs;
}

double ExternalProcess::GetReadTimeout()
{
	return m_ReadTimeout;
}

void ExternalProcess::SetWriteTimeout(double secs)
{
	m_WriteTimeout = secs;
}

double ExternalProcess::GetWriteTimeout()
{
	return m_WriteTimeout;
}

void ExternalProcess::Cleanup()
{	
	if (m_Event != INVALID_HANDLE_VALUE && !CloseHandle(m_Event))
		LogString("Error cleaning up external process event: " + winutils::ErrorCodeToMsg(GetLastError()));
	m_Event = INVALID_HANDLE_VALUE;

	if (m_NamedPipe != INVALID_HANDLE_VALUE && !CloseHandle(m_NamedPipe))
		LogString("Error cleaning up external process named pipe: " + winutils::ErrorCodeToMsg(GetLastError()));
	m_NamedPipe = INVALID_HANDLE_VALUE;

	if (m_Task.hProcess != INVALID_HANDLE_VALUE  && !TerminateProcess(m_Task.hProcess, 1) && !CloseHandle(m_Task.hProcess))
		LogString("Error cleaning up external process: " + winutils::ErrorCodeToMsg(GetLastError()));
	m_Task.hProcess = INVALID_HANDLE_VALUE;
}
