#pragma once
#include "FileSystem.h"
#include "Utility.h"

template<>
class SvnCommand<DownloadRequest>
{
public:
	bool Run(SvnTask& task, DownloadRequest& req, DownloadResponse& resp)
	{
		size_t idx = 0;
		for (VersionedAssetList::const_iterator i = req.assets.begin(); i != req.assets.end(); ++i, idx++)
		{
			std::string assetPath = i->GetPath();

			std::string tmpFile = req.targetDir + "/" + IntToString(idx) + "_";
			
			for (ChangelistRevisions::const_iterator j = req.revisions.begin(); j != req.revisions.end(); j++)
			{
				const std::string& revision = *j;
				try 
				{
					if (revision == kDefaultListRevision || revision == "head")
					{
						if (!EnsureDirectory(req.targetDir))
						{
							std::string msg = ToString("Could not create temp dir: ", req.targetDir);
							req.Pipe().ErrorLine(msg);
							req.assets.clear();
							resp.Write();
							return true;
						}
						std::string f = tmpFile + "_" + revision;
						Download(task, assetPath, f, "HEAD");
						VersionedAsset asset;
						asset.SetPath(f);
						resp.assets.push_back(asset);
					}
					else if (revision == "mineAndConflictingAndBase")
					{
						// We cheat a little here since svn have already stored
						// these files next to the source file. We simply ignore the
						// toPath and returns the locations reported by svn. If by change the 
						// files are not there we download the ones we can.
						
						VersionedAsset mine, conflict, base;
						GetConflictInfo(task, assetPath, mine, conflict, base);
						
						// If we cannot locate the working file we do not 
						// include this asset in the response.
						if (!EnsurePresent(task, mine))
						{
							//req.conn.Log() << access(mine.GetPath().c_str(), F_OK) << unityplugin::Endl;
							req.conn.Log() << "No 'mine' file " << mine.GetPath() << unityplugin::Endl;
							req.Pipe().ErrorLine(std::string("No 'mine' file for ") + assetPath);
							continue;
						}
						
						if (!EnsurePresent(task, conflict, assetPath))
						{
							req.conn.Log() << "No 'conflict' file " << conflict.GetPath() << unityplugin::Endl;
							req.Pipe().ErrorLine(std::string("No 'conflict' file for ") + assetPath);
							continue;
						}

						if (!EnsurePresent(task, base, assetPath))
						{
							req.conn.Log() << "No 'base' file " << base.GetPath() << unityplugin::Endl;
							req.Pipe().ErrorLine(std::string("No 'base' file for ") + assetPath);
							continue;
						}
							
						resp.assets.push_back(mine);
						resp.assets.push_back(conflict);
						resp.assets.push_back(base);
					}
				}
				catch (std::exception& e)
				{
					req.conn.Log() << e.what() << unityplugin::Endl;
					req.conn.Pipe().ErrorLine("Error downloading file through svn");
					resp.assets.clear();
					resp.Write();
					return true;
				}
			}
		}
		
		resp.Write();
		return true;
	}

	void Download(SvnTask& task, const std::string& path, const std::string& toPath, const std::string& revision = "HEAD")
	{
		APOpen ppipe = task.RunCommand(std::string("cat ") + path + "@" + revision);
		ppipe->ReadIntoFile(toPath);
	}

	void GetConflictInfo(SvnTask& task, const std::string& path, 
						 VersionedAsset& mine, VersionedAsset& conflict, VersionedAsset& base)
	{
	    APOpen ppipe = task.RunCommand(std::string("info ") + path);
		std::string line;
		while (ppipe->ReadLine(line))
		{
			Enforce<SvnException>(!EndsWith(line, "is not a working copy"), "Project is not a subversion working copy.");

			if (StartsWith(line, "Conflict Previous Base File:"))
			{
				// file format is abx.txt.r20
				std::string::size_type i = line.rfind('.');
				
				// extract e.g 20 part from abx.txt.r20
				base.SetRevision(line.substr(i+2)); 
				
				// strip field name part to get filename
				base.SetPath(task.GetAssetsPath() + "/" + line.substr(29)); 
			}
			else if (StartsWith(line, "Conflict Previous Working File:"))
			{
				// file format is abx.txt.mine
				std::string::size_type i = line.rfind('.');
				
				// strip field name part to get filename
				mine.SetPath(task.GetAssetsPath() + "/" + line.substr(32)); 
			}
			else if (StartsWith(line, "Conflict Current Base File:"))
			{
				// file format is abx.txt.r21
				std::string::size_type i = line.rfind('.');
				
				// extract e.g 21 part from abx.txt.r21
					conflict.SetRevision(line.substr(i+2)); 
					
					// strip field name part to get filename
					conflict.SetPath(task.GetAssetsPath() + "/" + line.substr(28)); 
			}
		}
	}
	
	// Ensure that a file is present by optionally downloading
	// the revision using sourceFilePath. If sourceFilePath is empty
	// no download we be attempted.
	bool EnsurePresent(SvnTask& task, const VersionedAsset& asset, const std::string& sourceFilePath = "")
	{
		if (!PathExists(asset.GetPath()))
		{
			if (!sourceFilePath.empty())
			{
				Download(task, sourceFilePath, asset.GetPath(), asset.GetRevision());
				return EnsurePresent(task, asset);
			}
			return false;
		}
		return true;
	}
};
