#include "Utility.h"
#include "FileSystem.h"
#include "P4StatusBaseCommand.h"
#include "P4Utility.h"

P4StatusBaseCommand::P4StatusBaseCommand(const char* name, bool streamResultToConnection) 
	: P4Command(name), m_StreamResultToConnection(streamResultToConnection)
{
}

const std::string invalidPath = "//...";
const std::string notFound = " - no such file(s).";
const std::string notInClientView = " - file(s) not in client view.";

void P4StatusBaseCommand::OutputStat( StrDict *varList )
{
	if (!P4Task::IsOnline())
		return;

	int i;
	StrRef var, val;
	
	std::vector<std::string> toks;
	VersionedAsset current;
	std::string action;
	std::string headAction;
	std::string headRev;
	std::string haveRev;
	std::string depotFile;
	bool exclLockType = false;
	
	// Dump out the variables, using the GetVar( x ) interface.
	// Don't display the function, which is only relevant to rpc.
	for( i = 0; varList->GetVar( i, var, val ); i++ )
	{
		if( var == "func" ) 
			continue;
		
		std::string key(var.Text());
		std::string value(val.Text());
		// Conn().Log().Debug() << key << " # " << value << Endl;
		
		if (key == "headType")
		{
			exclLockType = value.find("+l") != std::string::npos;
		}
		else if (key == "clientFile")
		{
			current.SetPath(Replace(value, "\\", "/"));
		}
		else if (key == "movedFile")
		{
			current.SetMovedPath(Replace(value, "\\", "/"));
		}
		else if (key == "depotFile")
		{
			depotFile = value;
		}
		else if (key == "action")
		{
			action = value;
			if (value == "edit" && exclLockType)
				current.AddState(kLockedLocal);
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
			if (exclLockType)
				current.AddState(kLockedRemote);
		}
        else if (StartsWith(key, "otherAction") && value == "delete")
        {
            current.AddState(kDeletedRemote); 
        }
         else if (StartsWith(key, "otherAction") && value == "add")
        {
            current.AddState(kAddedRemote); 
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

	int actionState = ActionToState(action, headAction, haveRev, headRev);
	/*
	Conn().Log().Debug() << current.GetPath() << ": action '" << action << "', headAction '" << headAction 
							<< "', haveRev '" << haveRev << "', headRev '" << headRev << "' " << actionState << " " << current.GetState() << Endl;
	*/
	current.AddState((State)actionState);

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

	std::string value(buf.Text());
	value = TrimEnd(value, '\n');
	VersionedAsset asset;	
	
	if (EndsWith(value, notFound) || EndsWith(value, notInClientView))
	{
		if (StartsWith(value, invalidPath) || EndsWith(value.substr(0, value.length() - notFound.length()), "...") || EndsWith(value.substr(0, value.length() - notInClientView.length()), "..."))
		{
			// tried to get status with no files matching wildcard //... which is ok
			// or
			// tried to get status of empty dir ie. not matching /path/to/empty/dir/... which is ok
			Conn().VerboseLine(value);
			return;
		}

		if (AddUnknown(asset, value))
		{
			asset.AddState(kUnversioned);
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

bool P4StatusBaseCommand::AddUnknown(VersionedAsset& current, const std::string& value)
{
	if (EndsWith(value, notFound))
		current.SetPath(WildcardsRemove(value.substr(0, value.length() - notFound.length())));
	else if (EndsWith(value, notInClientView))
		current.SetPath(WildcardsRemove(value.substr(0, value.length() - notInClientView.length())));

	if (EndsWith(current.GetPath(), "*")) 
		return false; // skip invalid files
	return true;
}
