#pragma once
#include "clientapi.h"
#include "Status.h"
#include "Connection.h"
#include "P4Info.h"
#include "P4Stream.h"

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
	std::string GetP4Port();
	void SetP4User(const std::string& u);
	std::string GetP4User();
	void SetP4Client(const std::string& c);
	std::string GetP4Client();
	void SetP4Password(const std::string& p);
	const std::string& GetP4Password() const;
	void SetP4Root(const std::string& r);
	const std::string& GetP4Root() const;
	void SetProjectPath(const std::string& p);
	const std::string& GetProjectPath() const;
	void SetP4Info(const P4Info& info);
	const P4Info& GetP4Info() const;
	void SetP4Streams(const P4Streams& s);
	const P4Streams& GetP4Streams() const;

	int Run(const bool testmode);
	bool IsConnected();
	bool Reconnect();
	bool Login();

	// Run a single command and write response to stdout. Will (re)connect and (re)login if needed.
	// Returns true on success
	bool CommandRun( const std::string& command, P4Command* client );

	// Same as above but does not do any connect and login.
	bool CommandRunNoLogin( const std::string& command, P4Command* client );

	bool Disconnect();

	static void NotifyOffline(const std::string& reason);
	static void NotifyOnline();

	// Set but do not notify unity about it
	static void SetOnline(bool isOnline);
	static bool IsOnline();
	void Logout();
	void DisableUTF8Mode();

private:

	bool Dispatch(UnityCommand c, const std::vector<std::string>& args);

	void EnableUTF8Mode();
	static bool ShowOKCancelDialogBox(const std::string& windowTitle, const std::string& message);

	bool HasUnicodeNeededError(VCSStatus status);
	bool HasServerFingerPrintError(VCSStatus status);
	bool IsLoggedIn();


	bool m_IsOnline;
	bool m_IsLoginInProgress;
	bool m_IsTestMode;

	// Perforce connection
	bool            m_P4Connect;
	ClientApi       m_Client;
	StrBuf          m_Spec;
	std::string		m_Root;
	P4Info          m_Info;
	P4Streams       m_Streams;

	std::string m_PortConfig;
	std::string m_UserConfig;
	std::string m_ClientConfig;
	std::string m_PasswordConfig;
	std::string m_ProjectPathConfig;
	std::string m_OfflineReason;

	// Command execution
	std::string m_CommandOutput;

	Connection* m_Connection;

	friend class P4Command;
	static P4Task* s_Singleton;
};

