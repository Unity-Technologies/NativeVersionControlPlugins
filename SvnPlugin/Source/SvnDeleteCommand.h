#pragma once

template<>
class SvnCommand<DeleteRequest>
{
public:
	bool Run(SvnTask& task, DeleteRequest& req, DeleteResponse& resp)
	{
		std::string cmd = "delete ";
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
