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
		if (args.size() < 2)
		{
			upipe.Warn("Perforce plugin got invalid config setting :", MAConfig);
			for (CommandArgs::const_iterator i = args.begin(); i != args.end(); ++i) {
				upipe.Write(" ");
				upipe.Write(*i);
			}
			upipe.WriteLine("");
			upipe.EndResponse();
			return true;
		}
		
		string key = args[1];
		string value = args.size() > 2 ? args[2] : string();
		
		ClearStatus();
		
		string logValue = value;
		if (key == "vcPassword")
			logValue = "*";
		upipe.Log() << "Got config " << key << " = '" << logValue << "'" << endl;

		// This command actually handles several commands all 
		// concerning connecting to the perforce server
		if (key == "vcUsername")
		{
			task.SetP4User(value);
		}
		else if (key == "vcWorkspace")
		{
			task.SetP4Client(value);
		}
		else if (key == "assetsPath")
		{
			task.SetAssetsPath(value);
		}
		else if (key == "vcPassword")
		{
			task.SetP4Password(value);
			value = "*";
		}
		else if (key == "vcServer")
		{
			string::size_type i = value.find(":");
			if (i == string::npos)
				value += ":1666"; // default port
			task.SetP4Port(value);
		}
		else if (key == "pluginVersions")
		{
			int sel = SelectVersion(args);
			upipe.OkLine(sel, MAConfig); 
			upipe.Log() << "Selected plugin protocol version " << sel << endl;
		}
		else if (key == "pluginTraits")
		{
			upipe.OkLine("1");
			upipe.OkLine("requiresNetwork", MAConfig); // requires network			
		
			upipe.OkLine("4");
			upipe.OkLine("vcUsername");
			upipe.OkLine("Username", MAConfig);
			upipe.OkLine("The perforce user name", MAConfig);
			upipe.OkLine("");
			upipe.OkLine("1"); // required field

			upipe.OkLine("vcPassword");
			upipe.OkLine("Password", MAConfig);
			upipe.OkLine("The perforce password", MAConfig);
			upipe.OkLine("");
			upipe.OkLine("3"); // required field | password field

			upipe.OkLine("vcWorkspace");
			upipe.OkLine("Workspace", MAConfig);
			upipe.OkLine("The perforce workspace/client", MAConfig);
			upipe.OkLine("");
			upipe.OkLine("1"); // required field

			upipe.OkLine("vcServer");
			upipe.OkLine("Server", MAConfig);
			upipe.OkLine("The perforce server using format: hostname:port. Port hostname defaults to 'perforce' and port defaults to 1666", MAConfig);
			upipe.OkLine("perforce");
			upipe.OkLine("1"); // required field
		} 
		else 
		{
			upipe.Warn("Unknown config field set on version control plugin: ", MAConfig);
			upipe.WriteLine(key);
		}
		upipe.EndResponse();
		return true;
	}
	
	int SelectVersion(const CommandArgs& args)
	{
		set<int> unitySupportedVersions;
		set<int> pluginSupportedVersions;
		
		pluginSupportedVersions.insert(1);
		
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

