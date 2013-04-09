#pragma once

template<>
class SvnCommand<StatusRequest>
{
public:
	bool Run(SvnTask& task, StatusRequest& req, StatusResponse& resp)
	{
		bool recursive = req.args.size() > 1 && req.args[1] == "recursive";
		bool offline = req.args.size() > 2 && req.args[2] == "offline";
		task.GetStatus(req.assets, resp.assets, recursive, !offline);
		resp.Write();
		return true;
	}
};
