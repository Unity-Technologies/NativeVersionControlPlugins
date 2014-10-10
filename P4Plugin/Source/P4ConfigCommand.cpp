#include "P4Command.h"
#include "P4Task.h"
#include <set>
#include <algorithm>
#include <iterator>

using namespace std;

class P4ConfigCommand : public P4Command
{
public:
	P4ConfigCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		//ConfigRequest req(args, task.
		if (args.size() < 2)
		{
			string msg = "Perforce plugin got invalid config setting :"; 
			for (CommandArgs::const_iterator i = args.begin(); i != args.end(); ++i) {
				msg += " ";
				msg += *i;
			}
			Conn().WarnLine(msg, MAConfig);
			Conn().EndResponse();
			return true;
		}
		
		string key = args[1];
		string value = Join(args.begin() + 2, args.end(), " ");
		
		ClearStatus();
		
		string logValue = value;
		if (key == "vcPerforcePassword")
			logValue = "*";

		Conn().Log().Info() << "Got config " << key << " = '" << logValue << "'" << Endl;

		// This command actually handles several commands all 
		// concerning connecting to the perforce server
		if (key == "vcPerforceUsername") 
		{
			task.SetP4User(value);
		}
		else if (key == "vcPerforceWorkspace")
		{
			task.SetP4Client(value);
		}
		else if (key == "projectPath")
		{
			task.SetProjectPath(TrimEnd(value));
			Conn().Log().Info() << "Set projectPath to" << value << Endl;
		}
		else if (key == "vcSharedLogLevel")
		{
			Conn().Log().Info() << "Set log level to " << value << Endl;
			LogLevel level = LOG_DEBUG;
			if (value == "info")
				level = LOG_INFO;
			else if (value == "notice")
				level = LOG_NOTICE;
			else if (value == "fatal")
				level = LOG_FATAL;
		    Conn().Log().SetLogLevel(level);
		}
		else if (key == "vcPerforcePassword")
		{
			task.SetP4Password(value);
			value = "*";
		}
		else if (key == "vcPerforceServer")
		{
			if (value.empty())
				value = "perforce";
			
			string::size_type i = (StartsWith(value, "ssl:") ? value.substr(4) : value).find(":");
			if (i == string::npos)
				value += ":1666"; // default port
			task.SetP4Port(value);
		}
		else if (key == "vcPerforceHost")
		{
			task.SetP4Host(value);
		}
		else if (key == "pluginVersions")
		{
			int sel = SelectVersion(args);
			Conn().DataLine(sel, MAConfig); 
			Conn().Log().Info() << "Selected plugin protocol version " << sel << Endl;
		}
		else if (key == "pluginTraits")
		{
			// We have 4 flags set
			Conn().DataLine("6");
			Conn().DataLine("requiresNetwork", MAConfig); 			
			Conn().DataLine("enablesCheckout", MAConfig);
			Conn().DataLine("enablesLocking", MAConfig);
			Conn().DataLine("enablesRevertUnchanged", MAConfig);
			Conn().DataLine("enablesChangelists", MAConfig);
			Conn().DataLine("enablesGetLatestOnChangeSetSubset", MAConfig);

			// We provide 4 configuration fields for the GUI to display
			Conn().DataLine("5");
			Conn().DataLine("vcPerforceUsername");               // key
			Conn().DataLine("Username", MAConfig);               // label
			Conn().DataLine("The perforce user name", MAConfig); // description
			Conn().DataLine("");                                 // default
			Conn().DataLine("1");                                // 1 == required field, 2 == password field

			Conn().DataLine("vcPerforcePassword");
			Conn().DataLine("Password", MAConfig);
			Conn().DataLine("The perforce password", MAConfig);
			Conn().DataLine("");
			Conn().DataLine("2"); // password field

			Conn().DataLine("vcPerforceWorkspace");
			Conn().DataLine("Workspace", MAConfig);
			Conn().DataLine("The perforce workspace/client", MAConfig);
			Conn().DataLine("");
			Conn().DataLine("1"); // required field

			Conn().DataLine("vcPerforceServer");
			Conn().DataLine("Server", MAConfig);
			Conn().DataLine("The perforce server using format: hostname:port. Port hostname defaults to 'perforce' and port defaults to 1666", MAConfig);
			Conn().DataLine("perforce");
			Conn().DataLine("0"); // 

			Conn().DataLine("vcPerforceHost");
			Conn().DataLine("Host", MAConfig);
			Conn().DataLine("The perforce host ie. P4HOST. It can often be left blank.", MAConfig);
			Conn().DataLine("");
			Conn().DataLine("0"); // 

			// We have 11 custom overlay icons
			Conn().DataLine("overlays");
			Conn().DataLine("11");
			Conn().DataLine(IntToString(kLocal)); // for this state
			Conn().DataLine("default");           // use this path. "default" and "blank" paths can be used when you have not custom overlays.
			Conn().DataLine(IntToString(kOutOfSync));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kCheckedOutLocal));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kCheckedOutRemote));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kDeletedLocal));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kDeletedRemote));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kAddedLocal));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kAddedRemote));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kConflicted));
			Conn().DataLine("default");
			Conn().DataLine(IntToString(kLockedLocal));
			Conn().DataLine("default");
		    Conn().DataLine(IntToString(kLockedRemote));
			Conn().DataLine("default");
		}
		else if (key == "end")
		{
			task.Logout();
			if (task.Reconnect())
				task.Login();
		}
		else 
		{
			Conn().WarnLine(ToString("Unknown config field set on version control plugin: ", key), MAConfig);
		}
		Conn().EndResponse();
		return true;
	}
	
	int SelectVersion(const CommandArgs& args)
	{
		set<int> unitySupportedVersions;
		set<int> pluginSupportedVersions;
		
		pluginSupportedVersions.insert(2);
		
		// Read supported versions from unity
		CommandArgs::const_iterator i = args.begin();
		i += 2;
		for	( ; i != args.end(); ++i)
		{
			unitySupportedVersions.insert(atoi(i->c_str()));
		}
		
		set<int> candidates;
		set_intersection(unitySupportedVersions.begin(), unitySupportedVersions.end(),
						 pluginSupportedVersions.begin(), pluginSupportedVersions.end(),
						 inserter(candidates, candidates.end()));
		if (candidates.empty())
			return -1;
		
		return *candidates.rbegin();
	}
	
} cConfig("pluginConfig");

