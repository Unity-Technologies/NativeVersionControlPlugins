#pragma once
#include "TestCommand.h"

template<>
class TestCommand<ConfigRequest>
{
public:
	bool Run(TestTask& task, ConfigRequest& req, ConfigResponse& resp)
	{
		req.conn.Log().Notice() << "[ConfigRequest]" << Endl;
		std::string values = req.Values();
		req.conn.Log().Notice() << "Config '" << req.keyStr << "' = '" << values << "'" << Endl;

		switch (req.key)
		{
			case CK_Traits:
				resp.requiresNetwork = false;
				resp.enablesCheckout = true;
				resp.enablesLocking = true;
				resp.enablesRevertUnchanged = false;
				resp.addTrait("vcTestTrait", "TestTrait", "Test Plugin Trait Option", "", ConfigResponse::TF_None);
				break;
			case CK_Versions:
				resp.AddSupportedVersion(2);
				break;
			case CK_ProjectPath:
				req.conn.Log().Info() << "Set projectPath to " << req.Values() << Endl;
				break;
			case CK_LogLevel:
			    req.conn.Log().SetLogLevel(req.GetLogLevel());
				req.conn.Log().Info() << "Set log level to " << req.GetLogLevel() << Endl;
				break;
			case CK_End:
				req.conn.Log().Info() << "Pluging config end" << Endl;
				req.conn.Command("online", MAProtocol);
				break;
			case CK_Unknown:
			default:
				std::string msg = "Unknown Config field: ";
				msg += req.keyStr;
				req.conn.WarnLine(msg, MAConfig);
				req.invalid = true;
				break;
		}
		resp.Write();
		return true;
	}
};
