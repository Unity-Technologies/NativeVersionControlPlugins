#include "P4Task.h"
#include "P4Command.h"
#include "error.h"
#include "msgclient.h"
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
			sev = VCSSEV_Warn;
			break;
		case E_FAILED: // the command was used incorrectly
		case E_FATAL:  // fatal error, the command can't be processed
			sev = VCSSEV_Error;
			break;
		default:
			break;
	}

	StrBuf msg;
	e.Fmt(&msg);
	VCSStatus status;
	status.insert(VCSStatusItem(sev, msg.Text()));
	return status;
}

// This class essentially manages the command line interface to the API and replies.  Commands are read from stdin and results
// written to stdout and errors to stderr.  All text based communications used tags to make the parsing easier on both ends.
P4Task::P4Task()
	: m_Connection("./vcsplugin.log")
{
    m_P4Connect = false;
}

P4Task::~P4Task()
{
	VCSStatus dummy;
	Disconnect(dummy);
}

void P4Task::SetP4Port(const string& p)
{ 
	m_Client.SetPort(p.c_str()); 
	m_PortConfig = p;
}

void P4Task::SetP4User(const string& u)
{ 
	m_Client.SetUser(u.c_str()); 
	m_UserConfig = u;
}

string P4Task::GetP4User()
{ 
	return m_Client.GetUser().Text();
}

void P4Task::SetP4Client(const string& c)
{ 
	m_Client.SetClient(c.c_str()); 
	m_ClientConfig = c;
}

string P4Task::GetP4Client()
{ 
	return m_Client.GetClient().Text(); 
}

void P4Task::SetP4Password(const string& p)
{ 
	m_Client.SetPassword(p.c_str()); 
	m_PasswordConfig = p;
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


void P4Task::SetAssetsPath(const std::string& p)
{
	m_AssetsPathConfig = p;
}

const std::string& P4Task::GetAssetsPath() const
{
  return m_AssetsPathConfig;
}

// Main run and processing loop
int P4Task::Run()
{
	try
	{
		m_Connection.Connect();
	} catch (exception& e)
	{
		m_Connection.Log() << e.what() << std::endl;
		return 0;
	}

	// Make it convenient to get the pipe even though the commands
	// are callback based.
	P4Command::s_UnityPipe = &m_Connection.Pipe();

	// Read commands
	UnityCommand cmd;
	vector<string> args;
	for ( ;; )
	{
		// Read
		try 
		{
			cmd = m_Connection.ReadCommand(args);
		} 
		catch (CommandException& ce)
		{
			m_Connection.Pipe().ErrorLine(string("invalid command - ") + Join(args));
			return 1;
		}

		// Dispatch
		P4Command* p4c = LookupCommand(UnityCommandToString(cmd));
		if (!p4c)
		{
			m_Connection.Pipe().ErrorLine(string("unknown command - ") + UnityCommandToString(cmd));
			return 1;
		}
		
		if (!p4c->Run(*this, args))
			break;
	}
    return 0;
}

// Initialise the perforce client
bool P4Task::Connect(VCSStatus& result)
{   
	// If connection is still active then return success
    if ( IsConnected() )
		return Login(result);

	// Disconnect client if marked as connected and return error
	// if not possible
    if ( m_P4Connect && !Disconnect(result)) return false;

    m_Error.Clear();
	m_Client.SetProg( "Unity" );
    m_Client.SetVersion( "1.0" );

	// Set the config because in case of reconnect the 
	// config has been reset
	SetP4Root("");
	m_Client.SetPort(m_PortConfig.c_str());
	m_Client.SetUser(m_UserConfig.c_str());
	m_Client.SetPassword(m_PasswordConfig.c_str());
	m_Client.SetClient(m_ClientConfig.c_str());
	m_Client.Init( &m_Error );
	
	VCSStatus status = errorToVCSStatus(m_Error);
	result.insert(status.begin(), status.end());

	if( m_Error.Test() )
	    return false;

	m_P4Connect = true;

	// We enforce ticket based authentication since that
	// is supported on every security level.
	return Login(result);
}

bool P4Task::Login(VCSStatus& result)
{	
	// First check if we're already logged in
	P4Command* p4c = LookupCommand("login");
	vector<string> args;
	args.push_back("login");
	args.push_back("-s");
	bool loggedIn = p4c->Run(*this, args); 
	result.insert(p4c->GetStatus().begin(), p4c->GetStatus().end());
	if (loggedIn) 
		return true; // All is fine. We're already logged in

	// Do the actual login
	args.clear();
	args.push_back("login");
	loggedIn = p4c->Run(*this, args); 
	result.insert(p4c->GetStatus().begin(), p4c->GetStatus().end());
	if (!loggedIn)
		return false; // error login
	
	if (GetP4Root().empty())
	{
		// Need to get Root path as the first thing on connect
		p4c = LookupCommand("spec");
		vector<string> args;
		args.push_back("spec");
		bool res = p4c->Run(*this, args); // fetched root info
		result.insert(p4c->GetStatus().begin(), p4c->GetStatus().end());
		return res;
	}
	return true; // root reused
}

// Finalise the perforce client 
bool P4Task::Disconnect(VCSStatus& result)
{
    m_Error.Clear();

    if ( !m_P4Connect ) // Nothing to do?
        return true;

	m_Client.Final( &m_Error );
    m_P4Connect = false;

	VCSStatus status = errorToVCSStatus(m_Error);
	result.insert(status.begin(), status.end());

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
	m_Connection.Pipe().Log() << command << endl;
	
	// Force connection if this hasn't been set-up already.
	// That is unless the command explicitely disallows connect.
	if (!client->ConnectAllowed() || Connect(client->GetStatus()))
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
