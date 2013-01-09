#include "P4Command.h"
#include "P4Task.h"

class P4ExitCommand : public P4Command
{
public:
	P4ExitCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		return false;
	}
} cExit("exit");
