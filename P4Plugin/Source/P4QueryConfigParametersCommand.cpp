#include "P4Command.h"
#include "P4Task.h"

class P4QueryConfigParametersCommand : public P4Command
{
public:
	P4QueryConfigParametersCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		Pipe().DataLine("text username");
		Pipe().DataLine("text password");
		Pipe().DataLine("hostAndPort server localhost 1666");
		Pipe().DataLine("text workspace");
		Pipe().DataLine("");
		
		return true;
	}
} cQueryConfigParameters("queryConfigParameters");
