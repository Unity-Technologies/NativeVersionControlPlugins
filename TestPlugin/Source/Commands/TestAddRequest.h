#pragma once
#include "TestCommand.h"

template<>
class TestCommand<AddRequest>
{
public:
	bool Run(TestTask& task, AddRequest& req, AddResponse& resp)
	{
		req.conn.Log().Notice() << "[AddRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
