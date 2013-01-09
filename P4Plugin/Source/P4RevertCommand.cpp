#include "P4FileSetBaseCommand.h"

using namespace std;

class P4RevertCommand : public P4FileSetBaseCommand
{
public:
	P4RevertCommand() : P4FileSetBaseCommand("revert") {}
	
	virtual string SetupCommand(const CommandArgs& args)
	{
		return 
		args.size() > 1 && args[1] == "unchangedOnly" ? 
		"revert -a " :
		"revert ";
	}	
} cRevert;

