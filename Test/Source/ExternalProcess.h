#pragma once
#if defined(_WIN32)
#include <Windows.h>

struct AutoHandle
{
	explicit AutoHandle() : handle(INVALID_HANDLE_VALUE) { }
	AutoHandle( HANDLE h ) : handle(h) { }
	~AutoHandle() { Close(); }

	void Close()
	{
		if(handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}
	HANDLE	handle;
};
#else
#include <unistd.h>
#endif

#include <string>
#include <vector>

enum ExternalProcessState {
	EPSTATE_NotRunning,
	EPSTATE_TimeoutReading,
	EPSTATE_TimeoutWriting,
	EPSTATE_BrokenPipe
};


class ExternalProcessException
{
public:
	ExternalProcessException(ExternalProcessState st, const std::string& msg = "") : m_State(st), m_Msg(msg) {}
	const std::string& Message() const throw() { return m_Msg; }
	const char* what() const throw() {  return m_Msg.c_str(); } // standard
private:
	const ExternalProcessState m_State;
	const std::string m_Msg;
};


class ExternalProcess
{
public:
	ExternalProcess(const std::string& app, const std::vector<std::string>& arguments);
	~ExternalProcess();
	bool Launch();
	bool IsRunning();
	bool Write(const std::string& data);
	std::string ReadLine();
	std::string PeekLine();
	void Shutdown();
	void SetReadTimeout(double secs);
	double GetReadTimeout();
	void SetWriteTimeout(double secs);
	double GetWriteTimeout();
	const std::string& GetApplicationPath() const { return m_ApplicationPath; }
private:
	void Cleanup();

	bool m_LineBufferValid;
	std::string m_LineBuffer;
	std::string m_ApplicationPath;
	std::vector<std::string> m_Arguments;
	double m_ReadTimeout;
	double m_WriteTimeout;

#if defined(_WIN32)
	
	PROCESS_INFORMATION m_Task;

	OVERLAPPED m_Overlapped;
	HANDLE m_NamedPipe;
	HANDLE m_Event; // Read or Write event
	
	// Main task writes to m_Task_stdin_wr and child task reads from m_Task_stdin_rd
	AutoHandle m_Task_stdin_rd;
	AutoHandle m_Task_stdin_wr;

	// Main task reads m_Task_stdout_rd and child task writes to m_Task_stdout_wr
	AutoHandle m_Task_stdout_rd;
	AutoHandle m_Task_stdout_wr;

	std::string m_Buffer;

#else // posix

	pid_t m_Pid;
	int m_ExitCode;
	FILE *m_Input,
		 *m_Output;
	bool m_Exited;
	std::string ReadLine (FILE *file, std::string &buffer, bool removeFromBuffer);
#endif
};
