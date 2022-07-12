#include "P4Command.h"
#include "P4Task.h"
#include "POpen.h"
#include <sstream>

class P4SpecCommand : public P4Command
{
public:
	P4SpecCommand(const char* name) : P4Command(name) { }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_Root.clear();

		const std::string cmd = std::string("client -o ") + Quote(task.GetP4Client());

		if (!task.CommandRun(cmd, this))
		{
			std::string errorMessage = GetStatusMessage();
			//This error is handled here instead of in P4LoginCommand as it only shows up when running the first non-login command (currently always spec)
			if (StatusContains(GetStatus(), "login2"))
			{
				ClearStatus();
				const std::string exePath = std::string(
#ifdef _MACOS
						"/Applications/HelixMFA/HelixMFA.app/Contents/MacOS/HelixMFA"
#else
						"HelixMFA"
#endif
				);

				try {
					POpen proc = POpen(exePath + std::string(" ") + task.GetP4Port() + std::string(" ") + task.GetP4User()));
					proc.ReadIntoFile(std::string("./helixmfa.txt")); // Wait until stdout is closed
					proc.~POpen();
				} catch (PluginException e) {
					GetStatus().insert(VCSStatusItem(VCSSEV_Error, "Your chosen Perforce server has multi-factor authentication enabled. To log in please install HelixMFA or use the \"p4 login\" or \"p4 login2\" CLI commands"));
				}
			}
			Conn().Log().Fatal() << errorMessage << Endl;
			return false;
		}
		else
		{
			if (!m_Root.empty())
				task.SetP4Root(m_Root);
			Conn().Log().Info() << "Root set to " << m_Root << Endl;
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
		while ( getline(ss, line) )
		{

			if (line.length() <= minlen || line.substr(0,minlen) != "Root:")
				continue;

			// Got the Root specification line
			std::string::size_type i = line.find_first_not_of(" \t:", minlen-1);
			if (i == std::string::npos)
			{

				GetStatus().insert(VCSStatusItem(VCSSEV_Error, std::string("invalid root specification - ") + line));
				return;
			}
			m_Root = line.substr(i);
			break;
		}
	}
private:
	std::string m_Root;

} cSpec("spec");
