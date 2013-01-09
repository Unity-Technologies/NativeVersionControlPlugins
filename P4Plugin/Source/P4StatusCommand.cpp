#include "P4StatusCommand.h"
#include "P4Utility.h"

using namespace std;

P4StatusCommand::P4StatusCommand(const char* name) : P4StatusBaseCommand(name) {}

bool P4StatusCommand::Run(P4Task& task, const CommandArgs& args)
{
	ClearStatus();
	bool recursive = args.size() > 1;
	upipe.Log() << "StatusCommand::Run()" << endl;
			
	VersionedAssetList assetList;
	upipe >> assetList;
	
	RunAndSend(task, assetList, recursive);

	upipe << GetStatus();
	upipe.EndResponse();
	
	return true;
}

void P4StatusCommand::RunAndSend(P4Task& task, const VersionedAssetList& assetList, bool recursive)
{
	string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders | (recursive ? kPathRecursive : kNone) );
	
	upipe.Log() << "Paths to stat are: " << paths << endl;
	
	upipe.BeginList();
	
	if (paths.empty())
	{
		upipe.EndList();
		upipe.ErrorLine("No paths to stat", MASystem);
		return;
	}
	
	string cmd = "fstat -T \"depotFile,clientFile,action,ourLock,unresolved,headAction,otherOpen,otherLock,headRev,haveRev\" ";
	cmd += " " + paths;

	// We're sending along an asset list with an unknown size.
	
	task.CommandRun(cmd, this);
	
	// The OutputState and other callbacks will now output to stdout.
	// We just wrap up the communication here.
	upipe.EndList();
}

P4StatusCommand cStatus("status");
