#include "P4Command.h"
#include "P4Task.h"

class P4QueryConfigParametersCommand : public P4Command
{
public:
	P4QueryConfigParametersCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		Conn().DataLine("text username");
		Conn().DataLine("text password");
		Conn().DataLine("hostAndPort server localhost 1666");
		Conn().DataLine("text workspace");
		Conn().DataLine("");
		
		return true;
	}
} cQueryConfigParameters("queryConfigParameters");
