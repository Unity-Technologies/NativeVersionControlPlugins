#include "Changes.h"
#include "P4StatusBaseCommand.h"

using namespace std;

class P4ChangeStatusCommand : public P4StatusBaseCommand
{
public:
	P4ChangeStatusCommand(const char* name) : P4StatusBaseCommand(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		// Since the changestatus command is used to check for online state we start out by
		// forcing online state to true and check if it has been set to false in the
		// end to determine if we should send online notifications.
		bool wasOnline = P4Task::IsOnline();
		P4Task::SetOnline(true);

		ClearStatus();
		Conn().Log().Info() << "ChangeStatusCommand::Run()" << Endl;
		
		ChangelistRevision cl;
		Conn() >> cl;
		
		// Compatibility with old perforce servers (<2008). -T is not supported, so just retrieve all the information for the requested files
		string cmd = "fstat -W -e ";
		cmd += (cl == kDefaultListRevision ? string("default") : cl) + " //...";
		
		// We're sending along an asset list with an unknown size.
		Conn().BeginList();
		
		task.CommandRun(cmd, this);
		
		// The OutputState and other callbacks will now output to stdout.´
		// We just wrap up the communication here.
		Conn().EndList();
		Conn() << GetStatus();

		if (P4Task::IsOnline() && !wasOnline)
		{
			// If set to online already we cannot notify as online so we fake an offline state.
			P4Task::SetOnline(false);
			P4Task::NotifyOnline();
		}

		Conn().EndResponse();
		
		return true;
	}
	
	virtual void HandleError( Error *err )
	{
		if ( err == 0 )
			return;
		
		StrBuf buf;
		err->Fmt(&buf);
		
		const string noOpenFound = "//... - file(s) not opened on this client.";
		string value(buf.Text());
		value = TrimEnd(value, '\n');
		
		if (EndsWith(value, noOpenFound))
		{
			return; // tried to get status with no files matching wildcard //... which is ok
		}
		
		P4StatusBaseCommand::HandleError(err);
	}
	
} cChangeStatus("changeStatus");

