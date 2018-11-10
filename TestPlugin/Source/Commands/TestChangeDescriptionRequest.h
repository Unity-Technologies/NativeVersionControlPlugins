#pragma once
#include "TestCommand.h"

template<>
class TestCommand<ChangeDescriptionRequest>
{
public:
	bool Run(TestTask& task, ChangeDescriptionRequest& req, ChangeDescriptionResponse& resp)
	{
		req.conn.Log().Notice() << "[ChangeDescriptionRequest]" << Endl;
		req.conn.Log().Notice() << "ChangelistRevision '" << req.revision << Endl;
		resp.Write();
		return true;
	}
};