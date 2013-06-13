#include "P4Command.h"
#include "P4Utility.h"
#include "FileSystem.h"

using namespace std;

class P4fileModeCommand : public P4Command
{
public:
	P4fileModeCommand() : P4Command("filemode") { }
	
	bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();

		Pipe().Log().Info() << args[0] << "::Run()" << unityplugin::Endl;
	
		string cmd = SetupCommand(args);
		if (cmd.empty())
		{
			Pipe().BeginList();
			Pipe().EndList();
			Pipe().EndResponse();
			return true;
		}

		VersionedAssetList assetList;
		Pipe() >> assetList;
		string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);
	
		Pipe().Log().Debug() << "Paths resolved are: " << paths << unityplugin::Endl;
	
		if (paths.empty())
		{
			Pipe().BeginList();
			Pipe().WarnLine("No paths for filemode command", MARemote);
			Pipe().EndList();
			Pipe().EndResponse();
			return true;
		}
	
		cmd += " " + paths;
	
		task.CommandRun(cmd, this);
		
		assetList.clear();
		Pipe() << assetList;
		assetList.clear();
		Pipe() << GetStatus();
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Pipe().EndResponse();
		return true;
	}

	string SetupCommand(const CommandArgs& args)
	{
		if (args.size() < 3)
		{
			Pipe().WarnLine("Too few arguments for filemode command");
			return ""; // no command
		}

		string method = args[1];
		string mode = args[2];

		if (method == "set")
		{
			if (mode == "binary")
			{
				return "reopen -t binary ";
			}
			else if (mode == "text")
			{
				return "reopen -t text ";
			}
			else
			{
				Pipe().WarnLine(string("Unknown filemode flag ") + mode);
			}
		}
		else
		{
			Pipe().WarnLine(string("Unknown filemode method ") + method);
		}
		return "";
	}

} cFileModePt;
