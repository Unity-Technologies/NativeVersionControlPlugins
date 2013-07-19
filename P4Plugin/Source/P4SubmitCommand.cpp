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
		
		Conn().Log().Info() << args[0] << "::Run()" << Endl;
		
		bool saveOnly = args.size() > 1 && args[1] == "saveOnly";
		
		Changelist changelist;
		Conn() >> changelist;
		
		VersionedAssetList assetList;
		Conn() >> assetList;
		bool hasFiles = !assetList.empty();
		
		// Run a view mapping job to get the right depot relative paths for the spec file
		string localPaths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);
		Conn().Log().Debug() << "Paths resolved are: " << localPaths << Endl;
		
		const vector<Mapping>& mappings = GetMappings(task, assetList);

		if (mappings.empty() && !assetList.empty())
		{
			// Abort since there was an error mapping files to depot path
			RunAndSendStatus(task, assetList);
			Conn().EndResponse();
			return true;
		}
				
		// Submit the changelist
		P4SpecWriter writer;
		
		// We can never submit the default changelist so essentially we create a new one with its contents
		// in the case a default change list is passed here.
		string cn = changelist.GetRevision();
		if (cn == kNewListRevision || cn == kDefaultListRevision)
			cn = "new";

		writer.WriteSection ("Change", cn);
		writer.WriteSection ("Description", changelist.GetDescription());
		
		if (hasFiles)
		{
			string paths;			
			for (vector<Mapping>::const_iterator i = mappings.begin(); i != mappings.end(); ++i) 
			{
				if (i != mappings.begin())
					paths += "\n";
				paths += i->depotPath;
			}
			writer.WriteSection ("Files", paths);
		}
		
		m_Spec = writer.GetText();
		
		// Submit or update the change list
		const string cmd = saveOnly ? "change -i" : "submit -i";

		task.CommandRun(cmd, this);
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Conn() << GetStatus();

		if (hasFiles)
		{
			RunAndSendStatus(task, assetList);
		} 
		else 
		{
			; // @TODO: handle this case
		}

		Conn().EndResponse();
		
		m_Spec.clear();
		return true;
	}
	
	// Default handler of P4
	virtual void InputData( StrBuf *buf, Error *err ) 
	{
		Conn().Log().Debug() << "Spec is:" << Endl;
		Conn().Log().Debug() << m_Spec << Endl;
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
			Conn().WarnLine("Merges still pending. Resolve before submitting.", MASystem);
			return; // ignore
		}
		
		P4Command::HandleError(err);
	}
	
	
} cSubmit("submit");
