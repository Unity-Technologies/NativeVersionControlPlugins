#include "VersionedAsset.h"
#include "Changes.h"
#include "P4Command.h"
#include "P4Task.h"
#include "P4Utility.h"
#include <map>

using namespace std;

struct ConflictInfo
{
	string local;
	string base;
	string conflict;
};

const size_t kDelim1Len = 11; // length of " - merging "
const size_t kDelim2Len = 12; // length of " using base "
const size_t kDelim3Len = 6; // length of " - vs "

// Command to get the lowest base and highest merge target of all conflicts available 
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
		size_t msgLen = msg.length();
		
		if (level != 48)
			P4Command::OutputInfo(level, data);

		if (msgLen > (kDelim1Len + kDelim2Len + 12) && msg.find(" - merging ") != string::npos) // 12 being the smallest possible path repr.
			HandleMergableAsset(msg);
		else if (msgLen > (kDelim3Len + 12) && msg.find(" - vs //") != string::npos)
			HandleNonMergableAsset(msg);
		else if (EndsWith(msg, " - no file(s) to resolve."))
			; // ignore
		else
			P4Command::OutputInfo(level, data);
	}

	void HandleMergableAsset(const string& data)
	{
		// format of the string should be 
		// <localPath> - merging <conflictDepotPath> using base <baseDepotPath>
		// e.g.
		// /Users/foo/Project/foo.txt - merging //depot/Project/foo.txt#6,#7 using base //depot/foo.txt#5
		bool ok = false;
		{
		string::size_type i = data.find("//", 2);

		if (i == string::npos || i < (kDelim1Len + 2) ) // 2 being smallest possible path repr of "/x"
			goto out1;
		
		string localPath = Replace(data.substr(0, i - kDelim1Len), "\\", "/");
		
		string::size_type j = data.find("//", i+2);
		if (j == string::npos || j < (kDelim1Len + kDelim2Len + 2 + 3)) // 2 + 5 + 2 being smallest possible path repr of "/x - merging //y#1 using base "
			goto out1;
		
		string conflictPath = data.substr(i,  j - i - kDelim2Len);
		
		if (j + 5 > data.length()) // the basePath must be a least 5 chars "//x#1"
			goto out1;
		
		string basePath = data.substr(j);
		
		ConflictInfo ci = { localPath, basePath, conflictPath };
		conflicts[localPath] = ci;
		ok = true;
		}
	out1:
		if (!ok)
			P4Command::OutputInfo(48, data.c_str());
	}

	void HandleNonMergableAsset(const string& data)
	{
		// format of the string should be 
		// <localPath> - vs <conflictDepotPath>
		// e.g.
		// /Users/foo/Project/foo.txt - vs //depot/Project/foo.txt#6,#7
		bool ok = false;
		{
		string::size_type i = data.find("//", 2);
		if (i == string::npos || i < (kDelim3Len + 2) ) // 2 being smallest possible path repr of "/x"
			goto out2;
		
		string localPath = Replace(data.substr(0, i - kDelim3Len), "\\", "/");
		string conflictPath = data.substr(i);
		
		if (i + 5 > data.length()) // the basePath must be a least 5 chars "//x#1"
			goto out2;
		
		ConflictInfo ci = { localPath, string(), conflictPath };
		conflicts[localPath] = ci;
		ok = true;
		}
	out2:
		if (!ok)
			P4Command::OutputInfo(48, data.c_str());
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
		Pipe().Log().Info() << args[0] << "::Run()" << unityplugin::Endl;
		
		string baseCmd = "print -q -o ";
		string targetDir;
		Pipe().ReadLine(targetDir);
		
		vector<string> versions;
		// The wanted versions to download. e.g. you can download both head, base of a file at the same time
		Pipe() >> versions;
		
		VersionedAssetList assetList;
		Pipe() >> assetList;
		vector<string> paths;
		ResolvePaths(paths, assetList, kPathWild | kPathSkipFolders);
		
		Pipe().Log().Debug() << "Paths resolved" << unityplugin::Endl;

		Pipe().BeginList();
		
		if (paths.empty())
		{
			Pipe().EndList();
			Pipe().WarnLine("No paths in fileset perforce command", MARemote);
			Pipe().EndResponse();
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

					Pipe().Log().Info() << fileCmd << unityplugin::Endl;
					if (!task.CommandRun(fileCmd, this))
						break;
					
					VersionedAsset asset;
					asset.SetPath(tmpFile);
					Pipe() << asset;

				}
				else if (*j == "mineAndConflictingAndBase")
				{
					// make a dry run resolve with the -o flag to determine the first 
					// conflicting version and its base. The download.

					if (!hasConflictInfo)
					{
						cConflictInfo.ClearStatus();
						cConflictInfo.conflicts.clear();
						string localPaths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);
						string rcmd = "resolve -o -n " + localPaths;
						Pipe().Log().Info() << rcmd << unityplugin::Endl;
						task.CommandRun(rcmd, &cConflictInfo);
						Pipe() << cConflictInfo.GetStatus();
						
						if (cConflictInfo.HasErrors())
						{
							// Abort since there was an error fetching conflict info
							string msg = cConflictInfo.GetStatusMessage();
							if (!StartsWith(msg, "No file(s) to resolve"))
							{
								Pipe() << cConflictInfo.GetStatus();
								goto error;
							}
						}
						hasConflictInfo = true;
					}
					
					map<string,ConflictInfo>::const_iterator ci = cConflictInfo.conflicts.find(*i);
					
					// Location of "mine" version of file. In Perforce this is always
					// the original location of the file.
					Pipe() << assetList[idx];

					VersionedAsset asset;
					if (ci != cConflictInfo.conflicts.end())
					{
						string conflictFile = tmpFile + "conflicting";
						string conflictCmd = cmd + "\"" + conflictFile + "\" \"" + ci->second.conflict + "\"";
						Pipe().Log().Info() << conflictCmd << unityplugin::Endl;
						if (!task.CommandRun(conflictCmd, this))
							break;
						
						asset.SetPath(conflictFile);
						Pipe() << asset;
						
						string baseFile = "";

						// base can be empty when files are not mergeable
						if (!ci->second.base.empty())
						{
							baseFile = tmpFile + "base";
							string baseCmd = cmd + "\"" + baseFile + "\" \"" + ci->second.base + "\"";
							Pipe().Log().Info() << baseCmd << unityplugin::Endl;
							if (!task.CommandRun(baseCmd, this))
								break;
						}
						else
						{
							asset.SetState(kMissing);
						}
						asset.SetPath(baseFile);
						Pipe() << asset;						
					}
					else 
					{
						// no conflict info for this file
						asset.SetState(kMissing);
						Pipe() << asset << asset;
					}	
				}
			}
		}
	error:
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Pipe().EndList();
		Pipe() << GetStatus();
		Pipe().EndResponse();

		cConflictInfo.conflicts.clear();
		
		return true;
	}

} cDownload;
