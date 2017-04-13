#include "P4Command.h"
#include "P4Task.h"
#include "P4Stream.h"
#include "Utility.h"
#include <sstream>

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
			std::string errorMessage = GetStatusMessage();			
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
		std::stringstream ss(data);
		Conn().VerboseLine(data);
		size_t minlen = 5; // "Root:" 
		
		std::string line;
		const std::string streamPrefix = "Stream ";
		while ( getline(ss, line) ) 
		{	
			if (StartsWith(line, streamPrefix))
			{
				// TODO: safety
				std::vector<std::string> toks;
				Tokenize(toks, line);

				P4Stream s;
				s.stream = toks[1];
				s.type = toks[2];
				m_Streams.push_back(s);
			}
		}
	}


  void HandleError( Error *err )
  {
    if ( err == 0 )
      return;
    
    StrBuf buf;
    err->Fmt(&buf);
    std::string value(buf.Text());

    if (value.find("No such stream.") != std::string::npos)
    {
      Conn().Log().Debug() << value << Endl;
      return;      
    }

    P4Command::HandleError(err);
  }

private:
	P4Streams m_Streams;
	
} cStreams("streams");
