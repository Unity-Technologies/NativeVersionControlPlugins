#include "P4FileSetBaseCommand.h"

static class P4LockCommand : public P4FileSetBaseCommand
{
public:
	P4LockCommand() : P4FileSetBaseCommand("lock", "lock") {}
	
	virtual void OutputInfo( char level, const char *data )
	{
		std::string d(data);

		if (d.find("already locked by") != std::string::npos)
		{
			Conn().WarnLine(data, MARemote);
		}
		else
		{
			P4Command::OutputInfo(level, data);
		}
	}

} cLock;
