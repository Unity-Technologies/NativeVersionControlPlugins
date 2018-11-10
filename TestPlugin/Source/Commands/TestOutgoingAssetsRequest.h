#pragma once
#include "TestCommand.h"

template<>
class TestCommand<OutgoingAssetsRequest>
{
public:
	bool Run(TestTask& task, OutgoingAssetsRequest& req, OutgoingAssetsResponse& resp)
	{
		req.conn.Log().Notice() << "[OutgoingAssetsRequest]" << Endl;
		req.conn.Log().Notice() << "ChangelistRevision '" << req.revision << Endl;
		resp.Write();
		return true;
	}
};
