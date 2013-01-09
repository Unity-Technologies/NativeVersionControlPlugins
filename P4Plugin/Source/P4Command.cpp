#include "P4StatusCommand.h"

#include <map>
#include <cassert>

using namespace std;

UnityPipe& operator<<(UnityPipe& p, const P4Status& st)
{
	// Convertion of p4 errors to unity vcs errors
	/*
	p.Log() << "HandleError: " 
		<< e.SubCode() << " " 
		<< e.Subsystem() << " " 
		<< e.Generic() << " " 
		<< e.ArgCount() << " " 
		<< e.Severity() << " " 
		<< e.UniqueCode() << " " 
		<< e.fmt << endl;
	 */
	
	for (P4Status::const_iterator i = st.begin(); i != st.end(); ++i)
	{
		switch (i->severity)
		{
		case P4SEV_OK: 
			p.OkLine(i->message);
			break;
		case P4SEV_Info:
			p.InfoLine(i->message);
			break;
		case P4SEV_Warn:
			p.WarnLine(i->message);
			break;
		case P4SEV_Error:
			p.ErrorLine(i->message);
			break;
		default:
			p.ErrorLine(string("<Unknown errortype>: ") + i->message);
		}
	}
	return p;
}


// Global map of all commands registered at initialization time
typedef std::map<string, P4Command*> CommandMap;
static CommandMap* s_Commands = NULL;

P4Command* LookupCommand(const string& name)
{
	assert(s_Commands != NULL);
	CommandMap::iterator i = s_Commands->find(name);
	if (i == s_Commands->end()) return NULL;
	return i->second;
}


// Communication with unity process and logging
UnityPipe P4Command::upipe("./p4plugin.log");


P4Command::P4Command(const char* name) : m_AllowConnect(true)
{
	if (s_Commands == NULL)
		s_Commands = new CommandMap();

	s_Commands->insert(make_pair(name,this));
}


const P4Status& P4Command::GetStatus() const
{ 
	return m_Status; 
}

P4Status& P4Command::GetStatus()
{ 
	return m_Status; 
}

bool P4Command::HasErrors() const
{ 
	return !m_Status.empty() && m_Status.begin()->severity == P4SEV_Error;
}

void P4Command::ClearStatus() 
{ 
	m_Status.clear(); 
}


string P4Command::GetStatusMessage() const
{
	string delim = "";
	string msg;
	for (P4Status::const_iterator i = m_Status.begin(); i != m_Status.end(); ++i)
	{
		msg += p4SeverityToString(i->severity);
		msg += ": ";
		msg += i->message;
		msg += delim;
		delim = "\n";
	}
	return msg;
}

bool P4Command::ConnectAllowed()
{
	return m_AllowConnect;
}

// Default handler of P4
void P4Command::OutputStat( StrDict *varList ) 
{ 
	upipe.Log() << "Default ClientUser OutputState()\n";
}


// Default handler of P4
void P4Command::InputData( StrBuf *buf, Error *err ) 
{ 
	upipe.Log() << "Default ClientUser InputData()\n";
}

void P4Command::Prompt( const StrPtr &msg, StrBuf &buf, int noEcho ,Error *e )
{
	upipe.Log() << "Default ClientUser Prompt(" << msg.Text() << ")\n";
}


// Default handler of P4
void P4Command::Finished() 
{ 
//	upipe.Log() << "Default ClientUser Finished()\n";
}


// Default handler of P4 error output. Called by the default P4Command::Message() handler.
void P4Command::HandleError( Error *err )
{
	if ( err == 0 )
		return;
	
	P4Status s = errorToP4Status(*err);
	m_Status.insert(s.begin(), s.end());

	// Base implementation. Will callback to P4Command::OutputError 
	ClientUser::HandleError( err );
}

// Default handler of perforce error calbacks
void P4Command::OutputError( const char *errBuf )
{
	upipe.Log() << errBuf << "\n";
}

void P4Command::ErrorPause( char* errBuf, Error* e)
{
	upipe.Log() << "Error: Default ClientUser ErrorPause()\n";
}


void P4Command::OutputText( const char *data, int length)
{
	upipe.Log() << "Error: Default ClientUser OutputText\n";
}


void P4Command::OutputBinary( const char *data, int length)
{
	upipe.Log() << "Error: Default ClientUser OutputBinary\n";
}


// Default handle of perforce info callbacks. Called by the default P4Command::Message() handler.
void P4Command::OutputInfo( char level, const char *data )
{
	upipe.Log() << "level " << (int) level << ": " << data << endl;
}

P4Command* P4Command::RunAndSendStatus(P4Task& task, const VersionedAssetList& assetList)
{
	P4StatusCommand* c = dynamic_cast<P4StatusCommand*>(LookupCommand("status"));
	if (!c)
	{
		upipe.ErrorLine("Cannot locate status command");
		return this; // Returning this is just to keep things running.
	}
	
	bool recursive = false;
	c->RunAndSend(task, assetList, recursive);
	return c;
}

