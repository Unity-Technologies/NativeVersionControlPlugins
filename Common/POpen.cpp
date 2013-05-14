#include "Utility.h"

using namespace std;

#if defined(_WINDOWS)
#include "string.h"
#include "FileSystem.h"

#define BUFSIZE 4096 

POpen::POpen(const string& cmd) : m_Command(cmd)
,	m_ChildStd_IN_Rd(NULL)
,	m_ChildStd_IN_Wr(NULL)
,	m_ChildStd_OUT_Rd(NULL)
,	m_ChildStd_OUT_Wr(NULL)
,	m_BufSize(0)
{
	//
	// Setup pipes
	//
	SECURITY_ATTRIBUTES saAttr; 

	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 
	
	// Create a pipe for the child process's STDOUT. 
	Enforce<PluginException>(CreatePipe(&m_ChildStd_OUT_Rd, &m_ChildStd_OUT_Wr, &saAttr, 0) == TRUE, string("Error creating STDOUT for '") + cmd + "' " + ErrorCodeToMsg(GetLastError()));

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	Enforce<PluginException>(SetHandleInformation(m_ChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) == TRUE, string("Error STDOUT no inherit '") + cmd + "' " + ErrorCodeToMsg(GetLastError())); 
	
	// Create a pipe for the child process's STDIN. 
	Enforce<PluginException>(CreatePipe(&m_ChildStd_IN_Rd, &m_ChildStd_IN_Wr, &saAttr, 0) == TRUE,  string("Error creating STDIN for '") + cmd + "' " + ErrorCodeToMsg(GetLastError())); 

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	Enforce<PluginException>(SetHandleInformation(m_ChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) == TRUE, string("Error STDIN no inherit '") + cmd + "'" + ErrorCodeToMsg(GetLastError()));

	//
	// Create child process
	//
	STARTUPINFOW siStartInfo;
	BOOL bSuccess = FALSE; 

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory( &m_ProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = m_ChildStd_OUT_Wr;
	siStartInfo.hStdOutput = m_ChildStd_OUT_Wr;
	siStartInfo.hStdInput = m_ChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	siStartInfo.wShowWindow = SW_HIDE;

	const size_t kCommandSize = 32768; 
	static wchar_t widePath[kCommandSize]; 
	ConvertUnityPathName(m_Command.c_str(), widePath, kCommandSize);

	// Create the child process. 
	bSuccess = CreateProcessW(NULL, 
		widePath,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&m_ProcInfo);  // receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	Enforce<PluginException>(bSuccess == TRUE, string("Could not start '") + cmd + "'" + ErrorCodeToMsg(GetLastError()));
	
	// Close the client pipe ends here or we will keep the handles open even though the
	// client has terminated. This is turn would block reads/writes and we would get EOL.
	CloseHandle(m_ChildStd_OUT_Wr);
	CloseHandle(m_ChildStd_IN_Rd);
	m_ChildStd_OUT_Wr = NULL;
	m_ChildStd_IN_Rd = NULL;

	// We need to wait for proper initialization of the process (e.g. dll loading)
	// before we can be confident that the process is running ok.
	DWORD msWait = 10000; // Wait at most 10 seconds 
	DWORD exitCode;
	DWORD waitRes = WaitForInputIdle(m_ProcInfo.hProcess, msWait);
	switch (waitRes)
	{
	case WAIT_TIMEOUT:
		throw PluginException(string("Timed out starting '" + cmd + "'"));
	case WAIT_FAILED:
		{
			if (!GetExitCodeProcess(m_ProcInfo.hProcess, &exitCode))
				throw PluginException(string("Could not get exit code for failed process '") + cmd + "': " + LastErrorToMsg());

			if (exitCode == STILL_ACTIVE)
			{
				//WaitForSingleObject(m_ProcInfo.hProcess, msWait);
				//GetExitCodeProcess(m_ProcInfo.hProcess, &exitCode);
			}
			else
			{
				throw PluginException(string("Process failed '") + cmd + "' exit code " + IntToString(exitCode));
			}
		}
	default:
		break;
	}

	if (!GetExitCodeProcess(m_ProcInfo.hProcess, &exitCode))
		throw PluginException(string("Could not get exit code for process '") + cmd + "': " + LastErrorToMsg());

	if (exitCode != STILL_ACTIVE && exitCode != 0)
		throw PluginException(string("Failed to start process '") + cmd + "' exit code " + IntToString(exitCode));
}

POpen::~POpen()
{
	string msg = "";
	if (m_ChildStd_IN_Rd != NULL && !CloseHandle(m_ChildStd_IN_Rd))
		msg += ErrorCodeToMsg(GetLastError()) + " - ";
	if (m_ChildStd_IN_Wr != NULL && !CloseHandle(m_ChildStd_IN_Wr))
		msg += ErrorCodeToMsg(GetLastError()) + " - ";
	if (m_ChildStd_OUT_Rd != NULL && !CloseHandle(m_ChildStd_OUT_Rd))
		msg += ErrorCodeToMsg(GetLastError()) + " - ";
	if (m_ChildStd_OUT_Wr != NULL && !CloseHandle(m_ChildStd_OUT_Wr))
		msg += ErrorCodeToMsg(GetLastError()) + " - ";
	if (m_ProcInfo.hProcess != NULL && !CloseHandle(m_ProcInfo.hProcess))
		msg += ErrorCodeToMsg(GetLastError()) + " - ";
	if (m_ProcInfo.hThread != NULL && !CloseHandle(m_ProcInfo.hThread))
		msg += ErrorCodeToMsg(GetLastError()) + " - ";
	// msg could be printed or something at some point
}

bool POpen::ReadLine(string& result)
{
	static int aa = 0;

	if (m_BufSize == BUFSIZE+1)
		return false;
	
	DWORD dwRead; 

	for (;;) 
	{ 

		DWORD i = 0;
		for ( ; i < m_BufSize; ++i)
			if (m_Buf[i] == '\n')
				break;

		//if (aa++ == 23)
		//	throw PluginException(string("got '") + string(m_Buf, m_BufSize) + "'" + ToString(m_BufSize));

		// throw PluginException(ToString(m_BufSize) + " " + ToString(i) + ": " + string(m_Buf, dwRead));
		if (i != m_BufSize)
		{
			// Found a newline
			bool stripCR = i > 0 && m_Buf[i-1] == '\r';
	
			result.assign(m_Buf, stripCR ? i-1 : i);

			if (i+1 < m_BufSize)
			{
				// +1 to skip \n
				m_BufSize -= i + 1;
				memmove(m_Buf, m_Buf + i + 1, m_BufSize);
			}
			else
			{
				m_BufSize = 0;
			}
			return true;
		}

		// Need more data 
		DWORD bufferLeft = BUFSIZE - m_BufSize - 1;
		Enforce<PluginException>(bufferLeft > 0, string("Buffer overflow in ReadLine for '") + m_Command + "'");
		
		if (!ReadFile( m_ChildStd_OUT_Rd, m_Buf + m_BufSize, bufferLeft, &dwRead, NULL))
		{
			DWORD err = GetLastError();
			if (err != ERROR_BROKEN_PIPE)
				throw PluginException(string("Error during read from '") + m_Command + "': " + ErrorCodeToMsg(GetLastError()));
			dwRead = 0;
		}

		if (dwRead == 0)
		{
			// End if pipe
			if (m_BufSize != 0)
			{
				result.assign(m_Buf, m_BufSize);
				m_BufSize = BUFSIZE+1; // signal no more data
				return true;
			}
			return false;
		}

		// look for newline
		m_BufSize += dwRead;
	}
}

const size_t kDefaultPathBufferSize = 1024;

void POpen::ReadIntoFile(const std::string& path)
{
	wchar_t widePath[kDefaultPathBufferSize];
	ConvertUnityPathName(path.c_str(), widePath, kDefaultPathBufferSize);
	HANDLE fh = CreateFileW(
		widePath, 
		GENERIC_WRITE | GENERIC_READ, 
		0, 
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL); 
	
	Enforce<PluginException>(fh != INVALID_HANDLE_VALUE, string("Invalid result file handle for ReadIntoFile during '") + m_Command + "' " + path + " " + ErrorCodeToMsg(GetLastError())); 

	DWORD dwRead, dwWritten; 

	for (;;) 
	{ 
		if (!ReadFile( m_ChildStd_OUT_Rd, m_Buf, BUFSIZE, &dwRead, NULL))
		{
			DWORD err = GetLastError();
			if (err != ERROR_BROKEN_PIPE)
				throw PluginException(string("Error during read from '") + m_Command + "': " + ErrorCodeToMsg(err));
			break;
		}

		if( dwRead == 0 ) break; // end of pipe

		if (!WriteFile(fh, m_Buf, dwRead, &dwWritten, NULL))
		{
			CloseHandle(fh);
			throw PluginException(string("Error during read from '") + m_Command + "': " + ErrorCodeToMsg(GetLastError()));
		}
	}
	CloseHandle(fh);
}

#else // posix

POpen::POpen(const string& cmd) : m_Command(cmd)
{
	m_Handle = popen((cmd + " 2>&1").c_str(), "r");
	Enforce<PluginException>(m_Handle, string("Error starting '") + cmd + "'");
}

POpen::~POpen()
{
	if (m_Handle)
		pclose(m_Handle);
}

bool POpen::ReadLine(string& result)
{
	Enforce<PluginException>(m_Handle, string("Null handle when reading from command pipe: ") + m_Command);

	const size_t BUFSIZE = 8192;
	static char buf[BUFSIZE];
	if (feof(m_Handle)) return false;

	char* res = fgets(buf, BUFSIZE, m_Handle);
	if (!res)
	{
		if (feof(m_Handle))
			return false; // no more data

		throw PluginException(string("Error reading command pipe :") + 
			strerror(ferror(m_Handle)) 
			+ " - "  + m_Command);
	}
	result = res;
	string::reverse_iterator i = result.rbegin();
	if (!result.empty() && *i == '\n')
		result.resize(result.size()-1);

	return true;
}

void POpen::ReadIntoFile(const std::string& path)
{
	Enforce<PluginException>(m_Handle, string("Null handle when reading into filefrom command pipe: ") + m_Command);

	const size_t BUFSIZE = 8192;
	static char buf[BUFSIZE];

	FILE* fh = fopen(path.c_str(), "w");

	if (feof(m_Handle)) 
	{
		fclose(fh);
		return;
	}

	size_t bytes = fread(buf, 1, BUFSIZE, m_Handle);
	while (!ferror(m_Handle))
	{
		if (bytes == 0 && feof(m_Handle))
		{
			// done ok
			fclose(fh);
			return;
		}

		if (fwrite(buf, 1, bytes, fh) != bytes)
		{
			// Error writing to disk
			fclose(fh);
			throw PluginException(string("Error writing process output into file: ") + path + " for command " + m_Command);
		}
		bytes = fread(buf, BUFSIZE, 1, m_Handle);
	}

	stringstream os;
	os << "Error writing process output to file: ";
	os << path << " code " << ferror(fh);
	os << " for command " <<  m_Command << std::endl;
	fclose(fh);
	throw PluginException(os.str());
}

#endif // end defined(_WINDOWS) or posix
