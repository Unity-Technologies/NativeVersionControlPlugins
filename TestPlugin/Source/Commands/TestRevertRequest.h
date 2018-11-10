#pragma once
#include "TestCommand.h"

template<>
class TestCommand<RevertRequest>
{
public:
	bool Run(TestTask& task, RevertRequest& req, RevertResponse& resp)
	{
		req.conn.Log().Notice() << "[RevertRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
