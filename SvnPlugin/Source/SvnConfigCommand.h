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
			resp.addTrait("vcSubversionUsername", "Username", "Subversion username", "", ConfigResponse::TF_None);
			resp.addTrait("vcSubversionPassword", "Password", "Subversion password", "", ConfigResponse::TF_None | ConfigResponse::TF_Password);
			//resp.addTrait("vcSubversionRepos", "Repository", "Subversion Repository", "", ConfigResponse::TF_None);
			resp.addTrait("vcSubversionOptions", "Options", "Subversion extra options", "", ConfigResponse::TF_None);
			resp.addTrait("vcSubversionExecutable", "Executable", "Path to the svn.exe executable", task.GetSvnExecutable(), ConfigResponse::TF_None);
		}
		else if (req.key == "pluginVersions")
		{
			resp.supportedVersions.insert(1);
		}
		else if (req.key == "assetsPath")
		{
			req.conn.Log().Info() << "Set assetsPath to " << val << unityplugin::Endl;
			task.SetAssetsPath(val);
		}
		else if (req.key == "vcSharedLogLevel")
		{
			req.conn.Log().Info() << "Set log level to " << val << unityplugin::Endl;
			unityplugin::LogLevel level = unityplugin::LOG_DEBUG;
			if (val == "info")
				level = unityplugin::LOG_INFO;
			else if (val == "notice")
				level = unityplugin::LOG_NOTICE;
			else if (val == "fatal")
				level = unityplugin::LOG_FATAL;
		    req.conn.Log().SetLogLevel(level);
		}
		else if (req.key == "vcSubversionRepos")
		{
			req.conn.Log().Info() << "Set repos to " << val << unityplugin::Endl;
			task.SetRepository(val);
		}
		else if (req.key == "vcSubversionUsername")
		{
			req.conn.Log().Info() << "Set username to " << val << unityplugin::Endl;
			task.SetUser(val);
		}		
		else if (req.key == "vcSubversionPassword")
		{
			req.conn.Log().Info() << "Set password to *********" << unityplugin::Endl;
			task.SetPassword(val);
		}
		else if (req.key == "vcSubversionOptions")
		{
			req.conn.Log().Info() << "Set options to " << val << unityplugin::Endl;
			task.SetOptions(val);
		}
		else if (req.key == "vcSubversionExecutable")
		{
			req.conn.Log().Info() << "Set executable path to \"" << val << "\"" << unityplugin::Endl;
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
