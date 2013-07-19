#pragma once
#include "SvnTask.h"
#include "FileSystem.h"

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


		bool keepLocalModifications = req.args.size() > 1 && req.args[1] == "keepLocalModifications";
		static unsigned int uniqueDirID = 0;
		std::string tmpDir;
		std::string projectPath = task.GetProjectPath();

		// Subversion does not natively support this command so we fake it.
		// First we copy the local content to a temp place and then do the revert
		// then we copy the content back in place.
		if (keepLocalModifications)
		{
			// Copy file to temp dir including their relative dirs in the project
			do {
				tmpDir = projectPath + "/Temp/svndelete_" + ToString(uniqueDirID);
				uniqueDirID++;
			}
			while (PathExists(tmpDir));

			if (!EnsureDirectory(tmpDir))
			{
				req.conn.WarnLine(std::string("Could not create temp folder during 'revert keep modifications': ") + tmpDir);
				resp.Write();
				return true;
			}

			for (VersionedAssetList::const_iterator i = req.assets.begin(); 
				 i != req.assets.end(); ++i)
			{
				// Find relative folder path to projectPath
				std::string relPath = i->GetPath().substr(0, projectPath.length());
				std::string copyTo = tmpDir + relPath;
				req.conn.Log().Debug() << "CopyFile " << i->GetPath() << " -> " << copyTo << Endl;
				if (!CopyAFile(i->GetPath(), copyTo, true))
				{
					req.conn.WarnLine(std::string("Could not copy file info to temp folder during 'revert keep modifications'"));
					resp.Write();
					return true;
				}
			}
			
			// Do the revert and copy it back afterwards
		}

		// Revert to base
		std::string cmd = "revert --depth infinity ";
		cmd += Join(Paths(req.assets), " ", "\"");

		APOpen ppipe = task.RunCommand(cmd);

		std::string line;
		while (ppipe->ReadLine(line))
		{
			req.conn.Log().Info() << line << "\n";
		}

		// Copy stored local modifications back if needed
		if (keepLocalModifications)
		{
			for (VersionedAssetList::const_iterator i = req.assets.begin(); 
				 i != req.assets.end(); ++i)
			{
				// Find relative folder path to projectPath
				std::string relPath = i->GetPath().substr(0, projectPath.length());
				std::string copyFrom = tmpDir + relPath;
				req.conn.Log().Debug() << "CopyFile " << copyFrom << " -> " << i->GetPath() << Endl;
				if (!CopyAFile(copyFrom, i->GetPath(), true))
				{
					req.conn.WarnLine(std::string("Could not copy file info from temp folder during 'revert keep modifications'"));
					// Keep going to try to make the damage as small as possible
				}
			}
		}

		const bool recursive = false;
		task.GetStatus(req.assets, resp.assets, recursive);

		resp.Write();
		return true;
	}
};
