#include "P4Command.h"
#include "P4Task.h"
#include "POpen.h"
#include <sstream>
#include <stdio.h>

class P4SpecCommand : public P4Command
{
public:
	P4SpecCommand(const char* name) : P4Command(name) { }
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_Root.clear();

		Conn().Log().Info() << args[0] << "::Run()" << Endl;
		const std::string fallback_error = (args.size() > 1) ? args[1] : std::string();

		const std::string cmd = std::string("client -o ") + Quote(task.GetP4Client());
		if (!task.CommandRun(cmd, this))
		{
			//MFA: This error is handled here instead of in P4LoginCommand as it only shows up when running the first non-login command (currently always spec)
			if (StatusContains(GetStatus(), "login2"))
			{
				ClearStatus();
#if defined(_WINDOWS)
				static const char* kMFAExePath = "\"C:\\Program Files\\Perforce\\HelixMFA.exe\"";
#elif defined(_MACOS)
				static const char* kMFAExePath = "/Applications/HelixMFA.app/Contents/MacOS/HelixMFA";
#elif defined(_LINUX)
				static const char* kMFAExePath = "helixmfa";
#endif
				Conn().VerboseLine(kMFAExePath);
				try {
					POpen proc = POpen(kMFAExePath + std::string(" ") + task.GetP4Port() + std::string(" ") + task.GetP4User());
#if !defined(_WINDOWS)
					std::string line = std::string();
					bool helixmfa_found = false;
					while (proc.ReadLine(line)) {
						Conn().VerboseLine(std::string("HelixMFA: ") + std::string(line));
						//If first line reported by HelixMFA does not contain Authenticating, then we didnt find it
						if (!helixmfa_found && line.find("Authenticating") == std::string::npos) {
							const std::string notfound_error = "HelixMFA Authenticator could not be found. Download and install it to continue. https://www.perforce.com/downloads/helix-mfa-authenticator\n"
							"If this error keeps showing after installation, you can try running HelixMFA manually with the following parameters: Port=" + task.GetP4Port() + ". User=" + task.GetP4User();
							GetStatus().insert(VCSStatusItem(VCSSEV_Error, notfound_error));
							Conn().Log().Notice() << GetStatusMessage() << Endl;
						}
						else {
							helixmfa_found = true;
						}
					}; // Wait until stdout is closed
					if (!helixmfa_found) return false;
					//We are not closing the handle because PerforcePlugin is terminated after reconnection and closes them all. Thus it would fail
					//proc.~POpen();
				}
#else
				}
				catch (PluginException& pe)
				{
					if (StartsWith(pe.what(), "Process aborted "))
					{
						const std::string aborted_error = "The Helix MFA Authenticator did not complete. Run it again or use the \"p4 login\" or \"p4 login2\" CLI commands";
						GetStatus().insert(VCSStatusItem(VCSSEV_Error, aborted_error));
					}
					else
					{
						const std::string notfound_error = "The Helix MFA Authenticator could not be found. Download and install it to continue. https://www.perforce.com/downloads/helix-mfa-authenticator";
						GetStatus().insert(VCSStatusItem(VCSSEV_Error, notfound_error));
					}
					Conn().Log().Notice() << GetStatusMessage() << Endl;
					return false;
				}
#endif
				catch (std::exception& e)
				{
					Conn().Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
					return false;
				}
				//Try again the spec command so that we can get a value for m_Root
				if (!task.CommandRun(cmd, this))
				{
					if (StatusContains(GetStatus(), "login2"))
					{
						GetStatus().insert(VCSStatusItem(VCSSEV_Error, fallback_error));
						Conn().Log().Fatal() << GetStatusMessage() << Endl;
						return false;
					}
				}
			}
		}
		if (!m_Root.empty())
			task.SetP4Root(m_Root);
		Conn().Log().Info() << "Root set to " << m_Root << Endl;
		return true;
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
