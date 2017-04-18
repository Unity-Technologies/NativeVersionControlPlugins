#include "Changes.h"
#include "P4Command.h"
#include "P4Task.h"
#include <sstream>

class P4ChangeDescriptionCommand : public P4Command
{
public:
	P4ChangeDescriptionCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		Conn().Log().Info() << "ChangeDescriptionCommand::Run()"  << Endl;
		
		ChangelistRevision cl;
		Conn() >> cl;
		
		const std::string cmd = std::string("change -o ") + (cl == kDefaultListRevision ? std::string("") : cl);
				
		task.CommandRun(cmd, this);
		Conn() << GetStatus();
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Conn().EndResponse();
		
		return true;
	}
	
	// Called once
	void OutputInfo( char level, const char *data )
    {		
		std::string result;
		Conn().VerboseLine(data);
		ReadDescription(data, result);
		if (result == "<enter description here>\n\n")
			result.clear();
		Conn().DataLine(result);
	}
	
	int ReadDescription(const char *data, std::string& result)
	{
		std::stringstream ss(data);
		char buf[512];
		
		const std::string kFind = "Description:";
		const std::string kFiles = "Files:";
		const std::string kJobs = "Jobs:";
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

