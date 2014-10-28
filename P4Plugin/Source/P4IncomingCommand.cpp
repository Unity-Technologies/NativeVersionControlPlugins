#include "Changes.h"
#include "P4Command.h"
#include "P4Task.h"
#include <set>
#include <sstream>

using namespace std;

/*
 * Returns the Changelists that are on perforce server but locally.
 * The changelists does not include the assets. To get that use the 
 * IncomingChangeAssetsCommand.
 */
class P4IncomingCommand : public P4Command
{
public:
	P4IncomingCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{		
		// Old version used "changes -l -s submitted ...#>have" but that does not include submitted
		// changelist only containing files not already synced to current workspace ie. newly added
		// files from another client.
		// Instead do as the p4v tool does:
		// Run a fstat on entire workspace and get headChange for all files. This will give the
		// most recent changelist available.
		// On the same fstate get the headRev and haveRev revision for each file. If they are not equal 
		// then record the changelist id. 
		ClearStatus();
		m_Changelists.clear();

		Conn().Log().Info() << args[0] << "::Run()" << Endl;
		
		// const std::string cmd = string("fstat -T \"depotFile headChange haveRev headRev headAction action\" //depot/...");
		string rootPathWildcard = TrimEnd(TrimEnd(task.GetProjectPath(), '/'), '\\') + "/...";
		// Compatibility with old perforce servers (<2008). -T is not supported, so just retrieve all the information for the requested files
		const std::string cmd = string("fstat \"") + rootPathWildcard + "\"";
		
		Conn().BeginList();
		
		if (!task.CommandRun(cmd, this))
		{
			// The OutputStat and other callbacks will now output to stdout.
			// We just wrap up the communication here.
			m_Changelists.clear();
			Conn().EndList();
			Conn() << GetStatus();
			Conn().EndResponse();
			return true;
		}
		
		// Fetch the descriptions for the incoming changelists.
		// @TODO: could save some roundtrips by make only one changes call for each sequence of 
		//        changelist ids.
		stringstream ss;
		for (set<int>::const_iterator i = m_Changelists.begin(); i != m_Changelists.end(); ++i) 
		{
			ss.str("");
			ss << "changes -l -s submitted \"@" << *i << ",@" << *i << "\"";
			Conn().Log().Info() << "    " << ss.str() << Endl;
			
			if (!task.CommandRun(ss.str(), this))
			{
				break;
			}
		}
		
		m_Changelists.clear();
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Conn().EndList();
		Conn() << GetStatus();
		Conn().EndResponse();
		
		return true;
	}
	
	// Called once per file for status commands
	void OutputStat( StrDict *varList )
	{
		//Conn().Log().Info() << "Output list stat" << endl;
		
		int i;
		StrRef var, val;
		
		string depotFile;
		int headChange = -1;
		int headRev = -1;
		int haveRev = -1;
		string headAction;
		const string deleteAction("delete");
		bool headDeleted = false;
		bool added = false;
		
		string verboseLine;

		// Dump out the variables, using the GetVar( x ) interface.
		// Don't display the function, which is only relevant to rpc.
		for( i = 0; varList->GetVar( i, var, val ); i++ )
		{
			if( var == "func" ) 
				continue;
			
			string key(var.Text());
			string value(val.Text());
			verboseLine += key + ":" + value + ", ";

			//Conn().Log().Debug() << "    " << key << " # " << value << endl;
			
			if (key == "depotFile")
				depotFile = string(val.Text());
			else if (key == "headAction")
			{
				string action(val.Text());
				if (action == "delete" || action == "move/delete")
					headDeleted = true;
			}
			else if (key == "action")
			{
				string action(val.Text());
				if (action == "add" || action == "move/add")
					added = true;
			}
			else if (key == "headChange")
				headChange = atoi(val.Text());
			else if (key == "headRev")
				headRev = atoi(val.Text());
			else if (key == "haveRev" && value != "none")
				haveRev = atoi(val.Text());
			else 
				Conn().Log().Debug() << "Warning: skipping unknown stat variable: " << key << " : " << val.Text() << Endl;
		}

		Conn().VerboseLine(verboseLine);

		if (headChange == -1 && added)
			return; // don't think about files added locally
		
		// Deleted files does not have a haveRev property and we do not want to list
		// deleted files as incoming changelists unless they have not been deleted locally
		bool syncedDelete = headDeleted && haveRev == -1;
		
		if (depotFile.empty() || headChange == -1 || headRev == -1)
		{
			Conn().WarnLine(string("invalid p4 stat result: ") + (depotFile.empty() ? string("no depotFile") : depotFile));
		} 
		else if (haveRev != headRev && !syncedDelete)
		{
			m_Changelists.insert(headChange);
		}
	}
	
    // Called once per changelist 
	void OutputInfo( char level, const char *data )
    {
		string d(data);
		Conn().VerboseLine(d);

		const size_t minLength = 8; // "Change x".length()
		
		if (d.length() <= minLength)
		{
			Conn().WarnLine(string("p4 changelist too short: ") + d);
			return;
		}
		
		// Parse the change list
		string::size_type i = d.find(' ', 8);
		if (i == string::npos)
		{
			Conn().WarnLine(string("p4 couldn't locate revision: ") + d);
			return;
		}
		

		Changelist item;
		item.SetDescription(d.substr(i+1));
		item.SetRevision(d.substr(minLength-1, i - (minLength-1)));
		Conn() << item;
	}
	
private:	
	set<int> m_Changelists;
	
} cIncoming("incoming");

