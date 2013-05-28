#include "Utility.h"
#include "Changes.h"
#include "P4Command.h"
#include "P4Utility.h"

using namespace std;

class P4IncomingChangeAssetsCommand : public P4Command
{
public:
	P4IncomingChangeAssetsCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_ProjectPath = task.GetP4Root();
		m_Result.clear();

		ChangelistRevision cl;
		Pipe() >> cl;
		
		vector<string> toks;
		if (Tokenize(toks, m_ProjectPath, "/") == 0)
		{
			Pipe().BeginList();
			Pipe().WarnLine(string("Project path invalid - ") + m_ProjectPath);
			Pipe().EndList();
			Pipe().EndResponse();
			return true;
		}
		
		Pipe().Log().Debug() << "Project path is " << m_ProjectPath << unityplugin::Endl;
		
		string rev = cl == kDefaultListRevision ? string("default") : cl;
		const std::string cmd = string("describe -s ") + rev;
		
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

		Pipe().EndResponse();
		
		return true;
	}
	
	void OutputText( const char *data, int length)
	{
		Pipe().Log().Debug() << "OutputText()" << unityplugin::Endl;
	}
	
    // Called once per asset 
	void OutputInfo( char level, const char *data )
    {
		// The data format is:
		// //depot/...ProjectName/...#revnum action
		// where ... is an arbitrary deep path
		// to get the filesystem path we remove the append this
		// to the Root path.
		
		Pipe().Log().Info() << "OutputInfo: " << data << unityplugin::Endl;
		
		string d(data);
		Pipe().VerboseLine(d);
		string::size_type i = d.rfind(" ");
		if (i == string::npos || i < 2 || i+1 >= d.length()) // 2 == "//".length()
		{
			Pipe().WarnLine(string("Invalid change asset - ") + d);
			return;
		}
		
		// strip revision specifier "#ddd"
		string::size_type iPathEnd = d.rfind("#", i);
		if (iPathEnd == string::npos)
			iPathEnd = i;
		
		VersionedAsset a(d.substr(0, iPathEnd));
		string action = d.substr(i+1);
		int state = action.empty() ? kNone : ActionToState(action,"","","");
		a.SetState(state);
		
		m_Result.push_back(a);
	}
	
private:
	string m_ProjectPath;
	VersionedAssetList m_Result;
	
} cIncomingChangeAssets("incomingChangeAssets");
