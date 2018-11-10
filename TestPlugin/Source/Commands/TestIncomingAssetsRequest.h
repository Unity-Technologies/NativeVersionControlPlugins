#pragma once
#include "TestCommand.h"

template<>
class TestCommand<IncomingAssetsRequest>
{
public:
	bool Run(TestTask& task, IncomingAssetsRequest& req, IncomingAssetsResponse& resp)
	{
		req.conn.Log().Notice() << "[IncomingAssetsRequest]" << Endl;
		req.conn.Log().Notice() << "ChangelistRevision '" << req.revision << Endl;
		resp.Write();
		return true;
	}
};
