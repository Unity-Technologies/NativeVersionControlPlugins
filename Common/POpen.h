#pragma once
#if defined(_WINDOWS)
#include <Windows.h>
#endif

// Run a command line read result.
// This is done blocking which is ok since Unity will timeout
class POpen
{
public:
	POpen(const std::string& cmd);
	~POpen();
	bool ReadLine(std::string& result);
	void ReadIntoFile(const std::string& path);
private:
	std::string m_Command;
#if defined(_WINDOWS)
	HANDLE m_ChildStd_IN_Rd;
	HANDLE m_ChildStd_IN_Wr;
	HANDLE m_ChildStd_OUT_Rd;
	HANDLE m_ChildStd_OUT_Wr;
	PROCESS_INFORMATION m_ProcInfo; 
	CHAR m_Buf[4096];
	DWORD m_BufSize;

#else // posix
	FILE* m_Handle;
#endif
};

typedef std::auto_ptr<POpen> APOpen;
