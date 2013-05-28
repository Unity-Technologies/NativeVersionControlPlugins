#pragma once
#include "Base.h"
#include <set>
#include <algorithm>
#include <iterator>

class ConfigRequest : public BaseRequest
{
public:
	ConfigRequest(const CommandArgs& args, UnityConnection& conn) : BaseRequest(args, conn)
	{
		UnityPipe& upipe = conn.Pipe();

		if (args.size() < 2)
		{
			std::string msg = "Subversion plugin got invalid config setting :";
			for (CommandArgs::const_iterator i = args.begin(); i != args.end(); ++i) {
				msg += " ";
				msg += *i;
			}
			upipe.WarnLine(msg, MAConfig);
			upipe.EndResponse();
			invalid = true;
		}
		
		key = args[1];
		values = std::vector<std::string>(args.begin()+2, args.end());
	}
	
	std::string key;
	std::vector<std::string> values;
	
};

class ConfigResponse : public BaseResponse
{
public:

	enum TraitFlag
	{
		TF_None = 0,
		TF_Required = 1,
		TF_Password = 2
	};

	struct PluginTrait 
	{
		std::string id; // e.g. "svnUsername"
		std::string label; // e.g. "Username"
		std::string description; // e.g. "The subversion user name"
		std::string defaultValue; // e.g. "anonymous"
		int flags;
	};

	ConfigResponse(ConfigRequest& req) 
		: request(req), requiresNetwork(false), 
		  enablesCheckout(true), enablesLocking(true), enablesRevertUnchanged(true)
	{
	}

	void addTrait(const std::string& id, const std::string& label, 
				  const std::string& desc, const std::string& def,
				  int flags)
	{
		PluginTrait t = { id, label, desc, def, flags };
		traits.push_back(t);
	}
	
	void Write()
	{
		if (request.invalid)
			return;

		UnityPipe& upipe = request.conn.Pipe();
		
		if (request.key == "pluginVersions")
		{
			int v = SelectVersion(request.values);
			upipe.DataLine(v, MAConfig);
			upipe.Log().Info() << "Selected plugin protocol version " << v << unityplugin::Endl;
		} 
		else if (request.key == "pluginTraits")
		{
			// First mandatory traits
			int count = 
				(requiresNetwork ? 1 : 0) + 
				(enablesCheckout ? 1 : 0) + 
				(enablesLocking  ? 1 : 0) +
				(enablesRevertUnchanged  ? 1 : 0);

			upipe.DataLine(count);

			if (requiresNetwork)
				upipe.DataLine("requiresNetwork", MAConfig); 
			if (enablesCheckout)
				upipe.DataLine("enablesCheckout", MAConfig); 
			if (enablesLocking)
				upipe.DataLine("enablesLocking", MAConfig); 
			if (enablesRevertUnchanged)
				upipe.DataLine("enablesRevertUnchanged", MAConfig); 

			// The per plugin defined traits
			upipe.DataLine(traits.size());

			for (std::vector<PluginTrait>::const_iterator i = traits.begin();
				 i != traits.end(); ++i)
			{
				upipe.DataLine(i->id);
				upipe.DataLine(i->label, MAConfig);
				upipe.DataLine(i->description, MAConfig);
				upipe.DataLine(i->defaultValue);
				upipe.DataLine(i->flags);
			}
		}
		upipe.EndResponse();
	}
	
	bool requiresNetwork;
	bool enablesCheckout;
	bool enablesLocking;
	bool enablesRevertUnchanged;
	std::vector<PluginTrait> traits;
	std::set<int> supportedVersions;

	ConfigRequest request;
private:

	int SelectVersion(const CommandArgs& args)
	{
		std::set<int> unitySupportedVersions;
		std::set<int>& pluginSupportedVersions = supportedVersions;
		
		pluginSupportedVersions.insert(1);
		
		// Read supported versions from unity
		CommandArgs::const_iterator i = args.begin();
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

};
