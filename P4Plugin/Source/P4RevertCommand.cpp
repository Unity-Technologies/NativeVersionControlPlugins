#include "P4Command.h"
#include "P4Utility.h"
#include "FileSystem.h"

using namespace std;

class P4RevertCommand : public P4Command
{
public:
	P4RevertCommand() : P4Command("revert") { }
	
	bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		m_ProjectPath = task.GetP4Root();
		m_Result.clear();

		Conn().Log().Info() << args[0] << "::Run()" << Endl;
	
		string cmd = SetupCommand(args);
	
		VersionedAssetList assetList;
		Conn() >> assetList;
		string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);
	
		Conn().Log().Debug() << "Paths resolved are: " << paths << Endl;
	
		if (paths.empty())
		{
			Conn().BeginList();
			Conn().WarnLine("No paths for revert command", MARemote);
			Conn().EndList();
			Conn().EndResponse();
			return true;
		}
	
		cmd += " " + paths;
	
		task.CommandRun(cmd, this);

		if (!MapToLocal(task, m_Result))
		{
			// Abort since there was an error mapping files to depot path
			Conn().BeginList();
			Conn().WarnLine("Files couldn't be mapped in perforce view");
			Conn().EndList();
			Conn().EndResponse();
			return true;
		}

		IncludeFolders(assetList);

		// Set readonly and local flags
		for (VersionedAssetList::iterator i = m_Result.begin(); i != m_Result.end(); ++i)
		{
			if (PathExists(i->GetPath()))
			{
				i->AddState(kLocal);
				if (IsReadOnly(i->GetPath()))
					i->AddState(kReadOnly);
			}
		}

		Conn() << m_Result;
		m_Result.clear();
		Conn() << GetStatus();
		
		// The OutputState and other callbacks will now output to stdout.
		// We just wrap up the communication here.
		Conn().EndResponse();
		return true;
	}

	void IncludeFolders(const VersionedAssetList& assetList)
	{
		VersionedAssetSet assetsToRevert;
		assetsToRevert.insert(assetList.begin(), assetList.end());

		// Perforce doesn't support versioning of folders
		// but we need to tell Unity about reverted folders anyway.
		// This is simply done when the folders associated meta file is reverted.
		// It is especially needed in case of reverting renaming of folders.
		VersionedAssetList result;
		result.reserve(m_Result.size());

		for (VersionedAssetList::const_iterator i = m_Result.begin(); i != m_Result.end(); ++i)
		{
			string path = i->GetPath();
			if (EndsWith(path, ".meta"))
			{
				string folderPath = path.substr(0, path.length() - 5) + "/";
				VersionedAsset folderAsset(folderPath);
				if (assetsToRevert.find(folderAsset) != assetsToRevert.end())
				{
					folderAsset.SetState(i->GetState() & ~kMetaFile);
					result.push_back(folderAsset);
					if (i->GetState() & kDeletedLocal)
					{
						// Delete the folder from disk
						DeleteRecursive(folderAsset.GetPath());
					}
				}
			}
			result.push_back(*i);
		}

		m_Result.swap(result);
	}

	virtual string SetupCommand(const CommandArgs& args)
	{
		string mode = args.size() > 1 ? args[1] : string();
		if (mode == "unchangedOnly")
			return "revert -a ";
		else if (mode == "keepLocalModifications")
			return "revert -k ";
		else
			return "revert ";
	}

	void HandleError( Error *err )
	{
		if ( err == 0 )
			return;
		
		// The error we are looking for is severity 'error' according
		// to command line with -s options but here the error is set
		// as severity 'warning' for some reason. We play it safe and
		// check both.
		if (err->IsWarning() || err->IsError())
		{
			StrBuf buf;
			err->Fmt(&buf);
			string value(buf.Text());
			

			if (EndsWith(value, TrimEnd(" - file(s) not opened on this client.\n")))
			{
				Conn().Log().Debug() << value << Endl;
				Conn().VerboseLine(value);
				return; // ignore
			}
		}
		P4Command::HandleError(err);
	}

    // Called once per asset 
	void OutputInfo( char level, const char *data )
    {
		// The data format is:
		// //depot/...ProjectName/...#revnum action
		// where ... is an arbitrary deep path
		// to get the filesystem path we remove the append this
		// to the Root path.

		P4Command::OutputInfo(level, data);
		
		string d(data);

		// strip revision specifier "#ddd"
		string::size_type iPathEnd = d.rfind("#");
		if (iPathEnd == string::npos)
		{
			Conn().WarnLine(string("Invalid revert asset - ") + d);
			return;
		}
		
		string path = d.substr(0, iPathEnd);
		VersionedAsset a(path);

		if (EndsWith(d, ", not reverted"))
			return;

			
		bool revertAsDeleted = EndsWith(d, ", deleted");
		bool abandoned = EndsWith(d, " was add, abandoned");

		if (revertAsDeleted)
			a.AddState(kDeletedLocal);
		else if (abandoned)
		{
			; // no op
		}
		else
			a.AddState(kSynced);

		m_Result.push_back(a);
	}

private:
	string m_ProjectPath;
	VersionedAssetList m_Result;
} cReverPt;
