#pragma once
#include "clientapi.h"
#include <stdio.h>
#include <string>
#include <set>

class P4Command;
enum P4Severity
{
	P4SEV_OK,
	P4SEV_Info,
	P4SEV_Warn,
	P4SEV_Error
};

const char* p4SeverityToString(P4Severity s);

struct P4StatusItem
{
	P4StatusItem(P4Severity sev, const std::string& msg) : severity(sev), message(msg) { }
	P4Severity severity;
	std::string message;
};

// Sort higher severity as before when iterating a set
struct P4StatusItemCmp 
{
	bool operator()(const P4StatusItem& a, const P4StatusItem& b)
	{
		if (a.severity > b.severity)
			return true;
		return a.message < b.message;
	}
};

typedef std::set<P4StatusItem, P4StatusItemCmp> P4Status;
P4Status errorToP4Status(Error& e);

// This class essentially manages the command line interface to the API and replies.  Commands are read from stdin and results
// written to stdout and errors to stderr.  All text based communications used tags to make the parsing easier on both ends.
class P4Task
{
public:
	P4Task();
	virtual ~P4Task();

	// Setup
	void SetP4Port(const std::string& p);
	void SetP4User(const std::string& u);
	std::string GetP4User();
	void SetP4Client(const std::string& c);
	std::string GetP4Client();
	void SetP4Password(const std::string& p);
	const std::string& GetP4Password() const;
	void SetP4Root(const std::string& r);
	const std::string& GetP4Root() const;
	void SetAssetsPath(const std::string& p);
	const std::string& GetAssetsPath() const;
	
	bool IsConnected();

	// Class is a black box P4 server interface
	// Listens to commands in stdin and writes responses to stdout
	int Run();

	// Run a single command and write response to stdout
	// Returns true on success
	bool CommandRun( const std::string& command, P4Command* client );

private:

	// Connection
	bool Connect(P4Status& result);
	bool Login(P4Status& result);
	bool Disconnect(P4Status& result);


	// Perforce connection
	bool            m_P4Connect;
	ClientApi       m_Client;
	StrBuf          m_Spec;
	std::string		m_Root;
	Error			m_Error;

	std::string m_PortConfig;
	std::string m_UserConfig;
	std::string m_ClientConfig;
	std::string m_PasswordConfig;
	std::string m_AssetsPathConfig;
	
	// Command execution
	bool CommandRead();
	std::string m_CommandOutput;
	
	friend class P4Command;
};

