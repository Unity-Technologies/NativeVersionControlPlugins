#include "P4FileSetBaseCommand.h"

using namespace std;

static class P4LockCommand : public P4FileSetBaseCommand
{
public:
	P4LockCommand() : P4FileSetBaseCommand("lock", "lock") {}
	
	virtual void OutputInfo( char level, const char *data )
	{
		string d(data);

		if (d.find("already locked by") != string::npos)
		{
			Pipe().WarnLine(data, MARemote);
		}
		else
		{
			P4Command::OutputInfo(level, data);
		}
	}

} cLock;
