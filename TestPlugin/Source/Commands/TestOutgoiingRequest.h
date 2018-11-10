#pragma once
#include "TestCommand.h"

template<>
class TestCommand<OutgoingRequest>
{
public:
	bool Run(TestTask& task, OutgoingRequest& req, OutgoingResponse& resp)
	{
		req.conn.Log().Notice() << "[OutgoingRequest]" << Endl;
		resp.Write();
		return true;
	}
};
