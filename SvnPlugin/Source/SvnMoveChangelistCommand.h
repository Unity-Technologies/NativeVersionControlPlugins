#pragma once
#include "SvnTask.h"
#include <fstream>

template<>
class SvnCommand<MoveChangelistRequest>
{
public:
	bool Run(SvnTask& task, MoveChangelistRequest& req, MoveChangelistResponse& resp)
	{
		// Revert to base
		std::string cmd = "changelist ";
		if (req.changelist == kDefaultListRevision)
			cmd += " --remove";
		else
			cmd += req.changelist;
		cmd += " ";
		cmd += Join(Paths(req.assets), " ", "\"");

		APOpen ppipe = task.RunCommand(cmd);

		std::string line;
		while (ppipe->ReadLine(line))
		{
			Enforce<SvnException>(!EndsWith(line, "is not a working copy"), "Project is not a subversion working copy.");
			req.conn.Log().Info() << line << "\n";
		}

		const bool recursive = false;
		task.GetStatus(req.assets, resp.assets, recursive);

		resp.Write();
		return true;
	}
};
