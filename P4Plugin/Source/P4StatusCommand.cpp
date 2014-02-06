#include "P4StatusCommand.h"
#include "P4Utility.h"

using namespace std;

P4StatusCommand::P4StatusCommand(const char* name) : P4StatusBaseCommand(name) {}

bool P4StatusCommand::Run(P4Task& task, const CommandArgs& args)
{
	bool recursive = args.size() > 1;
	Conn().Log().Info() << "StatusCommand::Run()" << Endl;
			
	VersionedAssetList assetList;
	Conn() >> assetList;
	
	RunAndSend(task, assetList, recursive);
	Conn() << GetStatus();

	Conn().EndResponse();

	return true;
}

void P4StatusCommand::PreStatus()
{
	// Since the status command is use to check for online state we start out by
	// forcing online state to true and check if it has been set to false in the
	// end to determine if we should send online notifications.
	m_WasOnline = P4Task::IsOnline();
	P4Task::SetOnline(true);
	
	ClearStatus();
}

void P4StatusCommand::PostStatus()
{
	if (P4Task::IsOnline() && !m_WasOnline)
	{
		// If set to online already we cannot notify as online so we fake an offline state.
		P4Task::SetOnline(false);
		P4Task::NotifyOnline();
	}
}

void P4StatusCommand::RunAndSend(P4Task& task, const VersionedAssetList& assetList, bool recursive)
{
	m_StreamResultToConnection = true;
	string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders | (recursive ? kPathRecursive : kNone) );
	
	Conn().Log().Debug() << "Paths to stat are: " << paths << Endl;
	
	Conn().BeginList();
	
	if (paths.empty())
	{
		Conn().EndList();
		// Conn().ErrorLine("No paths to stat", MASystem);
		return;
	}
	

	string cmd = "fstat -T \"movedFile,depotFile,clientFile,action,ourLock,unresolved,headAction,otherOpen,otherLock,headRev,haveRev\" ";
	cmd += " " + paths;

	// We're sending along an asset list with an unknown size.
	PreStatus();
	task.CommandRun(cmd, this);

	// The OutputState and other callbacks will now output to stdout.
	// We just wrap up the communication here.
	Conn().EndList();

	PostStatus();
}

void P4StatusCommand::Run(P4Task& task, const VersionedAssetList& assetList, bool recursive, VersionedAssetList& result)
{
	m_StreamResultToConnection = false;
	m_StatusResult.clear();
	string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders | (recursive ? kPathRecursive : kNone) );
	
	result.clear();
	Conn().Log().Info() << "Paths to stat are: " << paths << Endl;
	
	if (paths.empty())
	{
		// Conn().ErrorLine("No paths to stat", MASystem);
		return;
	}
	
	string cmd = "fstat -T \"movedFile,depotFile,clientFile,action,ourLock,unresolved,headAction,otherOpen,otherLock,headRev,haveRev\" ";
	cmd += " " + paths;

	// We're sending along an asset list with an unknown size.
	PreStatus();
	task.CommandRun(cmd, this);
	PostStatus();

	result.swap(m_StatusResult);

}

P4StatusCommand cStatus("status");
