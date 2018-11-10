#pragma once
#include "TestCommand.h"

template<>
class TestCommand<MoveRequest>
{
public:
	bool Run(TestTask& task, MoveRequest& req, MoveResponse& resp)
	{
		req.conn.Log().Notice() << "[MoveRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
