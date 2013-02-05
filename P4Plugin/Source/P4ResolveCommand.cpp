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
		// 'mine' in p4 is always the actual source file. Unity merges
		// into the actual source file which makes them the same case.
		// This is in contrast to e.g. svn which have a specific .mine 
		return 
			args.size() > 1 && (args[1] == "mine" || args[1] == "merged") ? 
			"resolve -ay " :
			"resolve -at ";
	}	

} cResolve;
