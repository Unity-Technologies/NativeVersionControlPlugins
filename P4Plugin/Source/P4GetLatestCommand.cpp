#include "Utility.h"
#include "P4FileSetBaseCommand.h"
using namespace std;


class P4GetLatestCommand : public P4FileSetBaseCommand
{
public:
	P4GetLatestCommand() : P4FileSetBaseCommand("getLatest", "sync") {}
	
	virtual void HandleError( Error *err )
	{
		if ( err == 0 )
			return;
		
		StrBuf buf;
		err->Fmt(&buf);
		
		const string upToDate = " - file(s) up-to-date.";
		string value(buf.Text());
		value = TrimEnd(value, '\n');
		
		if (EndsWith(value, upToDate)) return; // ignore
		
		P4Command::HandleError(err);
	}
	
} cGetLatest;

