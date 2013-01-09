#include "Utility.h"
#include "P4FileSetBaseCommand.h"
#include "P4Utility.h"
using namespace std;


class P4AddCommand : public P4FileSetBaseCommand
{
public:
	P4AddCommand() : P4FileSetBaseCommand("add", "add -f") {}
	
	virtual void HandleError( Error *err )
	{
		if ( err == 0 )
			return;
		
		StrBuf buf;
		err->Fmt(&buf);
		
		const string dirNoAdd = " - directory file can't be added.";
		const string noDouble = " - can't add existing file";

		string value(buf.Text());
		value = TrimEnd(value, '\n');
		
		if (EndsWith(value, dirNoAdd) || EndsWith(value, noDouble)) return; // ignore
		
		P4Command::HandleError(err);
	}
	
	virtual int GetResolvePathFlags() const { return kPathSkipFolders; }
	
} cAdd;
