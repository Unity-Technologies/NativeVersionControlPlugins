#pragma once
#include "SvnTask.h"
#include <fstream>

// Submit == Svn commit
template<>
class SvnCommand<SubmitRequest>
{
public:
	bool Run(SvnTask& task, SubmitRequest& req, SubmitResponse& resp)
	{
		bool saveOnly = req.args.size() > 1 && req.args[1] == "saveOnly";
		const bool recursive = false;

		// saveOnly option really only makes sense for perforce but is 
		// also used to move assets to a newly constructed changelist
		if (saveOnly) 
		{
			std::string createChangelistCmd = "changelist ";
			std::string firstLineOfDescription = req.changelist.GetDescription();
			firstLineOfDescription = firstLineOfDescription.substr(0, firstLineOfDescription.find('\n'));
			createChangelistCmd += req.changelist.GetRevision() == kNewListRevision ? 
				firstLineOfDescription : req.changelist.GetRevision();
			createChangelistCmd += " ";
			createChangelistCmd += Join(Paths(req.assets), " ", "\"");
			APOpen cppipe = task.RunCommand(createChangelistCmd);

			std::string line;
			while (cppipe->ReadLine(line))
			{
				req.conn.Log() << line << "\n";
			}
			
			task.GetStatus(req.assets, resp.assets, recursive);
			
			resp.Write();
			return true;
		}

		std::string tmpfilepath = task.GetProjectPath() + "/Temp/svncommitmessage.txt";
		{
			std::ofstream os(tmpfilepath.c_str());
			os << req.changelist.GetDescription();
			os.flush();
		}
		
		// Revert to base
		std::string cmd = "commit -F ";
		cmd += tmpfilepath;
		cmd += " ";
		cmd += Join(Paths(req.assets), " ", "\"");

		APOpen ppipe = task.RunCommand(cmd);

		std::string line;
		while (ppipe->ReadLine(line))
		{
			req.conn.Log() << line << "\n";
		}

		task.GetStatus(req.assets, resp.assets, recursive);

		resp.Write();
		return true;
	}
};
