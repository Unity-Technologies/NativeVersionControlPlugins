#include "Utility.h"
#include "FileSystem.h"
#include "P4StatusBaseCommand.h"
#include "P4Utility.h"

using namespace std;

P4StatusBaseCommand::P4StatusBaseCommand(const char* name, bool streamResultToConnection) 
	: P4Command(name), m_StreamResultToConnection(streamResultToConnection)
{
}

void P4StatusBaseCommand::OutputStat( StrDict *varList )
{
	if (!P4Task::IsOnline())
		return;

	const string invalidPath = "//...";
	const string notFound = " - no such file(s).";
	
	int i;
	StrRef var, val;
	
	vector<string> toks;
	VersionedAsset current;
	string action;
	string headAction;
	string headRev;
	string haveRev;
	string depotFile;
	bool isStateSet = false;
	
	// Dump out the variables, using the GetVar( x ) interface.
	// Don't display the function, which is only relevant to rpc.
	for( i = 0; varList->GetVar( i, var, val ); i++ )
	{
		if( var == "func" ) 
			continue;
		
		string key(var.Text());
		string value(val.Text());
		// Conn().Log().Debug() << key << " # " << value << Endl;
		
		if (EndsWith(value, notFound) && !StartsWith(key, invalidPath))
		{
			if (!AddUnknown(current, value))
				return; // invalid file
			isStateSet = true;
		}
		else if (key == "clientFile")
		{
			current.SetPath(Replace(value, "\\", "/"));
		}
		else if (key == "movedFile")
		{
			current.SetMovedPath(value);
		}
		else if (key == "depotFile")
		{
			depotFile = value;
		}
		else if (key == "action")
		{
			action = value;
		}
		else if (key == "ourLock")
		{
			current.AddState(kLockedLocal);
		}
		else if (key == "otherLock")
		{
			current.AddState(kLockedRemote);
		}
		else if (key == "otherOpen")
		{
			current.AddState(kCheckedOutRemote);
		}
		else if (key == "unresolved")
		{
			current.AddState(kConflicted);
		} 
		else if (key == "headAction")
		{
			headAction = value;
		}			
		else if (key == "headRev")
		{
			headRev = value;
		}			
		else if (key == "haveRev")
		{
			haveRev = value;
		}			
		else if (key == "desc")
		{
			return; // This is not a file output stat but a changelist description. Ignore it.
		}
	}
	
	if (PathExists(current.GetPath()))
	{
		current.AddState(kLocal);
		if (IsReadOnly(current.GetPath()))
			current.AddState(kReadOnly);
	}

	if (!isStateSet)
	{
		int actionState = ActionToState(action, headAction, haveRev, headRev);
		current.AddState((State)actionState);
	}		

	Conn().VerboseLine(current.GetPath());
	
	if (m_StreamResultToConnection)
		Conn() << current;
	else
		m_StatusResult.push_back(current);
}

void P4StatusBaseCommand::HandleError( Error *err )
{
	if ( err == 0  || !P4Task::IsOnline())
		return;
	
	StrBuf buf;
	err->Fmt(&buf);
	
	const string invalidPath = "//...";
	const string notFound = " - no such file(s).";
	string value(buf.Text());
	value = TrimEnd(value, '\n');
	VersionedAsset asset;	
	
	if (EndsWith(value, notFound))
	{
		if (StartsWith(value, invalidPath) || EndsWith(value.substr(0, value.length() - notFound.length()), "..."))
		{
			// tried to get status with no files matching wildcard //... which is ok
			// or
			// tried to get status of empty dir ie. not matching /path/to/empty/dir/... which is ok
			Conn().VerboseLine(value);
			return;
		}
		else if (AddUnknown(asset, value))
		{

			if (PathExists(asset.GetPath()))
			{
				asset.AddState(kLocal);
				if (IsReadOnly(asset.GetPath()))
					asset.AddState(kReadOnly);
			}

			if (m_StreamResultToConnection)
				Conn() << asset;
			else
				m_StatusResult.push_back(asset);
			Conn().VerboseLine(value);
			return; // just ignore errors for unknown files and return them anyway
		} 
	}

	P4Command::HandleError(err);
}

bool P4StatusBaseCommand::AddUnknown(VersionedAsset& current, const string& value)
{
	const string notFound = " - no such file(s).";

	current.SetPath(WildcardsRemove(value.substr(0, value.length() - notFound.length())));
	if (EndsWith(current.GetPath(), "*")) 
		return false; // skip invalid files
	return true;
}
