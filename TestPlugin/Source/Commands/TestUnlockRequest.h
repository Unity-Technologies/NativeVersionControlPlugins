#pragma once
#include "TestCommand.h"

template<>
class TestCommand<UnlockRequest>
{
public:
	bool Run(TestTask& task, UnlockRequest& req, UnlockResponse& resp)
	{
		req.conn.Log().Notice() << "[UnlockRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
