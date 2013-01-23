#include "Changes.h"
#include "P4StatusBaseCommand.h"

using namespace std;

class P4ChangeStatusCommand : public P4StatusBaseCommand
{
public:
	P4ChangeStatusCommand(const char* name) : P4StatusBaseCommand(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		Pipe().Log() << "ChangeStatusCommand::Run()" << endl;
		
		ChangelistRevision cl;
		Pipe() >> cl;
		
		string cmd = "fstat -T \"depotFile,clientFile,action,ourLock,unresolved,headAction,otherOpen,otherLock,headRev,haveRev\" -W -e ";
		cmd += (cl == kDefaultListRevision ? string("default") : cl) + " //...";
		
		// We're sending along an asset list with an unknown size.
		Pipe().BeginList();
		
		task.CommandRun(cmd, this);
		
		// The OutputState and other callbacks will now output to stdout.´
		// We just wrap up the communication here.
		Pipe().EndList();
		Pipe() << GetStatus();
		Pipe().EndResponse();
		
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

