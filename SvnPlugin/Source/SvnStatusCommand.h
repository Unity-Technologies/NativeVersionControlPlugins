#pragma once

template<>
class SvnCommand<StatusRequest>
{
public:
	bool Run(SvnTask& task, StatusRequest& req, StatusResponse& resp)
	{
		bool recursive = req.args.size() > 1;
		task.GetStatus(req.assets, resp.assets, recursive);
		resp.Write();
		return true;
	}
};
