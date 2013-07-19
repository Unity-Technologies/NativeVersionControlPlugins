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
			Pipe().WarnLine(msg, MAConfig);
			Pipe().EndResponse();
			return true;
		}
		
		string key = args[1];
		string value = args.size() > 2 ? args[2] : string();
		
		ClearStatus();
		
		string logValue = value;
		if (key == "vcPerforcePassword")
			logValue = "*";

		Pipe().Log().Info() << "Got config " << key << " = '" << logValue << "'" << unityplugin::Endl;

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
		else if (key == "assetsPath")
		{
			// The asset path is just the rest of the config commands joined
			string assetPath;
			for (size_t i = 2; i < args.size(); ++i)
			{
				assetPath += args[i];
				assetPath += " ";
			}
			task.SetAssetsPath(TrimEnd(assetPath));
			Pipe().Log().Info() << "Set assetPath to" << assetPath << unityplugin::Endl;
		}
		else if (key == "vcSharedLogLevel")
		{
			Pipe().Log().Info() << "Set log level to " << value << unityplugin::Endl;
			unityplugin::LogLevel level = unityplugin::LOG_DEBUG;
			if (value == "info")
				level = unityplugin::LOG_INFO;
			else if (value == "notice")
				level = unityplugin::LOG_NOTICE;
			else if (value == "fatal")
				level = unityplugin::LOG_FATAL;
		    Pipe().Log().SetLogLevel(level);
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
		else if (key == "pluginVersions")
		{
			int sel = SelectVersion(args);
			Pipe().DataLine(sel, MAConfig); 
			Pipe().Log().Info() << "Selected plugin protocol version " << sel << unityplugin::Endl;
		}
		else if (key == "pluginTraits")
		{
			// We have 4 flags set
			Pipe().DataLine("6");
			Pipe().DataLine("requiresNetwork", MAConfig); 			
			Pipe().DataLine("enablesCheckout", MAConfig);
			Pipe().DataLine("enablesLocking", MAConfig);
			Pipe().DataLine("enablesRevertUnchanged", MAConfig);
			Pipe().DataLine("enablesChangelists", MAConfig);
			Pipe().DataLine("enablesGetLatestOnChangeSetSubset", MAConfig);

			// We provide 4 configuration fields for the GUI to display
			Pipe().DataLine("4");
			Pipe().DataLine("vcPerforceUsername");               // key
			Pipe().DataLine("Username", MAConfig);               // label
			Pipe().DataLine("The perforce user name", MAConfig); // description
			Pipe().DataLine("");                                 // default
			Pipe().DataLine("1");                                // 1 == required field, 2 == password field

			Pipe().DataLine("vcPerforcePassword");
			Pipe().DataLine("Password", MAConfig);
			Pipe().DataLine("The perforce password", MAConfig);
			Pipe().DataLine("");
			Pipe().DataLine("2"); // password field

			Pipe().DataLine("vcPerforceWorkspace");
			Pipe().DataLine("Workspace", MAConfig);
			Pipe().DataLine("The perforce workspace/client", MAConfig);
			Pipe().DataLine("");
			Pipe().DataLine("1"); // required field

			Pipe().DataLine("vcPerforceServer");
			Pipe().DataLine("Server", MAConfig);
			Pipe().DataLine("The perforce server using format: hostname:port. Port hostname defaults to 'perforce' and port defaults to 1666", MAConfig);
			Pipe().DataLine("perforce");
			Pipe().DataLine("0"); // 

			// We have 11 custom overlay icons
			Pipe().DataLine("overlays");
			Pipe().DataLine("11");
			Pipe().DataLine(IntToString(kLocal)); // for this state
			Pipe().DataLine("default");           // use this path. "default" and "blank" paths can be used when you have not custom overlays.
			Pipe().DataLine(IntToString(kOutOfSync));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kCheckedOutLocal));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kCheckedOutRemote));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kDeletedLocal));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kDeletedRemote));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kAddedLocal));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kAddedRemote));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kConflicted));
			Pipe().DataLine("default");
			Pipe().DataLine(IntToString(kLockedLocal));
			Pipe().DataLine("default");
		    Pipe().DataLine(IntToString(kLockedRemote));
			Pipe().DataLine("default");
		}
		else if (key == "end")
		{
			task.Logout();
			task.Disconnect();
			task.DisableUTF8Mode();
		}
		else 
		{
			Pipe().WarnLine(ToString("Unknown config field set on version control plugin: ", key), MAConfig);
		}
		Pipe().EndResponse();
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

