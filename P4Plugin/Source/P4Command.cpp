#include "P4StatusCommand.h"
#include "P4Utility.h"
#include "msgclient.h"
#include "msgserver.h"

#include <map>
#include <cassert>
#include <sstream>

Connection& SendToConnection(Connection& p, const VCSStatus& st, MessageArea ma)
{
	// Convertion of p4 errors to unity vcs errors
	for (VCSStatus::const_iterator i = st.begin(); i != st.end(); ++i)
	{
		switch (i->severity)
		{
		case VCSSEV_OK: 
			p.VerboseLine(i->message, ma);
			break;
		case VCSSEV_Info:
			p.InfoLine(i->message, ma);
			break;
		case VCSSEV_Warn:
			p.WarnLine(i->message, ma);
			break;
		case VCSSEV_Error:
			p.ErrorLine(i->message, ma);
			break;
		case VCSSEV_Command: 
			p.Command(i->message, ma);
			break;
		default:
			// MAPlugin will make Unity restart the plugin
			p.ErrorLine(std::string("<Unknown errortype>: ") + i->message, MAPlugin);
			break;
		}
	}
	return p;
}

Connection& operator<<(Connection& p, const VCSStatus& st)
{
	return SendToConnection(p, st, MAGeneral);
}

// Global map of all commands registered at initialization time
typedef std::map<std::string, P4Command*> CommandMap;
static CommandMap* s_Commands = NULL;

P4Command* LookupCommand(const std::string& name)
{
	assert(s_Commands != NULL);
	CommandMap::iterator i = s_Commands->find(name);
	if (i == s_Commands->end()) return NULL;
	return i->second;
}

Connection* P4Command::s_Conn = NULL;

P4Command::P4Command(const char* name)
{
	if (s_Commands == NULL)
		s_Commands = new CommandMap();

	s_Commands->insert(std::make_pair(name,this));
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

std::string P4Command::GetStatusMessage() const
{
	std::string delim = "";
	std::string msg;
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

// Default handler of P4
void P4Command::OutputStat( StrDict *varList ) 
{ 
	Conn().Log().Info() << "Default ClientUser OutputState()\n";
}

// Default handler of P4
void P4Command::InputData( StrBuf *buf, Error *err ) 
{ 
	Conn().Log().Info() << "Default ClientUser InputData()\n";
}

void P4Command::Prompt( const StrPtr &msg, StrBuf &buf, int noEcho ,Error *e )
{
	Conn().VerboseLine("Prompting...");
	Conn().Log().Info() << "Default ClientUser Prompt(" << msg.Text() << ")\n";
}

// Default handler of P4
void P4Command::Finished() 
{ 
//	Conn().Log().Info() << "Default ClientUser Finished()\n";
}

// Default handler of P4 error output. Called by the default P4Command::Message() handler.
void P4Command::HandleError( Error *err )
{
	if ( err == 0 )
		return;

	StrBuf buf;
	err->Fmt(&buf);

	if (HandleOnlineStatusOnError(err))
	{
		Conn().Log().Fatal() <<  buf.Text() << Endl;
		return; // Error logged and Unity notified about plugin offline
	}

	// This is a regular non-connection related error
	VCSStatus s = errorToVCSStatus(*err);
	m_Status.insert(s.begin(), s.end());

	// m_Status will contain the error messages to be sent to unity
	// Just log locally
	Conn().Log().Fatal() << buf.Text() << Endl;
}

// Default handler of perforce error calbacks
void P4Command::OutputError( const char *errBuf )
{
	Conn().Log().Fatal() << "We cannot get to here" << Endl;
}

bool P4Command::HandleOnlineStatusOnError(Error *err)
{
	if (err->IsError())
	{
		StrBuf buf;
		err->Fmt(&buf);
		std::string value(buf.Text());

		if (value.find("Connect to server failed; check $P4PORT.") != std::string::npos)
			P4Task::NotifyOffline("Could not connect to the perforce server. Please check your Server setting in the Editor Settings.");
		else if (value.find("TCP connect to") != std::string::npos)
			P4Task::NotifyOffline("Could not connect to the perforce server. Please check your Server setting in the Editor Settings.");
		else if (value.find("Perforce password (P4PASSWD) invalid or unset.") != std::string::npos)
			P4Task::NotifyOffline("Perforce server login failed. Please check your Password setting in the Editor Settings.");
		else if (value.find(" - must create client '") != std::string::npos)
			P4Task::NotifyOffline("Perforce client workspace not present on the Perforce server. Please check your Workspace setting in the Editor Settings.");
		else if (value.find("Client '") != std::string::npos && value.find("' unknown") != std::string::npos)
			P4Task::NotifyOffline("Perforce client workspace not present on the Perforce server. Please check your Workspace setting in the Editor Settings.");
		else if (value.find("Unicode server permits only unicode enabled clients") != std::string::npos)
			P4Task::NotifyOffline("Perforce server connection failed. A unicode Perforce server only permits unicode enabled clients. Please check the unicode settings on your Perforce client workspace and Perforce server.");
		else if (StartsWith(value, "User ") && value.find("doesn't exist.") != std::string::npos)
			P4Task::NotifyOffline(std::string("User '") + P4Task::s_Singleton->GetP4User() + std::string("' not found on Perforce server. Please check the Username in the Editor settings."));
		else if (value.find("Password invalid.") != std::string::npos)
			P4Task::NotifyOffline(std::string("User '") + P4Task::s_Singleton->GetP4User() + std::string("' login failed with incorrect password. Please check the Password in the Editor settings."));

		else
		{
			return false; // error not recognized as a online/offline error
		}
		return true;
	}
	return false;
}

void P4Command::ErrorPause( char* errBuf, Error* e)
{
	Conn().Log().Fatal() << "Error: Default ClientUser ErrorPause()\n";
}

void P4Command::OutputText( const char *data, int length)
{
	Conn().Log().Fatal() << "Error: Default ClientUser OutputText\n";
}

void P4Command::OutputBinary( const char *data, int length)
{
	Conn().Log().Fatal() << "Error: Default ClientUser OutputBinary\n";
}

// Default handle of perforce info callbacks. Called by the default P4Command::Message() handler.
void P4Command::OutputInfo( char level, const char *data )
{
	if (Conn().Log().GetLogLevel() != LOG_DEBUG)
		Conn().Log().Info() << "level " << (int) level << ": " << data << Endl;
	std::stringstream ss;
	ss << data << " (level " << (int) level << ")";	
	Conn().InfoLine(ss.str());
}

void P4Command::RunAndSendStatus(P4Task& task, const VersionedAssetList& assetList)
{
	P4StatusCommand* c = dynamic_cast<P4StatusCommand*>(LookupCommand("status"));
	if (!c)
	{
		Conn().ErrorLine("Cannot locate status command");
		return; // Returning this is just to keep things running.
	}
	
	bool recursive = false;
	c->RunAndSend(task, assetList, recursive);
	Conn() << c->GetStatus();
}

void P4Command::RunAndGetStatus(P4Task& task, const VersionedAssetList& assetList, VersionedAssetList& result)
{
	P4StatusCommand* c = dynamic_cast<P4StatusCommand*>(LookupCommand("status"));
	if (!c)
	{
		Conn().ErrorLine("Cannot locate status command");
		return; // Returning this is just to keep things running.
	}
	
	bool recursive = false;
	c->Run(task, assetList, recursive, result);
}

const char * kDelim = "_XUDELIMX_"; // magic delimiter

static class P4WhereCommand : public P4Command
{
public:
	

	P4WhereCommand() : P4Command("where") {}

	bool Run(P4Task& task, const CommandArgs& args) 
	{ 
		mappings.clear();
		ClearStatus();
		return true;
	}
			
	// Default handle of perforce info callbacks. Called by the default P4Command::Message() handler.
	virtual void OutputInfo( char level, const char *data )
	{	
		// Level 48 is the correct level for view mapping lines. P4 API is really not good at providing these numbers
		std::string msg(data);
		bool propagate = true;
		if (level == 48 && msg.length() > 1)
		{
			// format of the string should be
			// depotPath workspacePath absFilePath
			// e.g.
			// //depot/Project/foo.txt //myworkspace/Project/foo.txt /Users/obama/MyProjects/P4/myworkspace/Project/foo.txt
			// Each path has a postfix of kDelim that we have added in order to tokenize the 
			// result into the three paths. Perforce doesn't check if files exists it just
			// shows the mappings and therefore this will work.
			// Additionally, if the info line starts with a '-' this is just informational and should be ignored.
			if (msg[0] == '-')
			{
				// Just informational line
				propagate = false;
			}
			else if (msg[0] != '/')
			{
				; // do propagate to log
			}
			else
			{
				std::string::size_type i = msg.find(kDelim); // depotPath end
				std::string::size_type j = msg.find(kDelim, i+1); // workspacePath end
				j += 10 + 1; // kDelim.length + (1 space) = start of clientPath
				std::string::size_type k = msg.find(kDelim, j); // clientPath end
				if (i != std::string::npos && i > 2 && k != std::string::npos)
				{
					propagate = false;
					P4Command::Mapping m = { msg.substr(0, i), Replace(msg.substr(j, k-j), "\\", "/") };
					mappings.push_back(m);
				}
			}
		}	

		if (propagate)
			P4Command::OutputInfo(level, data);
	}
	
	std::vector<Mapping> mappings;
	
} cWhere;

const std::vector<P4Command::Mapping>& P4Command::GetMappings(P4Task& task, const VersionedAssetList& assets)
{
	cWhere.mappings.reserve(assets.size());
	cWhere.mappings.clear();
	cWhere.ClearStatus();
	
	if (assets.empty())
		return cWhere.mappings;

	std::string localPaths = ResolvePaths(assets, kPathWild | kPathSkipFolders, "", kDelim);
	
	task.CommandRun("where " + localPaths, &cWhere);
	Conn() << cWhere.GetStatus();
	
	if (cWhere.HasErrors())
	{
		// Abort since there was an error mapping files to depot path
		cWhere.mappings.clear();
	}
	return cWhere.mappings;
}

bool P4Command::MapToLocal(P4Task& task, VersionedAssetList& assets)
{
	const std::vector<Mapping>& mappings = GetMappings(task, assets);
	if (mappings.size() != assets.size())
		return false; // error

	std::vector<Mapping>::const_iterator m = mappings.begin();
	for (VersionedAssetList::iterator i = assets.begin(); i != assets.end(); ++i, ++m)
	{
		i->SetPath(m->clientPath);
	}
	return true;
}
