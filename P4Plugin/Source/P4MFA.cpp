#include "P4MFA.h"
#include "Utility.h"
#include <stdlib.h>

P4MFA *P4MFA::s_Singleton = NULL;

const std::string aborted_error = "The Helix MFA Authenticator did not complete. Run it again or use the \"p4 login\" or \"p4 login2\" CLI commands";
const std::string notfound_error = "The Helix MFA Authenticator could not be found. Download and install it to continue. https://www.perforce.com/downloads/helix-mfa-authenticator";
#if defined(_WINDOWS)
    const char* kMFAExePath = "\"C:\\Program Files\\Perforce\\HelixMFA.exe\"";
#elif defined(_MACOS)
    const char* kMFAExePath = "/Applications/HelixMFA.app/Contents/MacOS/HelixMFA";
#elif defined(_LINUX)
    const char* kMFAExePath = "helixmfa";
#endif

P4MFA::P4MFA()
{
    if (s_Singleton != NULL)
        throw PluginException("P4MFA::P4MFA - s_Singleton != NULL");
    s_Singleton = this;
}

bool P4MFA::WaitForHelixMFA(Connection& conn, std::string port, std::string user, std::string& error)
{
    conn.VerboseLine(kMFAExePath);
    try {
        POpen proc = POpen(kMFAExePath + std::string(" ") + port + std::string(" ") + user);
#if !defined(_WINDOWS)
        std::string line = std::string();
        bool helixmfa_found = false;
        while (proc.ReadLine(line)) {
            conn.VerboseLine("HelixMFA: " + line);
            //If first line reported by HelixMFA does not contain Authenticating, then we didnt find it
            if (!helixmfa_found && line.find("Authenticating") == std::string::npos)
            {   
                error = notfound_error;
                error += "\nIf this error keeps showing after installation, you can try running HelixMFA manually with the following parameters: Port=" + port + ". User=" + user;
            }
            else {
                helixmfa_found = true;
            }
        }; // Wait until stdout is closed
        if (!helixmfa_found) return false;
    }
#else
    }
    catch (PluginException& pe)
    {
        if (StartsWith(pe.what(), "Process aborted "))
        {   
            error = aborted_error;
        }
        else
        {
            error = notfound_error;
        }
        return false;
    }
#endif
    catch (std::exception& e)
    {
        conn.Log().Fatal() << "Unhandled exception: " << e.what() << Endl;
        return false;
    }
    return true;
}