#include "VersionedAsset.h"
#include "Changes.h"
#include "P4Command.h"
#include "P4Task.h"
#include "P4Utility.h"
#include <map>

using namespace std;

#ifdef WIN32
#define snprintf _snprintf
#endif

static string IntToString (int i)
{
	char buf[255];
	snprintf (buf, sizeof(buf), "%i", i);
	return string (buf);		
}

#ifdef WIN32
#undef snprintf
#endif

struct ConflictInfo
{
	string local;
	string base;
	string conflict;
};

// Command to get the lowest base and highest mergetarget of all conflicts available 
// for the given files
static class P4ConflictInfoCommand : public P4Command
{
public:
	P4ConflictInfoCommand() : P4Command("_conflictInfo") { }
	
	bool Run(P4Task& task, const CommandArgs& args) 
	{ 
		ClearStatus();
		return true; 
	}
	
	// Default handle of perforce info callbacks. Called by the default P4Command::Message() handler.
	virtual void OutputInfo( char level, const char *data )
	{	
		// Level 48 is the correct level for view mapping lines. P4 API is really not good at providing these numbers
		string msg(data);
		bool propagate = true;
		size_t delim1Len = 11; // length of " - merging "
		size_t delim2Len = 12; // length of " using base "
		size_t msgLen = msg.length();
		
		if (msg.find(" - merging ") == string::npos)
		{
			if (EndsWith(msg, " - no file(s) to resolve."))
				return; // ok
			goto out;
		}
		
		if (level == 48 && msgLen > (delim1Len + delim2Len + 12) ) // 12 being the smallest possible path repr.
		{
			// format of the string should be 
			// <localPath> - merging <conflictDepotPath> using base <baseDepotPath>
			// e.g.
			// /Users/foo/Project/foo.txt - merging //depot/Project/foo.txt#6,#7 using base //depot/foo.txt#5
			string::size_type i = msg.find("//", 2);

			if (i == string::npos && i < (delim1Len + 2) ) // 2 being smallest possible path repr of "/x"
				goto out;
		
			string localPath = Replace(msg.substr(0, i - delim1Len), "\\", "/");
			
			string::size_type j = msg.find("//", i+2);
			if (j == string::npos && j < (delim1Len + delim2Len + 2 + 3)) // 2 + 5 + 2 being smallest possible path repr of "/x - merging //y#1 using base "
				goto out;
			
			string conflictPath = msg.substr(i,  j - i - delim2Len);

			if (j + 5 > msgLen) // the basePath must be a least 5 chars "//x#1"
				goto out;
			
			string basePath = msg.substr(j);
				
			ConflictInfo ci = { localPath, basePath, conflictPath };
			conflicts[localPath] = ci;
			propagate = false;
		}	
	out:
		if (propagate)
			P4Command::OutputInfo(level, data);
	}

	map<string,ConflictInfo> conflicts;
	
} cConflictInfo;


class P4DownloadCommand : public P4Command
{
public:
	P4DownloadCommand() : P4Command("download") { }

	bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		upipe.Log() << args[0] << "::Run()" << endl;
		
		string baseCmd = "print -q -o ";
		string targetDir;
		upipe.ReadLine(targetDir);
		
		vector<string> versions;
		// The wanted versions to download. e.g. you can download both head, base of a file at the same time
		upipe >> versions;
		
		VersionedAssetList assetList;
		upipe >> assetList;
		vector<string> paths;
		ResolvePaths(paths, assetList, kPathWild | kPathSkipFolders);
		
		upipe.Log() << "Paths resolved" << endl;

		upipe.BeginList();
		
		if (paths.empty())
		{
			upipe.EndList();
			upipe.ErrorLine("No paths in fileset perforce command", MARemote);
			upipe.EndResponse();
			return true;
		}

		bool hasConflictInfo = false;
		
		// One call per file per version
		int idx = 0;
		for (vector<string>::const_iterator i = paths.begin(); i != paths.end(); ++i, ++idx)
		{
			string cmd = baseCmd;
			string tmpFile = targetDir + "/" + IntToString(idx) + "_";

			for (vector<string>::const_iterator j = versions.begin(); j != versions.end(); ++j) 
			{
				if (*j == kDefaultListRevision || *j == "head")
				{
					// default is head
					tmpFile += "head";
					string fileCmd = cmd + "\"" + tmpFile + "\" \"" + *i + "#head\"";

					upipe.Log() << fileCmd << endl;
					if (!task.CommandRun(fileCmd, this))
						break;
					
					VersionedAsset asset;
					asset.SetPath(tmpFile);
					upipe << asset;

				}
				else if (*j == "conflictingAndBase")
				{
					// make a dry run resolve with the -o flag to determine the first 
					// conflicting version and its base. The download.

					if (!hasConflictInfo)
					{
						cConflictInfo.ClearStatus();
						cConflictInfo.conflicts.clear();
						string localPaths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);
						string rcmd = "resolve -o -n " + localPaths;
						upipe.Log() << rcmd << endl;
						task.CommandRun(rcmd, &cConflictInfo);
						upipe << cConflictInfo.GetStatus();
						
						if (cConflictInfo.HasErrors())
						{
							// Abort since there was an error fetching conflict info
							string msg = cConflictInfo.GetStatusMessage();
							if (!StartsWith(msg, "No file(s) to resolve"))
							{
								upipe << cConflictInfo.GetStatus();
								goto error;
							}
						}
						hasConflictInfo = true;
					}
					
					map<string,ConflictInfo>::const_iterator ci = cConflictInfo.conflicts.find(*i);
					
					VersionedAsset asset;
					if (ci != cConflictInfo.conflicts.end())
					{
						string conflictFile = tmpFile + "conflicting";
						string conflictCmd = cmd + "\"" + conflictFile + "\" \"" + ci->second.conflict + "\"";
						upipe.Log() << conflictCmd << endl;
						if (!task.CommandRun(conflictCmd, this))
							break;
						asset.SetPath(conflictFile);
						upipe << asset;
						
						string baseFile = tmpFile + "base";
						string baseCmd = cmd + "\"" + baseFile + "\" \"" + ci->second.base + "\"";
						upipe.Log() << baseCmd << endl;
						if (!task.CommandRun(baseCmd, this))
							break;
						asset.SetPath(baseFile);
						upipe << asset;						
					}
					else 
					{
						// no conflict info for this file
						upipe << asset << asset;
					}
				}
			}
		}
	error:
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		upipe.EndList();
		upipe << GetStatus();
		upipe.EndResponse();

		cConflictInfo.conflicts.clear();
		
		return true;
	}

} cDownload;
