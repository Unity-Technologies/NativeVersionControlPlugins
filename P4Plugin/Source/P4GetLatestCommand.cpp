#include "Utility.h"
#include "P4Command.h"
#include "P4Utility.h"
using namespace std;


class P4GetLatestCommand : public P4Command
{
public:
	P4GetLatestCommand() : P4Command("getLatest") {}
	
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		incomingAssetList.clear();
		ClearStatus();
		Pipe().Log().Info() << args[0] << "::Run()" << unityplugin::Endl;
		
		string cmd = "sync";
		
		VersionedAssetList assetList;
		Pipe() >> assetList;
		string paths = ResolvePaths(assetList, kPathWild | kPathRecursive);
		
		Pipe().Log().Debug() << "Paths resolved are: " << paths << unityplugin::Endl;
		
		if (paths.empty())
		{
			Pipe().WarnLine("No paths in getlatest perforce command", MARemote);
			Pipe().EndResponse();
			return true;
		}
		
		cmd += " " + paths;
		
		task.CommandRun(cmd, this);
		Pipe() << GetStatus();
		
		// Stat the files to get the most recent state.
		// This could probably be optimized by reading the output of the command better
		RunAndSendStatus(task, incomingAssetList);
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Pipe().EndResponse();
		return true;
	}

	virtual void HandleError( Error *err )
	{
		if ( err == 0 )
			return;
				
		StrBuf buf;
		err->Fmt(&buf);
		
		const string upToDate = " - file(s) up-to-date.";
		string value(buf.Text());
		value = TrimEnd(value, '\n');
		
		if (EndsWith(value, upToDate)) return; // ignore
		
		P4Command::HandleError(err);
	}
	

	// Default handle of perforce info callbacks. Called by the default P4Command::Message() handler.
	virtual void OutputInfo( char level, const char *data )
	{
		if (level != 48)
		{
			P4Command::OutputInfo(level, data);
			return;
		}

		Pipe().VerboseLine(data);

		// format e.g.:
		// //depot/P4Test/Assets/Lars.meta#2 - updating /Users/foobar/UnityProjects/PerforceTest/P4Test/Assets/Lars.meta
		// //depot/P4Test/Assets/killme.txt#1 - added as /Users/foo....
		// //depot/P4Test/Assets/killme.txt#1 - deleted as /Users/foo....

		string d(data);
		string::size_type i1 = d.find('#');
		string::size_type i2 = d.find(' ', i1);
		string rev = d.substr(i1+1, i2);

		// Skip " updating " or " xxxxx as "
		i1 = d.find(' ', i2+3);
		i1++;

		if (d.substr(i1, 3) == "as ")
			i1 += 3;

		string path = d.substr(i1);
		
		incomingAssetList.push_back(VersionedAsset(path, kSynced, rev));
	}

	VersionedAssetList incomingAssetList;

} cGetLatest;
