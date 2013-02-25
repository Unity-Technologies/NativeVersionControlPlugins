#include "Changes.h"
#include "P4Command.h"
#include "P4Task.h"

using namespace std;

class P4DeleteChangesCommand : public P4Command
{
public:
	P4DeleteChangesCommand(const char* name) : P4Command(name){}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		Pipe().Log().Info() << args[0] << "::Run()" << unityplugin::Endl;
		std::string errorMessage;
		
		const string cmd = "change -d";
		
		ChangelistRevisions changes;
		Pipe() >> changes;
		
		if (changes.empty())
		{
			
			Pipe().ErrorLine("Changes to delete is empty");
			Pipe().EndResponse();
			return true;
		}
		
		string cl;
		for (ChangelistRevisions::const_iterator i = changes.begin(); i != changes.end(); ++i)
		{
			string rev = *i == kDefaultListRevision ? string("default") : *i;
			string cmdr = cmd + " \"" + rev + "\"";
			if (!task.CommandRun(cmdr, this))
				break;
		}
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.

		Pipe() << GetStatus();
		
		// @TODO: send changed assets
		VersionedAssetList dummy;
		Pipe() << dummy;
		
		Pipe().EndResponse();
		
		return true;
	}	
} cDeleteChanges("deleteChanges");
