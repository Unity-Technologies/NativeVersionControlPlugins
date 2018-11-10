#pragma once
#include "TestCommand.h"

template<>
class TestCommand<GetLatestRequest>
{
public:
	bool Run(TestTask& task, GetLatestRequest& req, GetLatestResponse& resp)
	{
		req.conn.Log().Notice() << "[GetLatestRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
