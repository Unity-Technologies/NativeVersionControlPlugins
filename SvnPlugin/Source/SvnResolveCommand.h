#pragma once

template<>
class SvnCommand<ResolveRequest>
{
public:
	bool Run(SvnTask& task, ResolveRequest& req, ResolveResponse& resp)
	{
		std::string cmd = "resolve --accept=";

		if (req.args.size() == 1)
			cmd += "theirs-full";
		else if (req.args[1] == "mine")
			cmd += "mine-full";
		else if (req.args[1] == "theirs")
			cmd += "theirs-full";
		else if (req.args[1] == "merged")
			cmd += "working";

		cmd += " ";
		cmd += Join(Paths(req.assets), " ", "\"");
		APOpen cppipe = task.RunCommand(cmd);

		std::string line;
		while (cppipe->ReadLine(line))
		{
			req.conn.Log() << line << "\n";
		}

		const bool recursive = false;
		task.GetStatus(req.assets, resp.assets, recursive);
			
		resp.Write();
		return true;
	}
};
