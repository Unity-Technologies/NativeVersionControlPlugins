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
	upipe.Log() << args[0] << "::Run()" << endl;
	
	string cmd = SetupCommand(args);
	
	VersionedAssetList assetList;
	upipe >> assetList;
	string paths = ResolvePaths(assetList, GetResolvePathFlags());
	
	upipe.Log() << "Paths resolved are: " << paths << endl;
	
	if (paths.empty())
	{
		upipe.ErrorLine("No paths in fileset perforce command", MARemote);
		upipe.EndResponse();
		return true;
	}
	
	cmd += " " + paths;
	
	task.CommandRun(cmd, this);
	upipe << GetStatus();

	// Stat the files to get the most recent state.
	// This could probably be optimized by reading the output of the specific
	// commands and figure out the new state. 
	P4Command* statusCommand = RunAndSendStatus(task, assetList);	
	upipe << statusCommand->GetStatus();
	
	// The OutputState and other callbacks will now output to stdout.
	// We just wrap up the communication here.
	upipe.EndResponse();
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
