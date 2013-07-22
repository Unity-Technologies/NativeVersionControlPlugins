#include "P4Task.h"
#include "P4Command.h"
#include "error.h"
#include "i18napi.h"
#include "msgclient.h"
#include "msgserver.h"
#include "CommandLine.h"
#include "Utility.h"
#include "FileSystem.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <map>
#include <vector>

#if defined(_WINDOWS)
#include "windows.h"
// Windows converts the perforce ClientApi.SetPort into
// ClientApi.SetPortW because of its auto widestring
// function renaming magic. Fix this by undef'ing the
// colliding standard define.
#undef SetPort
#endif

using namespace std;

VCSStatus errorToVCSStatus(Error& e)
{
	VCSSeverity sev = VCSSEV_Error;

	switch (e.GetSeverity()) {
		case E_EMPTY: 
			sev = VCSSEV_OK;
			break; // no errors
		case E_INFO: // information, not necessarily an error
			sev = VCSSEV_Info;
			break;
		case E_WARN: // a minor error occurred
		case E_FAILED: // the command was used incorrectly
			sev = VCSSEV_Warn;
			break;
		case E_FATAL:  // fatal error, the command can't be processed
			sev = VCSSEV_Error;
			break;
		default:
			break;
	}

	if (e.CheckId(MsgClient::ClobberFile)) // TODO: Check if clobberfile is severity failed and remove if so
		sev = VCSSEV_Warn;
		
	StrBuf msg;
	e.Fmt(&msg);
	VCSStatus status;
	string msgStr = msg.Text();
	if (!msgStr.empty() || sev != VCSSEV_OK)
		status.insert(VCSStatusItem(sev, msgStr));
	return status;
}

P4Task* P4Task::s_Singleton = NULL;

// This class essentially manages the command line interface to the API and replies.  Commands are read from stdin and results
// written to stdout and errors to stderr.  All text based communications used tags to make the parsing easier on both ends.
P4Task::P4Task()
{
    m_P4Connect = false;
	m_IsOnline = false;
	s_Singleton = this;
}

P4Task::~P4Task()
{
	Disconnect();
}

void P4Task::SetP4Port(const string& p)
{ 
	m_Client.SetPort(p.c_str()); 
	m_PortConfig = p;
	m_IsOnline = false;
}

string P4Task::GetP4Port() const
{
	return m_PortConfig.empty() ? string("perforce:1666") : m_PortConfig;
}

void P4Task::SetP4User(const string& u)
{ 
	m_Client.SetUser(u.c_str()); 
	m_UserConfig = u;
	m_IsOnline = false;
}

string P4Task::GetP4User()
{
	return m_Client.GetUser().Text();
}

void P4Task::SetP4Client(const string& c)
{
	m_Client.SetClient(c.c_str()); 
	m_ClientConfig = c;
	m_IsOnline = false;
}

string P4Task::GetP4Client()
{
	return m_Client.GetClient().Text(); 
}

void P4Task::SetP4Password(const string& p)
{
	if (p.empty())
	{
		m_Client.SetIgnorePassword();
	}
	else
	{
		m_Client.SetPassword(p.c_str());
	}
	m_PasswordConfig = p;
	m_IsOnline = false;
}

const string& P4Task::GetP4Password() const
{
	return m_PasswordConfig;
}

const string& P4Task::GetP4Root() const
{ 
	return m_Root; 
}

void P4Task::SetP4Root(const string& r) 
{ 
	m_Root = r; 
}

void P4Task::SetProjectPath(const std::string& p)
{
	if (p != m_ProjectPathConfig)
	{
		m_ProjectPathConfig = p;
		ChangeCWD(p);
	}
	m_IsOnline = false;
}

const std::string& P4Task::GetProjectPath() const
{
  return m_ProjectPathConfig;
}

int P4Task::Run()
{
	m_Connection = new Connection("./Library/p4plugin.log");


	try 
	{
		UnityCommand cmd;
		vector<string> args;
		
		for ( ;; )
		{
			cmd = m_Connection->ReadCommand(args);
			
			// Make it convenient to get the pipe even though the commands
			// are callback based.
			P4Command::s_Conn = m_Connection;
			
			if (cmd == UCOM_Invalid)
				return 1; // error
			else if (cmd == UCOM_Shutdown)
				return 0; // ok 
			else if (!Dispatch(cmd, args))
				return 0; // ok
		}
	} 
	catch (exception& e)
	{
		m_Connection->Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
	}
	return 1;
}

bool P4Task::Dispatch(UnityCommand cmd, const std::vector<string>& args)
{
	// Simple hack to test custom commands
	if (cmd == UCOM_CustomCommand)
	{
		m_Connection->WarnLine(string("You called the custom command ") + args[1]);
		m_Connection->EndResponse();
		return true;
	}
	
	// Dispatch
	P4Command* p4c = LookupCommand(UnityCommandToString(cmd));
	if (!p4c)
	{
		throw CommandException(cmd, string("unknown command"));
	}
	
	if (!p4c->Run(*this, args))
		return false;

	return true;
}

// Initialize the perforce client
bool P4Task::Connect()
{   
	// If connection is still active then return success
    if ( IsConnected() )
		return Login();

	// Disconnect client if marked as connected and return error
	// if not possible
    if ( m_P4Connect && !Disconnect()) return false;

    m_Error.Clear();
	m_Client.SetProg( "Unity" );
    m_Client.SetVersion( "1.0" );

	// Set the config because in case of reconnect the 
	// config has been reset
	SetP4Root("");
	m_Client.SetPort(GetP4Port().c_str()); 
	m_Client.SetUser(m_UserConfig.c_str());
	if (m_PasswordConfig.empty())
		m_Client.SetIgnorePassword();
	else
		m_Client.SetPassword(m_PasswordConfig.c_str());
	m_Client.SetClient(m_ClientConfig.c_str());
	
	m_Client.Init( &m_Error );
	
	VCSStatus status = errorToVCSStatus(m_Error);

	// Retry in case of unicode needs to be enabled on client	
	if (HasUnicodeNeededError(status))
	{
		m_Error.Clear();
		EnableUTF8Mode();

		m_Client.Init( &m_Error );
		VCSStatus status = errorToVCSStatus(m_Error);
	}

	if (status.size())
	{
		if (P4Command::HandleOnlineStatusOnError(&m_Error))
			SendToConnection(*m_Connection, status, MAProtocol);
	}

	if( m_Error.Test() )
	    return false;

	m_P4Connect = true;

	// We enforce ticket based authentication since that
	// is supported on every security level.
	return Login();
}

void P4Task::NotifyOffline(const string& reason)
{
	const char* disableCmds[]  = { 
		"add", "changeDescription", "changeMove",
		"changes", "changeStatus", "checkout",
		"deleteChanges", "delete", "download",
		"getLatest", "incomingChangeAssets", "incoming",
		"lock", "move", "resolve",
		"revertChanges", "revert", "status",
		"submit", "unlock", 
		0
	};

	s_Singleton->m_IsOnline = false;

	int i = 0;
	while (disableCmds[i])
	{
		s_Singleton->m_Connection->Command(string("disableCommand ") + disableCmds[i], MAProtocol);
		++i;
	}
	s_Singleton->m_Connection->Command(string("offline ") + reason, MAProtocol);
}

void P4Task::NotifyOnline()
{
	const char* enableCmds[]  = { 
		"add", "changeDescription", "changeMove",
		"changes", "changeStatus", "checkout",
		"deleteChanges", "delete", "download",
		"getLatest", "incomingChangeAssets", "incoming",
		"lock", "move", "resolve",
		"revertChanges", "revert", "status",
		"submit", "unlock", 
		0
	};
	if (s_Singleton->m_IsOnline)
		return;

	s_Singleton->m_Connection->Command("online", MAProtocol);
	int i = 0;
	while (enableCmds[i])
	{
		s_Singleton->m_Connection->Command(string("enableCommand ") + enableCmds[i], MAProtocol);
		++i;
	}
	s_Singleton->m_IsOnline = true;
}

void P4Task::SetOnline(bool isOnline)
{
	s_Singleton->m_IsOnline = isOnline;
}

bool P4Task::IsOnline()
{
	return s_Singleton->m_IsOnline;
}

bool P4Task::Login()
{	
	// First check if we're already logged in
	P4Command* p4c = LookupCommand("login");
	vector<string> args;
	args.push_back("login");
	args.push_back("-s");
	bool loggedIn = p4c->Run(*this, args); 

	if (HasUnicodeNeededError(p4c->GetStatus()))
	{
		m_Connection->InfoLine("Enabling unicode mode");
		EnableUTF8Mode();
		loggedIn = p4c->Run(*this, args);
	}

	SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);
	
	if (loggedIn)
	{
		return true; // All is fine. We're already logged in
	}

	if (GetP4Password().empty())
	{
		m_Connection->Log().Debug() << "Empty password -> skipping login" << Endl;
		return true;
	}

	// Do the actual login
	args.clear();
	args.push_back("login");
	loggedIn = p4c->Run(*this, args); 
	
	if (HasUnicodeNeededError(p4c->GetStatus()))
	{
		EnableUTF8Mode();
		loggedIn = p4c->Run(*this, args);
	}

	SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);

	if (!loggedIn)
	{
		NotifyOffline("Login failed");
		return false; // error login
	}

	if (GetP4Root().empty())
	{
		// Need to get Root path as the first thing on connect
		p4c = LookupCommand("spec");
		vector<string> args;
		args.push_back("spec");
		bool res = p4c->Run(*this, args); // fetched root info
		SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);
		if (!res)
			NotifyOffline("Couldn't fetch client spec file from perforce server");
		return res;
	}
	return true; // root reused
}

void P4Task::Logout()
{
	m_IsOnline = false;

	P4Command* p4c = LookupCommand("logout");
	CommandArgs args;
	p4c->Run(*this, args); 
}

// Finalise the perforce client 
bool P4Task::Disconnect()
{
    m_Error.Clear();

	m_IsOnline = false;

    if ( !m_P4Connect ) // Nothing to do?
	{
		return true;
	}

	m_Client.Final( &m_Error );
    m_P4Connect = false;

	// NotifyOffline("Disconnected");

	VCSStatus status = errorToVCSStatus(m_Error);
	SendToConnection(*m_Connection, status, MAProtocol);

	if( m_Error.Test() )
	    return false;

    return true;
}

// Return the "connected" state
bool P4Task::IsConnected()
{
    return (m_P4Connect && !m_Client.Dropped());
}

// Run a perforce command
bool P4Task::CommandRun(const string& command, P4Command* client)
{
	
	m_Connection->Log().Info() << command << Endl;
	m_Connection->VerboseLine(command);

	// Force connection if this hasn't been set-up already.
	// That is unless the command explicitely disallows connect.
	if (!client->ConnectAllowed() || Connect())
	{
		// Split out the arguments
		int argc = 0;
		char** argv = CommandLineToArgv( command.c_str(), &argc );

		if ( argv == 0 || argc == 0 )
			return "No perforce command was passed";

		if ( argc > 1 )
			m_Client.SetArgv( argc-1, &argv[1] );

		m_Client.Run(argv[0], client);
		CommandLineFreeArgs(argv);
	}
	return !client->HasErrors();
}

bool P4Task::HasUnicodeNeededError( VCSStatus status )
{
	return StatusContains(status, "Unicode server permits only unicode enabled clients");
}

void P4Task::EnableUTF8Mode()
{
	CharSetApi::CharSet cs = CharSetApi::UTF_8;
	m_Client.SetTrans( cs, cs, cs, cs );
	m_Client.SetCharset("utf8");
}

void P4Task::DisableUTF8Mode()
{
	m_Client.SetCharset("");
	CharSetApi::CharSet cs = CharSetApi::NOCONV;
	m_Client.SetTrans( cs, -2, -2, -2 );
}



