#include "P4Command.h"
#include "P4Utility.h"
#include "FileSystem.h"

class P4FileModeCommand : public P4Command
{
public:
	P4FileModeCommand() : P4Command("filemode") { }
	
	bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();

		Conn().Log().Info() << args[0] << "::Run()" << Endl;
	
		std::string cmd = SetupCommand(args);
		if (cmd.empty())
		{
			Conn().BeginList();
			Conn().EndList();
			Conn().EndResponse();
			return true;
		}

		VersionedAssetList assetList;
		Conn() >> assetList;
		std::string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);
	
		Conn().Log().Debug() << "Paths resolved are: " << paths << Endl;
	
		if (paths.empty())
		{
			Conn().BeginList();
			Conn().WarnLine("No paths for filemode command", MARemote);
			Conn().EndList();
			Conn().EndResponse();
			return true;
		}
	
		cmd += " " + paths;
	
		task.CommandRun(cmd, this);
		
		assetList.clear();
		Conn() << assetList;
		assetList.clear();
		Conn() << GetStatus();
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Conn().EndResponse();
		return true;
	}

	std::string SetupCommand(const CommandArgs& args)
	{
		if (args.size() < 3)
		{
			Conn().WarnLine("Too few arguments for filemode command");
			return ""; // no command
		}

		std::string method = args[1];
		std::string mode = args[2];

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
				Conn().WarnLine(std::string("Unknown filemode flag ") + mode);
			}
		}
		else
		{
			Conn().WarnLine(std::string("Unknown filemode method ") + method);
		}
		return "";
	}

} cFileModePt;
