#include "Changes.h"
#include "P4Command.h"
#include "P4Task.h"

using namespace std;

/*
 * Returns the Changelists that are pending to be submitted to perforce server.
 * The assets associated with the changelists are not included.
 */
class P4ChangesCommand : public P4Command
{
public:
	P4ChangesCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{				
		ClearStatus();
		Conn().Log().Info() << args[0] << "::Run()" << Endl;
		const string cmd = string("changes -s pending -u ") + Quote(task.GetP4User()) + " -c " + Quote(task.GetP4Client());

		Conn().BeginList();
		
		// The default list is always there
		Changelist defaultItem;
		const char * kDefaultList = "default";		
		defaultItem.SetDescription(kDefaultList);
		defaultItem.SetRevision(kDefaultListRevision);
		
		Conn() << defaultItem;
		
		task.CommandRun(cmd, this);
		Conn().EndList();
		Conn() << GetStatus();

		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Conn().EndResponse();
		
		return true;
	}
	
	// Called once per changelist 
	void OutputInfo( char level, const char *data )
	{
		string d(data);
		const size_t minLength = 8; // "Change x".length()

		Conn().VerboseLine(d);

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
		item.SetDescription(d.substr(i));
		item.SetRevision(d.substr(minLength-1, i - (minLength-1)));
		Conn() << item;
	}
	
} cChanges("changes");
