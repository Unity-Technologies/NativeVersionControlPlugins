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

		Pipe().Log().Info() << args[0] << "::Run()" << unityplugin::Endl;
	
		string cmd = SetupCommand(args);
	
		VersionedAssetList assetList;
		Pipe() >> assetList;
		string paths = ResolvePaths(assetList, kPathWild | kPathRecursive);
	
		Pipe().Log().Debug() << "Paths resolved are: " << paths << unityplugin::Endl;
	
		if (paths.empty())
		{
			Pipe().ErrorLine("No paths in fileset perforce command", MARemote);
			Pipe().EndResponse();
			return true;
		}
	
		cmd += " " + paths;
	
		Pipe().BeginList();
		task.CommandRun(cmd, this);
		Pipe().EndList();
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
			Pipe().ErrorLine(string("Invalid revert asset - ") + d);
			return;
		}
		
		VersionedAsset a(m_ProjectPath + d.substr(1, iPathEnd-1));

		if (EndsWith(d, ", not reverted"))
			return;

		if (EndsWith(d, ", deleted"))
			a.AddState(kDeletedLocal);
		else
			a.AddState(kSynced);

		Pipe() << a;
	}

private:
	string m_ProjectPath;

} cReverPt;
