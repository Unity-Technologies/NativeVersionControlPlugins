#pragma once
#include "TestCommand.h"

template<>
class TestCommand<LockRequest>
{
public:
	bool Run(TestTask& task, LockRequest& req, LockResponse& resp)
	{
		req.conn.Log().Notice() << "[LockRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
