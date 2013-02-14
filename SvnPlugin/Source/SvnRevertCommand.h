#pragma once
#include "SvnTask.h"

template<>
class SvnCommand<RevertRequest>
{
public:
	bool Run(SvnTask& task, RevertRequest& req, RevertResponse& resp)
	{
		bool unchangedOnly = req.args.size() > 1 && req.args[1] == "unchangedOnly";
		// Unchanged only doesn't make sense for svn. It does for Perforce
		// where you manually mark a file as edit mode. In subversion in contrast
		// only track if file is modified.
		if (unchangedOnly) 
		{
			resp.Write();
			return true;
		}

		// Revert to base
		std::string cmd = "revert --depth infinity ";
		cmd += Join(Paths(req.assets), " ", "\"");

		APOpen ppipe = task.RunCommand(cmd);

		std::string line;
		while (ppipe->ReadLine(line))
		{
			req.conn.Log() << line << "\n";
		}

		const bool recursive = false;
		task.GetStatus(req.assets, resp.assets, recursive);

		resp.Write();
		return true;
	}
};
