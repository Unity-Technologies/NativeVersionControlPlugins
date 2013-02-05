#pragma once

template<>
class SvnCommand<AddRequest>
{
public:
	bool Run(SvnTask& task, const AddRequest& req, AddResponse& resp)
	{
		std::string cmd = "add --depth=empty ";
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

