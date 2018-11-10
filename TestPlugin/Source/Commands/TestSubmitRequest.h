#pragma once
#include "TestCommand.h"

template<>
class TestCommand<SubmitRequest>
{
public:
	bool Run(TestTask& task, SubmitRequest& req, SubmitResponse& resp)
	{
		req.conn.Log().Notice() << "[SubmitRequest]" << Endl;
		req.conn.Log().Notice() << "ChangelistRevision: " << req.changelist.GetRevision();
		req.conn.Log().Notice() << " TimeStamp: " << req.changelist.GetTimestamp();
		req.conn.Log().Notice() << " Commiter: " << req.changelist.GetCommitter() << Endl;
		req.conn.Log().Notice() << "Description:" << Endl;
		req.conn.Log().Notice() << req.changelist.GetDescription() << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};
