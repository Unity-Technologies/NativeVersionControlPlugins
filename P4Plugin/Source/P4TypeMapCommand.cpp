#include "P4Command.h"
#include "P4Task.h"
#include <sstream>

class P4TypeMapCommand : public P4Command
{
public:
	P4TypeMapCommand() : P4Command("typemap") {}

	bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();

		if (!task.CommandRun("typemap -o", this))
		{
			std::string errorMessage = GetStatusMessage();			
			Conn().Log().Fatal() << errorMessage << Endl;
			return false;
		}
		else
		{
			return true;
		}
	}

	virtual void OutputInfo(char level, const char *data)
	{
		std::stringstream ss(data);
		Conn().VerboseLine(data);
		std::string line;
		Conn().BeginList();
		while(getline(ss, line))
		{
			if (StartsWith(TrimStart(line), "#"))
				continue;
			if (TrimStart(TrimEnd(line)).size() == 0)
				continue;
			if (StartsWith(TrimStart(line), "TypeMap:"))
				continue;
			std::string trimmedLine = (TrimStart(TrimEnd(line)));
			std::string::size_type separator = trimmedLine.find(' ');
			if (separator == std::string::npos)
				Conn().WarnLine("Got unexpected output when parsing Perforce TypeMap: " + line);
			else
			{
				std::string type = trimmedLine.substr(0, separator);
				bool include = trimmedLine[separator+1] != '-';
				std::string depotPath = trimmedLine.substr(include ? separator+1 : separator+2);
				Conn().Log().Debug() << "TM Out:" << type << " " << include << " " << depotPath << "\n";
			}
		}
		Conn().EndList();
		Conn().EndResponse();
	}

} cTypeMapPt;

