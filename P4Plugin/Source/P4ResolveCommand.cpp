#include "VersionedAsset.h"
#include "Changes.h"
#include "P4FileSetBaseCommand.h"
#include "P4Task.h"
#include "P4Utility.h"

using namespace std;

class P4ResolveCommand : public P4FileSetBaseCommand
{
public:
	P4ResolveCommand() : P4FileSetBaseCommand("resolve") { }

	virtual string SetupCommand(const CommandArgs& args)
	{
		return 
		args.size() > 1 && args[1] == "mine" ? 
		"resolve -ay " :
		"resolve -at ";
	}	

} cResolve;
