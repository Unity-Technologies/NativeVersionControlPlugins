#include "P4Command.h"
#include "P4Task.h"
#include "P4Stream.h"
#include "Utility.h"
#include <sstream>

using namespace std;

class P4StreamsCommand : public P4Command
{
public:
	P4StreamsCommand(const char* name) : P4Command(name) { }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_Streams.clear();

		if (!task.CommandRun("streams", this))
		{
			string errorMessage = GetStatusMessage();			
			Conn().Log().Fatal() << errorMessage << Endl;
			return false;
		}
		else
		{
			task.SetP4Streams(m_Streams);
			return true;
		}
	}
	
    // Called with entire spec file as data
	void OutputInfo( char level, const char *data )
    {
		stringstream ss(data);
		Conn().VerboseLine(data);
		size_t minlen = 5; // "Root:" 
		
		string line;
		const string streamPrefix = "Stream ";
		while ( getline(ss, line) ) 
		{	
			if (StartsWith(line, streamPrefix))
			{
				// TODO: safety
				vector<string> toks;
				Tokenize(toks, line);

				P4Stream s;
				s.stream = toks[1];
				s.type = toks[2];
				m_Streams.push_back(s);
			}
		}
	}
private:
	P4Streams m_Streams;
	
} cStreams("streams");
