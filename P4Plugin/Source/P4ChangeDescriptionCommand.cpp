#include "Changes.h"
#include "P4Command.h"
#include "P4Task.h"
#include <sstream>

using namespace	 std;

class P4ChangeDescriptionCommand : public P4Command
{
public:
	P4ChangeDescriptionCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		upipe.Log() << "ChangeDescriptionCommand::Run()"  << endl;
		
		ChangelistRevision cl;
		upipe >> cl;
		
		const string cmd = string("change -o ") + (cl == kDefaultListRevision ? string("") : cl);
				
		task.CommandRun(cmd, this);
		upipe << GetStatus();
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		upipe.EndResponse();
		
		return true;
	}
	
	// Called once
	void OutputInfo( char level, const char *data )
    {		
		string result;
		ReadDescription(data, result);
		upipe.OkLine(result);
	}
	
	int ReadDescription(const char *data, string& result)
	{
		stringstream ss(data);
		char buf[512];
		
		const string kFind = "Description:";
		const string kFiles = "Files:";
		const string kJobs = "Jobs:";
		int lines = 0;
		
		while ( ! ss.getline(buf, 512).fail() )
		{
			if (kFind == buf)
			{				
				while ( (! ss.getline(buf, 512).fail() ) &&  kFiles != buf && kJobs != buf)
				{
					if (buf[0] == '\t')
						result += buf+1;
					else 
						result += buf;

					result += "\n";
					++lines;
				}
				
				return lines;
			}
		}
		
		return lines;
	}
	
	
} cChangeDescription("changeDescription");

