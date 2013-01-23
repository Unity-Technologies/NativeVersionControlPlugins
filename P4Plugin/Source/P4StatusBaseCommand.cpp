#include "Utility.h"
#include "FileSystem.h"
#include "P4StatusBaseCommand.h"
#include "P4Utility.h"

using namespace std;

P4StatusBaseCommand::P4StatusBaseCommand(const char* name) : P4Command(name) 
{
}

void P4StatusBaseCommand::OutputStat( StrDict *varList )
{
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
		// Pipe().Log() << key << " # " << value << endl;
		
		if (EndsWith(value, notFound) && !StartsWith(key, invalidPath))
		{
			if (!AddUnknown(current, value))
				return; // invalid file
			isStateSet = true;
		}
		else if (key == "clientFile")
		{
			current.SetPath(Replace(value, "\\", "/"));
			if (IsReadOnly(current.GetPath()))
				current.AddState(kReadOnly);
			else
				current.RemoveState(kReadOnly);
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
	
	if (!isStateSet)
	{
		int baseState = current.GetState() & ( kCheckedOutRemote | kLockedLocal | kLockedRemote | kConflicted | kReadOnly | kMetaFile );
		current.SetState(ActionToState(action, headAction, haveRev, headRev) | baseState);
	}
	
	Pipe() << current;
}

void P4StatusBaseCommand::HandleError( Error *err )
{
	if ( err == 0 )
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
		if (StartsWith(value, invalidPath))
		{
			return; // tried to get status with no files matching wildcard //... which is ok
		}
		else if (AddUnknown(asset, value))
		{
			Pipe() << asset;
			return; // just ignore errors for unknown files and return them anyway
		} 
	}
	
	P4Command::HandleError(err);
}

bool P4StatusBaseCommand::AddUnknown(VersionedAsset& current, const string& value)
{
	const string notFound = " - no such file(s).";
	
	current.SetPath(WildcardsRemove(value.substr(0, value.length() - notFound.length())));
	int baseState = current.GetState() & ( kConflicted | kReadOnly | kMetaFile );
	current.SetState(kLocal | baseState);
	current.RemoveState(kLockedLocal);
	current.RemoveState(kLockedRemote);
	if (IsReadOnly(current.GetPath()))
		current.AddState(kReadOnly);
	else 
		current.RemoveState(kReadOnly);
	
	if (EndsWith(current.GetPath(), "*")) 
		return false; // skip invalid files
	return true; 
}
