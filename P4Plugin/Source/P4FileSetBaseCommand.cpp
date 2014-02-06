#include "P4FileSetBaseCommand.h"
#include "P4Task.h"
#include "P4Utility.h"

using namespace std;

P4FileSetBaseCommand::P4FileSetBaseCommand(const char* name, const char* cmdstr) 
	: P4Command(name), m_CmdStr(cmdstr) 
{ 
}

bool P4FileSetBaseCommand::Run(P4Task& task, const CommandArgs& args)
{
	ClearStatus();
	Conn().Log().Info() << args[0] << "::Run()" << Endl;
		
	ReadConnection();

	VersionedAssetList assetList;
	Conn() >> assetList;

	if (Run(task, args, assetList))
	{
		// Stat the files to get the most recent state.
		// This could probably be optimized by reading the output of the specific
		// commands and figure out the new state. 
		RunAndSendStatus(task, assetList);	
	}
	
	// The OutputState and other callbacks will now output to stdout.
	// We just wrap up the communication here.
	Conn().EndResponse();
	return true;
}

bool P4FileSetBaseCommand::Run(P4Task& task, const CommandArgs& args, const VersionedAssetList& assetList)
{
	string cmd = SetupCommand(args);
	string paths = ResolvePaths(assetList, GetResolvePathFlags());
	
	Conn().Log().Debug() << "Paths resolved are: " << paths << Endl;
	
	if (paths.empty())
	{
		Conn().WarnLine("No paths in fileset perforce command", MARemote);
		return false;
	}
	
	cmd += " " + paths;
	
	task.CommandRun(cmd, this);
	Conn() << GetStatus();
	return true;
}

string P4FileSetBaseCommand::SetupCommand(const CommandArgs& args) 
{ 
	return m_CmdStr; 
}

int P4FileSetBaseCommand::GetResolvePathFlags() const 
{ 
	return kPathWild | kPathRecursive; 
}
