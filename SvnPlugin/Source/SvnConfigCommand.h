#pragma once

template<>
class SvnCommand<ConfigRequest>
{
public:
	bool Run(SvnTask& task, ConfigRequest& req, ConfigResponse& resp)
	{
		std::string val = req.Values();
		switch (req.key)
		{
		case CK_Traits:
			resp.requiresNetwork = false;
			resp.enablesCheckout = false;
			resp.enablesLocking = true;
			resp.enablesRevertUnchanged = false;
			resp.addTrait("vcSubversionUsername", "Username", "Subversion username", "", ConfigResponse::TF_None);
			resp.addTrait("vcSubversionPassword", "Password", "Subversion password", "", ConfigResponse::TF_None | ConfigResponse::TF_Password);
			//resp.addTrait("vcSubversionRepos", "Repository", "Subversion Repository", "", ConfigResponse::TF_None);
			resp.addTrait("vcSubversionOptions", "Options", "Subversion extra options", "", ConfigResponse::TF_None);
			resp.addTrait("vcSubversionExecutable", "Executable", "Path to the svn.exe executable", task.GetSvnExecutable(), ConfigResponse::TF_None);
			break;

		case CK_Versions:
			resp.AddSupportedVersion(1);
			break;

		case CK_AssetsPath:
			req.conn.Log().Info() << "Set assetsPath to " << val << unityplugin::Endl;
			task.SetAssetsPath(val);
			break;

		case CK_LogLevel:
			req.conn.Log().Info() << "Set log level to " << val << unityplugin::Endl;
		    req.conn.Log().SetLogLevel(req.GetLogLevel());
			break;

		case CK_Unknown:
			if (req.keyStr == "vcSubversionRepos")
			{
				req.conn.Log().Info() << "Set repos to " << val << unityplugin::Endl;
				task.SetRepository(val);
			}
			else if (req.keyStr == "vcSubversionUsername")
			{
				req.conn.Log().Info() << "Set username to " << val << unityplugin::Endl;
				task.SetUser(val);
			}		
			else if (req.keyStr == "vcSubversionPassword")
			{
				req.conn.Log().Info() << "Set password to *********" << unityplugin::Endl;
				task.SetPassword(val);
			}
			else if (req.keyStr == "vcSubversionOptions")
			{
				req.conn.Log().Info() << "Set options to " << val << unityplugin::Endl;
				task.SetOptions(val);
			}
			else if (req.keyStr == "vcSubversionExecutable")
			{
				req.conn.Log().Info() << "Set executable path to \"" << val << "\"" << unityplugin::Endl;
				task.SetSvnExecutable(val);
			}
			else
			{
				std::string msg = "Unknown config field set on subversion plugin: ";
				msg += req.keyStr;
				req.conn.Pipe().WarnLine(msg, MAConfig);
				req.invalid = true;
			}
		}

		resp.Write();
		return true;
	}
};
