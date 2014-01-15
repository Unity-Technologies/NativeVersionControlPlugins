#include "P4StatusCommand.h"
#include "P4Utility.h"

using namespace std;

P4StatusCommand::P4StatusCommand(const char* name) : P4StatusBaseCommand(name) {}

bool P4StatusCommand::Run(P4Task& task, const CommandArgs& args)
{
	// Since the status command is use to check for online state we start out by
	// forcing online state to true and check if it has been set to false in the
	// end to determine if we should send online notifications.
	bool wasOnline = P4Task::IsOnline();
	P4Task::SetOnline(true);
	
	ClearStatus();

	bool recursive = args.size() > 1;
	Conn().Log().Info() << "StatusCommand::Run()" << Endl;
			
	VersionedAssetList assetList;
	Conn() >> assetList;
	
	RunAndSend(task, assetList, recursive);
	Conn() << GetStatus();

	if (P4Task::IsOnline() && !wasOnline)
	{
		// If set to online already we cannot notify as online so we fake an offline state.
		P4Task::SetOnline(false);
		P4Task::NotifyOnline();
	}

	Conn().EndResponse();

	return true;
}

void P4StatusCommand::RunAndSend(P4Task& task, const VersionedAssetList& assetList, bool recursive)
{
	string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders | (recursive ? kPathRecursive : kNone) );
	
	Conn().Log().Debug() << "Paths to stat are: " << paths << Endl;
	
	Conn().BeginList();
	
	if (paths.empty())
	{
		Conn().EndList();
		// Conn().ErrorLine("No paths to stat", MASystem);
		return;
	}

	//[BHVR/AC]: compatibility with old perforce servers (<2008). -T is not supported, so just retrieve all the information for the requested files 
	string cmd = "fstat ";
	cmd += " " + paths;

	// We're sending along an asset list with an unknown size.
	task.CommandRun(cmd, this);
	
	// The OutputState and other callbacks will now output to stdout.
	// We just wrap up the communication here.
	Conn().EndList();
}

P4StatusCommand cStatus("status");
