#include "Utility.h"
#include "Changes.h"
#include "P4Command.h"
#include "P4Utility.h"

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
		Conn() >> cl;
		
		std::vector<std::string> toks;
		if (Tokenize(toks, m_ProjectPath, "/") == 0)
		{
			Conn().BeginList();
			Conn().WarnLine(std::string("Project path invalid - ") + m_ProjectPath);
			Conn().EndList();
			Conn().EndResponse();
			return true;
		}
		
		Conn().Log().Debug() << "Project path is " << m_ProjectPath << Endl;
		
		std::string rev = cl == kDefaultListRevision ? std::string("default") : cl;
		const std::string cmd = std::string("describe -s ") + rev;
		
		task.CommandRun(cmd, this);
		
		if (!MapToLocal(task, m_Result))
		{
			// Abort since there was an error mapping files to depot path
			Conn().BeginList();
			Conn().WarnLine("Files couldn't be mapped in perforce view");
			Conn().EndList();
			Conn().EndResponse();
			return true;
		}

		Conn() << m_Result;
		m_Result.clear();
		Conn() << GetStatus();

		Conn().EndResponse();
		
		return true;
	}
	
	void OutputText( const char *data, int length)
	{
		Conn().Log().Debug() << "OutputText()" << Endl;
	}
	
    // Called once per asset 
	void OutputInfo( char level, const char *data )
    {
		// The data format is:
		// //depot/...ProjectName/...#revnum action
		// where ... is an arbitrary deep path
		// to get the filesystem path we remove the append this
		// to the Root path.
		
		if (Conn().Log().GetLogLevel() != LOG_DEBUG)
			Conn().Log().Info() << "OutputInfo: " << data << Endl;
		
		std::string d(data);
		Conn().VerboseLine(d);
		std::string::size_type i = d.rfind(" ");
		if (i == std::string::npos || i < 2 || i+1 >= d.length()) // 2 == "//".length()
		{
			Conn().WarnLine(std::string("Invalid change asset - ") + d);
			return;
		}
		
		// strip revision specifier "#ddd"
		std::string::size_type iPathEnd = d.rfind("#", i);
		if (iPathEnd == std::string::npos)
			iPathEnd = i;
		
		VersionedAsset a(d.substr(0, iPathEnd));
		std::string action = d.substr(i+1);
		int state = action.empty() ? kNone : ActionToState(action,"","","");
		a.SetState(state);
		a.RemoveState(kCheckedOutLocal);
		
		m_Result.push_back(a);
	}
	
private:
	std::string m_ProjectPath;
	VersionedAssetList m_Result;
	
} cIncomingChangeAssets("incomingChangeAssets");
