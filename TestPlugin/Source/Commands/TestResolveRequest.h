#pragma once
#include "TestCommand.h"

template<>
class TestCommand<ResolveRequest>
{
public:
	bool Run(TestTask& task, ResolveRequest& req, ResolveResponse& resp)
	{
		req.conn.Log().Notice() << "[ResolveRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
