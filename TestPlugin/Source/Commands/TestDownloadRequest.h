#pragma once
#include "TestCommand.h"

template<>
class TestCommand<DownloadRequest>
{
public:
	bool Run(TestTask& task, DownloadRequest& req, DownloadResponse& resp)
	{
		req.conn.Log().Notice() << "[DownloadRequest]" << Endl;
		req.conn.Log().Notice() << "TargetDir '" << req.targetDir << Endl;
		for (size_t i = 0; i < req.revisions.size(); ++i)
			req.conn.Log().Notice() << "ChangelistRevision '" << req.revisions[i] << Endl;
		for (size_t i = 0; i < req.assets.size(); ++i)
			req.conn.Log().Notice() << "Asset '" << req.assets[i].GetPath() << Endl;
		resp.Write();
		return true;
	}
};