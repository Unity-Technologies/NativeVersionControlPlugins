#pragma once
#include "TestCommand.h"

template<>
class TestCommand<FileModeRequest>
{
public:
	bool Run(TestTask& task, FileModeRequest& req, FileModeResponse& resp)
	{
		req.conn.Log().Notice() << "[FileModeRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
