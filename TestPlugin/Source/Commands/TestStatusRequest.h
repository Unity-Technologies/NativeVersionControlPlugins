#pragma once
#include "TestCommand.h"

template<>
class TestCommand<StatusRequest>
{
public:
	bool Run(TestTask& task, StatusRequest& req, StatusResponse& resp)
	{
		req.conn.Log().Notice() << "[StatusRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;

		req.conn.BeginList();
		for (size_t i = 0; i < req.assets.size(); ++i)
		{
			VersionedAsset asset = req.assets[i];
			asset.SetState(kSynced);
			req.conn << asset;
		}
		req.conn.EndList();
		req.conn.EndResponse();
		return true;
	}
};
