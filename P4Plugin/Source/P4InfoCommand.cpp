#include "P4Command.h"
#include "P4Task.h"
#include <sstream>

static const char* kClientName = "Client name: ";
static const char* kClientUnknown = "Client unknown.";
static const char* kCurrentDir = "Current directory: ";
static const char* kPeerAddr = "Peer address: ";
static const char* kClientAddr = "Client address: ";
static const char* kServerAddr = "Server address: ";
static const char* kServerRoot = "Server root: ";
static const char* kServerVersion = "Server version: ";
static const char* kServerLicense = "Server license: ";
static const char* kCaseHandling = "Case Handling: ";

class P4InfoCommand : public P4Command
{
public:
	P4InfoCommand(const char* name) : P4Command(name) { }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_Info = P4Info();
		m_Info.clientIsKnown = true;

		if (!task.CommandRun("info", this))
		{
			std::string errorMessage = GetStatusMessage();			
			Conn().Log().Fatal() << errorMessage << Endl;
			return false;
		}
		else
		{
			task.SetP4Info(m_Info);
			return true;
		}
	}
	
	bool ExtractInfo(const std::string& line, const char* prefix, std::string& dest)
	{
		if (StartsWith(line, prefix))
		{
			dest = line.substr(strlen(prefix));
			return true;
		}
		return false;
	}

    // Called with entire spec file as data
	void OutputInfo( char level, const char *data )
    {
		std::stringstream ss(data);
		Conn().VerboseLine(data);
		size_t minlen = 5; // "Root:" 
		
		std::string line;
		while ( getline(ss, line) ) 
		{	
			if (ExtractInfo(line, kClientName, m_Info.clientName)) continue;
			if (ExtractInfo(line, kCurrentDir, m_Info.currentDir)) continue;
			if (ExtractInfo(line, kPeerAddr, m_Info.peerAddress)) continue;
			if (ExtractInfo(line, kClientAddr, m_Info.clientAddress)) continue;
			if (ExtractInfo(line, kServerAddr, m_Info.serverAddress)) continue;
			if (ExtractInfo(line, kServerRoot, m_Info.serverRoot)) continue;
			if (ExtractInfo(line, kServerVersion, m_Info.serverVersion)) continue;
			if (ExtractInfo(line, kServerLicense, m_Info.serverLicense)) continue;
			if (ExtractInfo(line, kCaseHandling, m_Info.caseHandling)) continue;
			if (StartsWith(line, kClientUnknown)) 
			{
				m_Info.clientIsKnown = false;
				continue;
			}
		}
	}

private:
	P4Info m_Info;
	
} cInfo("info");
