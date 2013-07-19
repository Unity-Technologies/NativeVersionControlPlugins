#include "P4Command.h"
#include "P4Task.h"
#include "Utility.h"
#include <sstream>

using namespace std;

class P4LogoutCommand : public P4Command
{
public:
	P4LogoutCommand(const char* name) : P4Command(name) { m_AllowConnect = false; }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		if (!task.IsConnected()) // Cannot logout without being connected
		{
			Conn().Log().Info() << "Cannot logout when not connected" << Endl;
			return false;
		}
		
		ClearStatus();
		
		if (!task.CommandRun("logout", this))
		{
			string errorMessage = GetStatusMessage();			
			Conn().Log().Notice() << errorMessage << Endl;
		}
		
		return true;
	}
	
	// Default handler of P4 error output. Called by the default P4Command::Message() handler.
	void HandleError( Error *err )
	{
		return;
	}
	
} cLogout("logout");
