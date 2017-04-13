#include "P4Command.h"
#include "P4Task.h"
#include <set>
#include <algorithm>
#include <iterator>

class P4ConfigCommand : public P4Command
{
public:
	P4ConfigCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		//ConfigRequest req(args, task.
		if (args.size() < 2)
		{
			std::string msg = "Perforce plugin got invalid config setting :"; 
			for (CommandArgs::const_iterator i = args.begin(); i != args.end(); ++i) {
				msg += " ";
				msg += *i;
			}
			Conn().WarnLine(msg, MAConfig);
			Conn().EndResponse();
			return true;
		}
		
		std::string key = args[1];
		std::string value = Join(args.begin() + 2, args.end(), " ");
		
		ClearStatus();
		
		std::string logValue = value;
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
			std::string p4address = value;
			bool foundAddress = false;
			std::string protocol = "";
			std::string address = "";
			std::string port = "";
			// Perforce server field is: <protocol>:<address>:<port>
			// <protocol> is optional but can be:
			// 		ssl: ssl4: ssl6: ssl46: ssl64:
			// 		tcp: tcp4: tcp6: tcp46: tcp64:
			// <address> can be IPv4 or IPv6
			// P4 documentation says this
			//		IPV6 numeric addresses should be surrounded with [] to isolate the IPv6 : from the : token to mark the <port>
			//		[] can be used to surround an IPv4 or IPv6 address
			// But in practice it isn't true and putting [] around a Helix cloud server stops it working
			// Example tests pushed through this code to verify it:
			//		Value:'' Output:'perforce:1666' Protocol:'' AddressPort:'perforce:1666' Address:'perforce' Port:'1666' ###
			//		Value:':1667' Output:'perforce:1667' Protocol:'' AddressPort:'perforce:1667' Address:'perforce' Port:'1667' ###
			//		Value:'localhost' Output:'localhost:1666' Protocol:'' AddressPort:'localhost:1666' Address:'localhost' Port:'1666' ###
			//		Value:'localhost:1667' Output:'localhost:1667' Protocol:'' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'[localhost]' Output:'[localhost]:1666' Protocol:'' AddressPort:'[localhost]:1666' Address:'[localhost]' Port:'1666' ###
			//		Value:'[localhost]:1667' Output:'[localhost]:1667' Protocol:'' AddressPort:'[localhost]:1667' Address:'[localhost]' Port:'1667' ###
			//		Value:'tcp:localhost:1667' Output:'tcp:localhost:1667' Protocol:'tcp:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'tcp4:localhost:1667' Output:'tcp4:localhost:1667' Protocol:'tcp4:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'tcp6:localhost:1667' Output:'tcp6:localhost:1667' Protocol:'tcp6:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'tcp46:localhost:1667' Output:'tcp46:localhost:1667' Protocol:'tcp46:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'tcp64:localhost:1667' Output:'tcp64:localhost:1667' Protocol:'tcp64:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'ssl:localhost:1667' Output:'ssl:localhost:1667' Protocol:'ssl:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'ssl4:localhost:1667' Output:'ssl4:localhost:1667' Protocol:'ssl4:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'ssl6:localhost:1667' Output:'ssl6:localhost:1667' Protocol:'ssl6:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'ssl46:localhost:1667' Output:'ssl46:localhost:1667' Protocol:'ssl46:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'ssl64:localhost:1667' Output:'ssl64:localhost:1667' Protocol:'ssl64:' AddressPort:'localhost:1667' Address:'localhost' Port:'1667' ###
			//		Value:'tcp:[localhost]' Output:'tcp:[localhost]:1666' Protocol:'tcp:' AddressPort:'[localhost]:1666' Address:'[localhost]' Port:'1666' ###
			//		Value:'tcp:[localhost]:1667' Output:'tcp:[localhost]:1667' Protocol:'tcp:' AddressPort:'[localhost]:1667' Address:'[localhost]' Port:'1667' ###
			//		Value:'[::1]' Output:'[::1]:1666' Protocol:'' AddressPort:'[::1]:1666' Address:'[::1]' Port:'1666' ###
			//		Value:'[::1]:1667' Output:'[::1]:1667' Protocol:'' AddressPort:'[::1]:1667' Address:'[::1]' Port:'1667' ###
			//		Value:'tcp:[::1]:1667' Output:'tcp:[::1]:1667' Protocol:'tcp:' AddressPort:'[::1]:1667' Address:'[::1]' Port:'1667' ###

			if (StartsWith(p4address, "ssl") || StartsWith(p4address, "tcp"))
			{
				const std::string temp = p4address.substr(3);
				const std::string validSubProtocols[] = { ":", "4:", "6:", "46:", "64:", "END" };
				int i = 0;
				while (validSubProtocols[i] != "END")
				{
					if (StartsWith(temp, validSubProtocols[i]))
					{
						protocol = p4address.substr(0,3) + validSubProtocols[i];
						break;
					}
					++i;
				}
			}
			const std::string::size_type addressStart = protocol.length();
			std::string addressPort = p4address.substr(addressStart);
			// If the address Port contains [] take that as the address
			const std::string::size_type leftSqBracket = addressPort.find('[');
			const std::string::size_type rightSqBracket = addressPort.find(']');
			if ((leftSqBracket != std::string::npos) && (rightSqBracket != std::string::npos) && (leftSqBracket < rightSqBracket))
			{
				address = addressPort.substr(leftSqBracket, (rightSqBracket-leftSqBracket+1));
				const std::string::size_type finalColon = addressPort.rfind(":");
				if ((finalColon != std::string::npos) && (finalColon > rightSqBracket))
					port = addressPort.substr(finalColon+1);
				foundAddress = true;
			}
			else
			{
				// If the address Port has a single (or no) colon in it split into <address>:<port> and reconstruct as [<address>]:<port> otherwise leave alone
				const std::string::size_type finalColon = addressPort.rfind(":");
				const std::string::size_type firstColon = addressPort.find(":");
				if (firstColon == finalColon)
				{
					address = addressPort.substr(0,firstColon);
					if (finalColon != std::string::npos)
						port = addressPort.substr(finalColon+1);
					foundAddress = true;
				}
			}

			if (p4address.empty())
				foundAddress = true;

			if (foundAddress)
			{
				// Perforce defaults
				if (address.empty())
					address = "perforce";
				if (port.empty())
					port = "1666";
				addressPort = address + ":" + port;
			}

			p4address = protocol + addressPort;

			Conn().Log().Info() << "### P4Address Value:'" << value << "' Output:'" << p4address << "' Protocol:'" << protocol << "' AddressPort:'" << addressPort << "' Address:'" << address << "' Port:'" << port << "' ###" << Endl;
			task.SetP4Port(p4address);
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
			Conn().DataLine("0");                                // 1 == required field, 2 == password field

			Conn().DataLine("vcPerforcePassword");
			Conn().DataLine("Password", MAConfig);
			Conn().DataLine("The perforce password", MAConfig);
			Conn().DataLine("");
			Conn().DataLine("2"); // password field

			Conn().DataLine("vcPerforceWorkspace");
			Conn().DataLine("Workspace", MAConfig);
			Conn().DataLine("The perforce workspace/client", MAConfig);
			Conn().DataLine("");
			Conn().DataLine("0"); 

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
		std::set<int> unitySupportedVersions;
		std::set<int> pluginSupportedVersions;
		
		pluginSupportedVersions.insert(2);
		
		// Read supported versions from unity
		CommandArgs::const_iterator i = args.begin();
		i += 2;
		for	( ; i != args.end(); ++i)
		{
			unitySupportedVersions.insert(atoi(i->c_str()));
		}
		
		std::set<int> candidates;
		std::set_intersection(unitySupportedVersions.begin(), unitySupportedVersions.end(),
						 pluginSupportedVersions.begin(), pluginSupportedVersions.end(),
						 inserter(candidates, candidates.end()));
		if (candidates.empty())
			return -1;
		
		return *candidates.rbegin();
	}
	
} cConfig("pluginConfig");

