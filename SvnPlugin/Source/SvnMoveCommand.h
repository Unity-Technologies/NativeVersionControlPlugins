#pragma once

template<>
class SvnCommand<MoveRequest>
{
public:
	bool Run(SvnTask& task, MoveRequest& req, MoveResponse& resp)
	{
		std::string cmd = "move ";
		VersionedAssetList statAssets;


		for (VersionedAssetList::const_iterator i = req.assets.begin(); 
			 i != req.assets.end(); ++i)
		{
			VersionedAssetList::const_iterator curIter = i;

			std::string srcPath = "\"";
			srcPath += i->GetPath() + "\"";

			++i;
			if (i == req.assets.end())
			{
				req.conn.Pipe().ErrorLine(ToString("No destination path while moving source path ", srcPath));
				break;
			}
			std::string dstPath = "\"";
			dstPath += i->GetPath() + "\"";
			
			APOpen cppipe = task.RunCommand(cmd);
			
			std::string line;
			while (cppipe->ReadLine(line))
			{
				Enforce<SvnException>(!EndsWith(line, "is not a working copy"), "Project is not a subversion working copy.");
				req.conn.Log().Info() << line << "\n";
			}

			statAssets.push_back(*curIter);

		}

		const bool recursive = false;
		task.GetStatus(statAssets, resp.assets, recursive);
		resp.Write();
		return true;
	}
};
