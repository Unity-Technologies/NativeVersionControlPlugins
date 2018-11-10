#pragma once
#include "TestCommand.h"

template<>
class TestCommand<MoveChangelistRequest>
{
public:
	bool Run(TestTask& task, MoveChangelistRequest& req, MoveChangelistResponse& resp)
	{
		req.conn.Log().Notice() << "[MoveChangelistRequest]" << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		req.conn.Log().Notice() << "ChangelistRevision '" << req.changelist << Endl;
		resp.Write();
		return true;
	}
};
