#pragma once

template<>
class SvnCommand<ConfigRequest>
{
public:
	bool Run(SvnTask& task, ConfigRequest& req, ConfigResponse& resp)
	{
		std::string val = Join(req.values, " ");

		if (req.key == "pluginTraits")
		{
			resp.requiresNetwork = false;
			resp.enablesCheckout = false;
			resp.enablesLocking = true;
			resp.enablesRevertUnchanged = false;
			resp.addTrait("vcSubversionUsername", "Username", "Subversion username", "", ConfigResponse::TF_Required);
			resp.addTrait("vcSubversionPassword", "Password", "Subversion password", "", ConfigResponse::TF_Required | ConfigResponse::TF_Password);
			//resp.addTrait("vcSubversionRepos", "Repository", "Subversion Repository", "", ConfigResponse::TF_Required);
			resp.addTrait("vcSubversionOptions", "Options", "Subversion extra options", "", ConfigResponse::TF_None);
			resp.addTrait("vcSubversionExecutable", "Executable", "Path to the svn.exe executable", task.GetSvnExecutable(), ConfigResponse::TF_None);
		}
		else if (req.key == "pluginVersions")
		{
			resp.supportedVersions.insert(1);
		}
		else if (req.key == "assetsPath")
		{
			req.conn.Log() << "Set assetsPath to " << val << unityplugin::Endl;
			task.SetAssetsPath(val);
		}
		else if (req.key == "vcSubversionRepos")
		{
			req.conn.Log() << "Set repos to " << val << unityplugin::Endl;
			task.SetRepository(val);
		}
		else if (req.key == "vcSubversionUsername")
		{
			req.conn.Log() << "Set username to " << val << unityplugin::Endl;
			task.SetUser(val);
		}		
		else if (req.key == "vcSubversionPassword")
		{
			req.conn.Log() << "Set password to *********" << unityplugin::Endl;
			task.SetPassword(val);
		}
		else if (req.key == "vcSubversionOptions")
		{
			req.conn.Log() << "Set options to " << val << unityplugin::Endl;
			task.SetOptions(val);
		}
		else if (req.key == "vcSubversionExecutable")
		{
			req.conn.Log() << "Set executable path to \"" << val << "\"" << unityplugin::Endl;
			task.SetSvnExecutable(val);
		}
		else
		{
			std::string msg = "Unknown config field set on subversion plugin: ";
			msg += req.key;
			req.conn.Pipe().WarnLine(msg, MAConfig);
			req.invalid = true;
		}

		resp.Write();
		return true;
	}
};
