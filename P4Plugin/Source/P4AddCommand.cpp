#include "Utility.h"
#include "P4FileSetBaseCommand.h"
#include "P4Utility.h"

class P4AddCommand : public P4Command
{
public:
	P4AddCommand(const char* name) : P4Command(name) {}
	
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		Conn().Log().Info() << args[0] << "::Run()" << Endl;

		VersionedAssetList incomingAssetList;
		Conn() >> incomingAssetList;

		if ( incomingAssetList.empty() ) 
		{
			Conn().EndResponse();
			return true;
		}

		// Perforce cannot add local deleted files and we need to revert them first.

		// Get latest state for the asset to see if any of them needs to be
		// reverted
		VersionedAssetList assetList;
		RunAndGetStatus(task, incomingAssetList, assetList);

		VersionedAssetList toAdd;
		StateFilter filterOpened(kDeletedLocal, kMovedLocal);
		Partition(filterOpened, assetList, toAdd);

		// Now assetList contains assets to be reverted.
		RevertAndCheckoutAssets(task, assetList);

		// If a move/delete is being reverted this is not allowed by perforce (only allows revert for move/add counterpart). 
		// This will later result in a error for adding a file in place of the move/deleted file.
		// We cannot do anything about this situation since reverting the move in the first place to make room for
		// the add is not what the user wants. He wants to keep the moved changes where they are. Therefore we let 
		// the error go to the user so that he can manually decide what to do. ie. actually revert or submit 
		// the move before adding new stuff on the move/delete file.

		std::string paths = ResolvePaths(toAdd, kPathRecursive);

		if (paths.empty())
		{
			RunAndSendStatus(task, incomingAssetList);
			Conn().EndResponse();
			return true;
		}

		std::string cmd = "add -f ";
		cmd += " " + paths;

		task.CommandRun(cmd, this);

		// We just wrap up the communication here.
		Conn() << GetStatus();

		RunAndSendStatus(task, incomingAssetList);

		Conn().EndResponse();

		return true;
	}

	void RevertAndCheckoutAssets(P4Task& task, const VersionedAssetList& assetList)
	{
		std::string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);

		Conn().Log().Debug() << "Paths to revert before add are: " << paths << Endl;

		if (paths.empty())
			return;

		// -k will revert only server meta data and not actual files on disk
		task.CommandRun("revert -k " + paths, this);

		// Now make sure they are editable since added files are editable and we
		// want the same state
		task.CommandRun("edit " + paths, this);
	}

	virtual void HandleError( Error *err )
	{
		if ( err == 0 )
			return;
		
		StrBuf buf;
		err->Fmt(&buf);
		
		const std::string dirNoAdd = " - directory file can't be added.";
		const std::string noDouble = " - can't add existing file";

		std::string value(buf.Text());
		value = TrimEnd(value, '\n');
		
		if (EndsWith(value, dirNoAdd) || EndsWith(value, noDouble)) return; // ignore
		
		P4Command::HandleError(err);
	}
	
	virtual int GetResolvePathFlags() const { return kPathSkipFolders; }
	
} cAdd("add");
