#pragma once
#include "TestCommand.h"

template<>
class TestCommand<IncomingRequest>
{
public:
	bool Run(TestTask& task, IncomingRequest& req, IncomingResponse& resp)
	{
		req.conn.Log().Notice() << "[IncomingRequest]" << Endl;
		resp.Write();
		return true;
	}
};
