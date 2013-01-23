#include "Changes.h"
#include "P4FileSetBaseCommand.h"
#include "P4Task.h"
#include "P4Utility.h"

using namespace std;

class P4ChangeMoveCommand : public P4FileSetBaseCommand
{
public:
	P4ChangeMoveCommand() : P4FileSetBaseCommand("changeMove") {}
	
	virtual string SetupCommand(const CommandArgs& args)
	{
		ChangelistRevision cr;
		Pipe() >> cr;
		string cmd("reopen -c ");
		return cmd += (cr == kDefaultListRevision ? string("default") : cr);
	}
} cChangeMove;
