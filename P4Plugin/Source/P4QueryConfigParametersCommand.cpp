#include "P4Command.h"
#include "P4Task.h"

class P4QueryConfigParametersCommand : public P4Command
{
public:
	P4QueryConfigParametersCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		upipe.OkLine("text username");
		upipe.OkLine("text password");
		upipe.OkLine("hostAndPort server localhost 1666");
		upipe.OkLine("text workspace");
		upipe.OkLine("");
		
		return true;
	}
} cQueryConfigParameters("queryConfigParameters");
