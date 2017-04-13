#include "P4Command.h"
#include "P4Task.h"
#include "Utility.h"
#include <sstream>

class P4LogoutCommand : public P4Command
{
public:
	P4LogoutCommand(const char* name) : P4Command(name) { }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
	  
	  if (!task.IsConnected())
	    task.Reconnect();

	  if (!task.IsConnected())
	  {
	    Conn().Log().Debug() << "Cannot logout with no connection" << Endl;
	    return true;
	  }

		if (!task.CommandRunNoLogin("logout", this))
		{
			std::string errorMessage = GetStatusMessage();
			Conn().Log().Fatal() << errorMessage << Endl;
		}
		
		return true;
	}
	
	// Default handler of P4 error output. Called by the default P4Command::Message() handler.
	void HandleError( Error *err )
	{
		return;
	}
	
} cLogout("logout");
