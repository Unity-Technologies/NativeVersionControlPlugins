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
		
		ChangelistRevision cl;
		Pipe() >> cl;
		
		Pipe().BeginList();
		
		vector<string> toks;
		if (Tokenize(toks, m_ProjectPath, "/") == 0)
		{
			Pipe().ErrorLine(string("Project path invalid - ") + m_ProjectPath);
			Pipe().EndList();
			Pipe().ErrorLine("Invalid project path", MARemote);
			Pipe().EndResponse();
			return true;
		}
		
		Pipe().Log() << "Project path is " << m_ProjectPath << unityplugin::Endl;
		
		string rev = cl == kDefaultListRevision ? string("default") : cl;
		const std::string cmd = string("describe -s ") + rev;
		
		task.CommandRun(cmd, this);
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Pipe().EndList();
		Pipe() << GetStatus();
		Pipe().EndResponse();
		
		return true;
	}
	
	void OutputText( const char *data, int length)
	{
		Pipe().Log() << "OutputText()" << unityplugin::Endl;
	}
	
    // Called once per asset 
	void OutputInfo( char level, const char *data )
    {
		// The data format is:
		// //depot/...ProjectName/...#revnum action
		// where ... is an arbitrary deep path
		// to get the filesystem path we remove the append this
		// to the Root path.
		
		Pipe().Log() << "OutputInfo: " << data << unityplugin::Endl;
		
		string d(data);
		string::size_type i = d.rfind(" ");
		if (i == string::npos || i < 2 || i+1 >= d.length()) // 2 == "//".length()
		{
			Pipe().ErrorLine(string("Invalid change asset - ") + d);
			return;
		}
		
		// strip revision specifier "#ddd"
		string::size_type iPathEnd = d.rfind("#", i);
		if (iPathEnd == string::npos)
			iPathEnd = i;
		
		VersionedAsset a(m_ProjectPath + d.substr(1, iPathEnd-1));
		string action = d.substr(i+1);
		int state = action.empty() ? kNone : ActionToState(action,"","","");
		a.SetState(state);
		
		Pipe() << a;
	}
	
private:
	string m_ProjectPath;
	
} cIncomingChangeAssets("incomingChangeAssets");
