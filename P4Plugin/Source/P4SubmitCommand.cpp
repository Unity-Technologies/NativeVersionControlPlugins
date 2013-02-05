#include "Changes.h"
#include "P4FileSetBaseCommand.h"
#include "P4Task.h"
#include "P4Utility.h"
#include <sstream>

using namespace std;

// Write a perforce SPEC which is essentially a list of attributes and text sections.  This is generic and
// can handle any perforce SPEC.  It primary use is for writing change lists.
class P4SpecWriter
{
public:
	stringstream writer;
	
	void WriteSection(const string& name, const string& data)
	{
		writer << name << ":" << endl;
		stringstream ss(data);
		string line;
		while (getline(ss, line))
			writer << "\t" << line << endl;
		
		writer << endl;
	}
	
	string GetText() const
	{
		return writer.str();
    }
};


static class P4WhereCommand : public P4Command
{
public:
	
	P4WhereCommand() : P4Command("where") {}

	bool Run(P4Task& task, const CommandArgs& args) 
	{ 
		ClearStatus();
		return true; 
	}
			
	// Default handle of perforce info callbacks. Called by the default P4Command::Message() handler.
	virtual void OutputInfo( char level, const char *data )
	{	
		// Level 48 is the correct level for view mapping lines. P4 API is really not good at providing these numbers
		string msg(data);
		bool propergate = true;
		if (level == 48 && msg.length() > 1)
		{
			// format of the string should be 
			// depotPath workspacePath absFilePath
			// e.g.
			// //depot/Project/foo.txt //myworkspace/Project/foo.txt //Users/obama/MyProjects/P4/myworkspace/Project/foo.txt
			string::size_type i = msg.find("//", 1);
			if (i != -1 && i > 2)
			{
				propergate = false;
				depotPaths.push_back(msg.substr(0, i-1));
			}
		}	

		if (propergate)
			P4Command::OutputInfo(level, data);
	}
	
	vector<string> depotPaths;
	
} cWhere;


class P4SubmitCommand : public P4Command
{
private:
	string m_Spec;
public:
	P4SubmitCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_Spec.clear();
		
		Pipe().Log() << args[0] << "::Run()" << unityplugin::Endl;
		
		bool saveOnly = args.size() > 1 && args[1] == "saveOnly";
		
		Changelist changelist;
		Pipe() >> changelist;
		
		VersionedAssetList assetList;
		Pipe() >> assetList;
		bool hasFiles = !assetList.empty();
		
		// Run a view mapping job to get the right depot relative paths for the spec file
		string localPaths = ResolvePaths(assetList, kPathWild | kPathRecursive);
		Pipe().Log() << "Paths resolved are: " << localPaths << unityplugin::Endl;
		
		cWhere.ClearStatus();
		cWhere.depotPaths.clear();
		cWhere.depotPaths.reserve(assetList.size());
		task.CommandRun("where " + localPaths, &cWhere);
		Pipe() << cWhere.GetStatus();

		if (cWhere.HasErrors())
		{
			// Abort since there was an error mapping files to depot path
			P4Command* statusCommand = RunAndSendStatus(task, assetList);
			Pipe() << statusCommand->GetStatus();
			Pipe().EndResponse();
			return true;
		}
		
		vector<string>& depotPaths = cWhere.depotPaths;
		
		// Submit the changelist
		P4SpecWriter writer;
		
		// We can never submit the default changelist so essentially we create a new one with its contents
		// in the case a default change list is passed here.
		writer.WriteSection ("Change", changelist.GetRevision() == kNewListRevision ? string("new") : changelist.GetRevision());
		writer.WriteSection ("Description", changelist.GetDescription());
		
		if (hasFiles)
		{
			string paths;			
			for (vector<string>::const_iterator i = depotPaths.begin(); i != depotPaths.end(); ++i) 
			{
				if (i != depotPaths.begin())
					paths += "\n";
				paths += *i;
			}
			writer.WriteSection ("Files", paths);
		}

		depotPaths.clear();
		
		m_Spec = writer.GetText();
		
		// Submit or update the change list
		const string cmd = saveOnly ? "change -i" : "submit -i";

		task.CommandRun(cmd, this);
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Pipe() << GetStatus();

		if (hasFiles)
		{
			P4Command* statusCommand = RunAndSendStatus(task, assetList);
			Pipe() << statusCommand->GetStatus();
		} 
		else 
		{
			; // @TODO: handle this case
		}

		Pipe().EndResponse();
		
		m_Spec.clear();
		return true;
	}
	
	// Default handler of P4
	virtual void InputData( StrBuf *buf, Error *err ) 
	{
		Pipe().Log() << "Spec is:" << unityplugin::Endl;
		Pipe().Log() << m_Spec << unityplugin::Endl;
		buf->Set(m_Spec.c_str());
	}
	
	virtual void HandleError( Error *err )
	{
		if ( err == 0 )
			return;
		
		StrBuf buf;
		err->Fmt(&buf);
		
		const string pendingMerges = "Merges still pending -- use 'resolve' to merge files.";
		
		string value(buf.Text());
		value = TrimEnd(value, '\n');
		
		if (StartsWith(value, pendingMerges))
		{
			Pipe().WarnLine("Merges still pending. Resolve before submitting.", MASystem);
			return; // ignore
		}
		
		P4Command::HandleError(err);
	}
	
	
} cSubmit("submit");
