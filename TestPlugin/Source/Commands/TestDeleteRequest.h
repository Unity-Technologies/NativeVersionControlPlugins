#pragma once
#include "TestCommand.h"

template<>
class TestCommand<DeleteRequest>
{
public:
	bool Run(TestTask& task, DeleteRequest& req, DeleteResponse& resp)
	{
		req.conn.Log().Notice() << "[DeleteRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
