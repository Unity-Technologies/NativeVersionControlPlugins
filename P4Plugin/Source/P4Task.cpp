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
#include <stdlib.h>

#if defined(_WINDOWS)
#include "windows.h"
// Windows converts the perforce ClientApi.SetPort into
// ClientApi.SetPortW because of its auto widestring
// function renaming magic. Fix this by undef'ing the
// colliding standard define.
#undef SetPort
#endif

//This uses 'p4 where' to check the location of a file that almost certainly doesn't exist - it's purpose is to check whether the project root is mapped in the client's workspace
class P4CheckRootCommand : public P4Command
{
public:
	P4CheckRootCommand(const char* name) : P4Command(name) { }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();

		if (!task.CommandRun("where \"./testForProjectRootMapping\"", this))
		{
			std::string errorMessage = GetStatusMessage();
			Conn().Log().Fatal() << errorMessage << Endl;
			return false;
		}
		else
		{
			return true;
		}
	}
} cCheckRoot("checkroot");

// Clean up messages to make it look nicer in Unity
// Return true is a line was removed
static bool ReplaceLineWithPrefix(std::string& m, const std::string& prefix, const std::string& replaceWith)
{
	if (!StartsWith(m, prefix))
		return false;
	std::string::size_type i = m.find('\n');
	if (i == std::string::npos)
		return false;
	m.replace(0, i+1, replaceWith);
	return true;
}

static bool RemoveLineWithPrefix(std::string& m, const std::string& prefix)
{
	return ReplaceLineWithPrefix(m, prefix, "");
}

static void CleanupErrorMessage(std::string& m)
{
	RemoveLineWithPrefix(m, "Submit aborted -- fix problems then use 'p4 submit -c") ||
	ReplaceLineWithPrefix(m, 
						  "Merges still pending -- use 'resolve' to merge files.",
						  "Merges still pending. Please resolve and resubmit.")
						  ;
}

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
		case E_WARN:   // a minor error occurred
			sev = VCSSEV_Warn;
			break;
		case E_FAILED: // the command was used incorrectly
		case E_FATAL:  // fatal error, the command can't be processed
			sev = VCSSEV_Error;
			break;
		default:
			break;
	}

	if (e.CheckId(MsgClient::ClobberFile)) // TODO: Check if clobberfile is severity failed and remove if so
		sev = VCSSEV_Error;
		
	StrBuf msg;
	e.Fmt(&msg);
	VCSStatus status;
	std::string msgStr = msg.Text();

	CleanupErrorMessage(msgStr);
	
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
	m_IsLoginInProgress = false;
	s_Singleton = this;
	SetOnline(false);
}

P4Task::~P4Task()
{
	Disconnect();
}

void P4Task::SetP4Port(const std::string& p)
{ 
	m_Client.SetPort(p.c_str()); 
	m_PortConfig = p;
	SetOnline(false);
}

void P4Task::SetP4User(const std::string& u)
{ 
	m_Client.SetUser(u.c_str()); 
	m_UserConfig = u;
	SetOnline(false);
}

std::string P4Task::GetP4User()
{
	return m_Client.GetUser().Text();
}

void P4Task::SetP4Client(const std::string& c)
{
	m_Client.SetClient(c.c_str()); 
	m_ClientConfig = c;
	SetOnline(false);
}

std::string P4Task::GetP4Client()
{
	return m_Client.GetClient().Text(); 
}

void P4Task::SetP4Password(const std::string& p)
{
	if (p.empty())
	{
		char* pw = getenv("P4PASSWD");
		if (pw == NULL)
		{
			m_Client.SetIgnorePassword();
			m_PasswordConfig.clear();
		}
		else
		{
			m_PasswordConfig = pw;
		}
	}
	else
	{
		m_Client.SetPassword(p.c_str());
		m_PasswordConfig = p;
	}
	SetOnline(false);
}

const std::string& P4Task::GetP4Password() const
{
	return m_PasswordConfig;
}

void P4Task::SetP4Host(const std::string& c)
{
	m_Client.SetHost(c.c_str()); 
	m_HostConfig = c;
	SetOnline(false);
}

std::string P4Task::GetP4Host()
{
	return m_Client.GetHost().Text(); 
}

const std::string& P4Task::GetP4Root() const
{ 
	return m_Root; 
}

void P4Task::SetP4Root(const std::string& r) 
{ 
	m_Root = r; 
}

void P4Task::SetProjectPath(const std::string& p)
{
	if (p != m_ProjectPathConfig)
	{
		m_ProjectPathConfig = p;
		ChangeCWD(p);
		m_Client.SetCwd(p.c_str());
	}
	SetOnline(false);
}

const std::string& P4Task::GetProjectPath() const
{
  return m_ProjectPathConfig;
}

void P4Task::SetP4Info(const P4Info& info)
{
	m_Info = info;
}

const P4Info& P4Task::GetP4Info() const
{
	return m_Info;
}

void P4Task::SetP4Streams(const P4Streams& s)
{
	m_Streams = s;
}

const P4Streams& P4Task::GetP4Streams() const
{
	return m_Streams;
}

int P4Task::Run()
{
	m_Connection = new Connection("./Library/p4plugin.log");


	try 
	{
		UnityCommand cmd;
		std::vector<std::string> args;
		
		for ( ;; )
		{
			cmd = m_Connection->ReadCommand(args);
			
			// Make it convenient to get the pipe even though the commands
			// are callback based.
			P4Command::s_Conn = m_Connection;
			
			if (cmd == UCOM_Invalid)
				return 1; // error
			else if (cmd == UCOM_Shutdown)
			{
				m_Connection->EndResponse(); // good manner shutdown
				return 0; // ok 
			}
			else if (!Dispatch(cmd, args))
				return 0; // ok
		}
	}
	catch (std::exception& e)
	{
		m_Connection->Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
	}
	return 1;
}

bool P4Task::Dispatch(UnityCommand cmd, const std::vector<std::string>& args)
{
	// Simple hack to test custom commands
	if (cmd == UCOM_CustomCommand)
	{
		m_Connection->WarnLine(std::string("You called the custom command ") + args[1]);
		m_Connection->EndResponse();
		return true;
	}
	
	// Dispatch
	P4Command* p4c = LookupCommand(UnityCommandToString(cmd));
	if (!p4c)
	{
		throw CommandException(cmd, std::string("unknown command"));
	}
	
	if (!p4c->Run(*this, args))
		return false;

	return true;
}

bool P4Task::Reconnect()
{
	Disconnect();
	m_OfflineReason.clear();
	// Ignore invalid configurations: empty server, empty username, empty workspace
	if (m_PortConfig.empty())
	{
		NotifyOffline("Perforce server and port not set. Ignoring connection request.");
		return false;
	}
	if (m_UserConfig.empty())
	{
		NotifyOffline("Perforce username not set. Ignoring connection request.");
		return false;
	}
	if (m_ClientConfig.empty())
	{
		NotifyOffline("Perforce workspace not set. Ignoring connection request.");
		return false;
	}
	
	Error err;
	m_Client.SetProg( "Unity" );
	m_Client.SetVersion( "1.0" );

	// Set the config because in case of reconnect the 
	// config has been reset
	SetP4Root("");
	m_Client.SetPort(m_PortConfig.c_str()); 
	m_Client.SetUser(m_UserConfig.c_str());
	if (m_PasswordConfig.empty())
		m_Client.SetIgnorePassword();
	else
		m_Client.SetPassword(m_PasswordConfig.c_str());
	m_Client.SetClient(m_ClientConfig.c_str());

	m_Client.Init( &err );

	VCSStatus status = errorToVCSStatus(err);

	// Retry in case of unicode needs to be enabled on client	
	if (HasUnicodeNeededError(status))
	{
		err.Clear();
		EnableUTF8Mode();

		m_Client.Init( &err );
		VCSStatus status = errorToVCSStatus(err);
	}

	if (status.size())
	{
		// If errors are not about connection becoming offline
		// send messages to Unity.
		if (P4Command::HandleOnlineStatusOnError(&err))
		{
			// Error was about vcs not online. Offline status
			// has been sent to Unity and logged.
			;
		}
		else
		{
			SendToConnection(*m_Connection, status, MAProtocol);
		}
	}

	if( err.Test() )
		return false;

	m_P4Connect = true;

	m_Client.SetVar("enableStreams");
	m_Client.SetProtocol("enableStreams", "");
	return true;
}

void P4Task::NotifyOffline(const std::string& reason)
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

	if (!s_Singleton->m_OfflineReason.empty() && !IsOnline())
		return;

	SetOnline(false);

	int i = 0;
	while (disableCmds[i])
	{
		s_Singleton->m_Connection->Command(std::string("disableCommand ") + disableCmds[i], MAProtocol);
		++i;
	}
	s_Singleton->m_Connection->Command(std::string("offline ") + reason, MAProtocol);
	s_Singleton->m_OfflineReason = reason;
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
	if (IsOnline())
		return;

	s_Singleton->m_Connection->Command("online", MAProtocol);
	int i = 0;
	while (enableCmds[i])
	{
		s_Singleton->m_Connection->Command(std::string("enableCommand ") + enableCmds[i], MAProtocol);
		++i;
	}
	SetOnline(true);
}

void P4Task::SetOnline(bool isOnline)
{
	s_Singleton->m_IsOnline = isOnline;
}

bool P4Task::IsOnline()
{
	return s_Singleton->m_IsOnline;
}

bool P4Task::IsLoggedIn()
{
	P4Command* p4c = LookupCommand("login");
	std::vector<std::string> args;
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
	return loggedIn;
}

bool P4Task::Login()
{	
	if (!IsConnected())
	{
		m_Connection->Log().Notice() << "Perforce server not connected. Ignoring login request." << Endl;
		return false;
	}

	m_IsLoginInProgress = true;
	P4Command* p4c = NULL;
	std::vector<std::string> args;
	
	SetOnline(true);

	if (GetP4Password().empty())
	{
		m_Connection->Log().Notice() << "Perforce password is empty. Ignoring login request." << Endl;
		bool loggedIn = IsLoggedIn();
		if (!loggedIn)
		{
			NotifyOffline("Login failed.");
			m_IsLoginInProgress = false;
			return false;
		}
	}
	else
	{
		// Do the actual login
		p4c = LookupCommand("login");
		
		args.push_back("login");
		bool loggedIn = p4c->Run(*this, args); 

		if (HasUnicodeNeededError(p4c->GetStatus()))
		{
			EnableUTF8Mode();
			loggedIn = p4c->Run(*this, args);
		}

		SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);	
		
		if (!loggedIn)
		{
			NotifyOffline("Login failed");
			m_IsLoginInProgress = false;
			return false; // error login
		}
	}

	// Need to get Root path as the first thing on connect
	p4c = LookupCommand("spec");
	args.clear();
	args.push_back("spec");
	bool res = p4c->Run(*this, args); // fetched root info
	SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);
	if (!res)
	{
		NotifyOffline("Couldn't fetch client spec file from Perforce server");
		m_IsLoginInProgress = false;
		return false;
	}

	p4c = LookupCommand("checkroot");
	args.clear();
	args.push_back("checkroot");
	res = p4c->Run(*this, args);
	SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);
	if (!res)
	{
		NotifyOffline("Couldn't fstat the project root directory. Please ensure that the selected workspace maps the project directory.");
		m_IsLoginInProgress = false;
		return false;
	}

	// Need to get Info
	p4c = LookupCommand("info");
	args.clear();
	args.push_back("info");
	res = p4c->Run(*this, args); // fetched root info
	SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);
	if (!res)
	{
		NotifyOffline("Couldn't fetch server info from Perforce server");
		m_IsLoginInProgress = false;
		return false;
	}

	// Need to get Streams if any
	p4c = LookupCommand("streams");
	args.clear();
	args.push_back("streams");
	res = p4c->Run(*this, args); // fetched root info
	SendToConnection(*m_Connection, p4c->GetStatus(), MAProtocol);
	if (!res)
	{
		NotifyOffline("Couldn't fetch client streams from perforce server");
		m_IsLoginInProgress = false;
		return false;
	}

	// Upon login check if the client is know to the server and if not
	// tell unity 
	if (GetP4Info().clientIsKnown)
	{
		; // TODO: do the stuff
	}

	SetOnline(false);
	NotifyOnline();
	m_IsLoginInProgress = false;

	return true; // root reused
}

void P4Task::Logout()
{
	NotifyOffline("Logging out");

	P4Command* p4c = LookupCommand("logout");
	CommandArgs args;
	p4c->Run(*this, args); 
}

// Finalise the perforce client 
bool P4Task::Disconnect()
{
	Error err;

	if (IsOnline())
		NotifyOffline("Disconnect");
	
	DisableUTF8Mode();
 
	//   if ( !m_P4Connect ) // Nothing to do?
	//{
	//	return true;
	//}

	m_Client.Final( &err );
	m_P4Connect = false;
	m_Info = P4Info();
	m_Streams = P4Streams();

	// NotifyOffline("Disconnected");

	VCSStatus status = errorToVCSStatus(err);
	SendToConnection(*m_Connection, status, MAProtocol);

	if( err.Test() )
	    return false;

    return true;
}

// Return the "connected" state
bool P4Task::IsConnected()
{
    return (m_P4Connect && !m_Client.Dropped());
}

// Run a perforce command
bool P4Task::CommandRun(const std::string& command, P4Command* client)
{
	if (m_Connection->Log().GetLogLevel() != LOG_DEBUG)
		m_Connection->Log().Info() << command << Endl;
	
	m_Connection->VerboseLine(command);

	m_OfflineReason.clear();
	if (IsConnected())
	{
		// Make sure we have not been logged out
		if (!m_IsLoginInProgress && !IsLoggedIn())
		{
			// relogin
			if (!Reconnect() || !Login())
				return false;
		}
	}
	else
	{
		// Make sure commands run as part of a login request does not reconnect. But all other commands
		// Should (re)establish the connection.
		if (m_IsLoginInProgress)
			return false; // This is an error since we are trying to login and has been disconnect while doing that
		else if (!Reconnect() || !Login())
			return false; // Cannot do any commands when not connected and logged in.
	}
	
	return CommandRunNoLogin(command, client);

}

bool P4Task::CommandRunNoLogin( const std::string &command, P4Command* client )
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
