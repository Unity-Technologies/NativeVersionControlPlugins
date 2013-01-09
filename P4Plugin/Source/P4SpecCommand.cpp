#include "P4Command.h"
#include "P4Task.h"
#include <sstream>

using namespace std;

class P4SpecCommand : public P4Command
{
public:
	P4SpecCommand(const char* name) : P4Command(name) { m_AllowConnect = false; }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_Root.clear();
		
		const string cmd = string("client -o ") + task.GetP4Client();
		
		if (!task.CommandRun(cmd, this))
		{
			string errorMessage = GetStatusMessage();			
			upipe.Log() << "ERROR: " << errorMessage << endl;
			return false;
		}
		else
		{
			if (!m_Root.empty())
				task.SetP4Root(m_Root);
			upipe.Log() << "Root set to " << m_Root << endl;
			return true;
		}
	}
	
    // Called with entire spec file as data
	void OutputInfo( char level, const char *data )
    {
		stringstream ss(data);
		size_t minlen = 5; // "Root:" 
		
		string line;
		while ( getline(ss, line) ) 
		{	
			
			if (line.length() <= minlen || line.substr(0,minlen) != "Root:")
				continue;
			
			// Got the Root specification line
			string::size_type i = line.find_first_not_of(" \t:", minlen-1);
			if (i == string::npos)
			{
				
				GetStatus().insert(P4StatusItem(P4SEV_Error, string("invalid root specification - ") + line));
				return;
			}
			m_Root = line.substr(i);
			break;
		}
	}
private:
	string m_Root;
	
} cSpec("spec");
