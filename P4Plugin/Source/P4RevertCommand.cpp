#include "P4Command.h"
#include "P4Utility.h"

using namespace std;

class P4RevertCommand : public P4Command
{
public:
	P4RevertCommand() : P4Command("revert") { }
	
	bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_ProjectPath = task.GetP4Root();
		m_Result.clear();

		Pipe().Log().Info() << args[0] << "::Run()" << unityplugin::Endl;
	
		string cmd = SetupCommand(args);
	
		VersionedAssetList assetList;
		Pipe() >> assetList;
		string paths = ResolvePaths(assetList, kPathWild | kPathRecursive);
	
		Pipe().Log().Debug() << "Paths resolved are: " << paths << unityplugin::Endl;
	
		if (paths.empty())
		{
			Pipe().BeginList();
			Pipe().ErrorLine("No paths for revert command", MARemote);
			Pipe().EndList();
			Pipe().EndResponse();
			return true;
		}
	
		cmd += " " + paths;
	
		task.CommandRun(cmd, this);

		if (!MapToLocal(task, m_Result))
		{
			// Abort since there was an error mapping files to depot path
			Pipe().BeginList();
			Pipe().WarnLine("Files couldn't be mapped in perforce view");
			Pipe().EndList();
			Pipe().EndResponse();
			return true;
		}
		Pipe() << m_Result;
		m_Result.clear();
		Pipe() << GetStatus();
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Pipe().EndResponse();
		return true;
	}

	virtual string SetupCommand(const CommandArgs& args)
	{
		string mode = args.size() > 1 ? args[1] : string();
		if (mode == "unchangedOnly")
			return "revert -a ";
		else if (mode == "keepLocalModifications")
			return "revert -k ";
		else
			return "revert ";
	}

    // Called once per asset 
	void OutputInfo( char level, const char *data )
    {
		// The data format is:
		// //depot/...ProjectName/...#revnum action
		// where ... is an arbitrary deep path
		// to get the filesystem path we remove the append this
		// to the Root path.
		
		P4Command::OutputInfo(level, data);
		
		string d(data);

		// strip revision specifier "#ddd"
		string::size_type iPathEnd = d.rfind("#");
		if (iPathEnd == string::npos)
		{
			Pipe().WarnLine(string("Invalid revert asset - ") + d);
			return;
		}
		
		VersionedAsset a(d.substr(0, iPathEnd));

		if (EndsWith(d, ", not reverted"))
			return;

		if (EndsWith(d, ", deleted"))
			a.AddState(kDeletedLocal);
		else
			a.AddState(kSynced);

		m_Result.push_back(a);
	}

private:
	string m_ProjectPath;
	VersionedAssetList m_Result;

} cReverPt;
