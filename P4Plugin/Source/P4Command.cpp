#include "P4StatusCommand.h"

#include <map>
#include <cassert>
#include <sstream>

using namespace std;

UnityPipe& SendToPipe(UnityPipe& p, const VCSStatus& st, MessageArea ma, bool safeSend)
{
	// Convertion of p4 errors to unity vcs errors
	for (VCSStatus::const_iterator i = st.begin(); i != st.end(); ++i)
	{
		switch (i->severity)
		{
		case VCSSEV_OK: 
			if (safeSend)
				p.InfoLine(i->message, ma);
			else
				p.OkLine(i->message, ma);
			break;
		case VCSSEV_Info:
			p.InfoLine(i->message, ma);
			break;
		case VCSSEV_Warn:
			p.WarnLine(i->message, ma);
			break;
		case VCSSEV_Error:
			if (safeSend)
				p.WarnLine(i->message, ma);
			else
				p.ErrorLine(i->message, ma);
			break;
		case VCSSEV_Command: 
			p.Command(i->message, ma);
			break;
		default:
			p.ErrorLine(string("<Unknown errortype>: ") + i->message, ma);
		}
	}
	return p;
}

UnityPipe& operator<<(UnityPipe& p, const VCSStatus& st)
{
	return SendToPipe(p, st, MAGeneral, false);
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

UnityPipe* P4Command::s_UnityPipe = NULL;

P4Command::P4Command(const char* name) : m_AllowConnect(true)
{
	if (s_Commands == NULL)
		s_Commands = new CommandMap();

	s_Commands->insert(make_pair(name,this));
}


const VCSStatus& P4Command::GetStatus() const
{ 
	return m_Status; 
}

VCSStatus& P4Command::GetStatus()
{ 
	return m_Status; 
}

bool P4Command::HasErrors() const
{ 
	return !m_Status.empty() && m_Status.begin()->severity == VCSSEV_Error;
}

void P4Command::ClearStatus() 
{ 
	m_Status.clear(); 
}


string P4Command::GetStatusMessage() const
{
	string delim = "";
	string msg;
	for (VCSStatus::const_iterator i = m_Status.begin(); i != m_Status.end(); ++i)
	{
		msg += VCSSeverityToString(i->severity);
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
	Pipe().Log().Info() << "Default ClientUser OutputState()\n";
}


// Default handler of P4
void P4Command::InputData( StrBuf *buf, Error *err ) 
{ 
	Pipe().Log().Info() << "Default ClientUser InputData()\n";
}

void P4Command::Prompt( const StrPtr &msg, StrBuf &buf, int noEcho ,Error *e )
{
	Pipe().Log().Info() << "Default ClientUser Prompt(" << msg.Text() << ")\n";
}


// Default handler of P4
void P4Command::Finished() 
{ 
//	Pipe().Log().Info() << "Default ClientUser Finished()\n";
}


// Default handler of P4 error output. Called by the default P4Command::Message() handler.
void P4Command::HandleError( Error *err )
{
	if ( err == 0 )
		return;
	
	VCSStatus s = errorToVCSStatus(*err);
	m_Status.insert(s.begin(), s.end());

	// Base implementation. Will callback to P4Command::OutputError 
	ClientUser::HandleError( err );
}

// Default handler of perforce error calbacks
void P4Command::OutputError( const char *errBuf )
{
	Pipe().Log().Debug() << errBuf << "\n";
	//	Pipe().InfoLine(errBuf);
}

void P4Command::ErrorPause( char* errBuf, Error* e)
{
	Pipe().Log().Notice() << "Error: Default ClientUser ErrorPause()\n";
}


void P4Command::OutputText( const char *data, int length)
{
	Pipe().Log().Info() << "Error: Default ClientUser OutputText\n";
}


void P4Command::OutputBinary( const char *data, int length)
{
	Pipe().Log().Info() << "Error: Default ClientUser OutputBinary\n";
}


// Default handle of perforce info callbacks. Called by the default P4Command::Message() handler.
void P4Command::OutputInfo( char level, const char *data )
{
	Pipe().Log().Info() << "level " << (int) level << ": " << data << unityplugin::Endl;
	std::stringstream ss;
	ss << data << " (level " << (int) level << ")";	
	Pipe().InfoLine(ss.str());
}

P4Command* P4Command::RunAndSendStatus(P4Task& task, const VersionedAssetList& assetList)
{
	P4StatusCommand* c = dynamic_cast<P4StatusCommand*>(LookupCommand("status"));
	if (!c)
	{
		Pipe().ErrorLine("Cannot locate status command");
		return this; // Returning this is just to keep things running.
	}
	
	bool recursive = false;
	c->RunAndSend(task, assetList, recursive);
	return c;
}

