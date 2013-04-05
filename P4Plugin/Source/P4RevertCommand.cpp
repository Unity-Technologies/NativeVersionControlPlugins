#include "P4FileSetBaseCommand.h"

using namespace std;

class P4RevertCommand : public P4FileSetBaseCommand
{
public:
	P4RevertCommand() : P4FileSetBaseCommand("revert") {}
	
	virtual string SetupCommand(const CommandArgs& args)
	{
		string mode = args[1];
		if (mode == "unchangedOnly")
			return "revert -a ";
		else if (mode == "keepLocalModifications")
			return "revert -k ";
		else
			return "revert ";
	}	
} cRevert;

