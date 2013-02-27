#pragma once
#include "clientapi.h"
#include "Status.h"
#include "Task.h"

#include <stdio.h>

class P4Command;
VCSStatus errorToVCSStatus(Error& e);

// This class essentially manages the command line interface to the API and replies.  Commands are read from stdin and results
// written to stdout and errors to stderr.  All text based communications used tags to make the parsing easier on both ends.
class P4Task
{
public:
	P4Task();
	~P4Task();

	// Setup
	void SetP4Port(const std::string& p);
	std::string GetP4Port() const;
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
	
	int Run();
	bool IsConnected();

	// Run a single command and write response to stdout
	// Returns true on success
	bool CommandRun( const std::string& command, P4Command* client );

private:

	bool Dispatch(UnityCommand c, const std::vector<std::string>& args);

	// Connection
	bool Connect(VCSStatus& result);
	bool Login(VCSStatus& result);
	bool Disconnect(VCSStatus& result);


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
	std::string m_CommandOutput;

	Task* m_Task;

	friend class P4Command;
};
