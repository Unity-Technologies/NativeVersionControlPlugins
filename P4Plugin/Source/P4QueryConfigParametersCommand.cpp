#include "P4Command.h"
#include "P4Task.h"

class P4QueryConfigParametersCommand : public P4Command
{
public:
	P4QueryConfigParametersCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		Pipe().OkLine("text username");
		Pipe().OkLine("text password");
		Pipe().OkLine("hostAndPort server localhost 1666");
		Pipe().OkLine("text workspace");
		Pipe().OkLine("");
		
		return true;
	}
} cQueryConfigParameters("queryConfigParameters");
