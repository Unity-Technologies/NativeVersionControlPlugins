#include "P4FileSetBaseCommand.h"

#include "FileSystem.h"
#include "P4Command.h"
#include "P4Task.h"
#include "P4Utility.h"

using namespace std;

class P4DeleteCommand : public P4Command
{
public:
	P4DeleteCommand(const char* name) : P4Command(name) {}
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

		// Perforce cannot delete opened files and we need to revert them first.

		// Get latest state for the asset to see if any of them needs to be
		// reverted
		VersionedAssetList assetList;
		RunAndGetStatus(task, incomingAssetList, assetList);

		VersionedAssetList toDelete;
		StateFilter filterOpened(kAddedLocal | kMovedLocal | kCheckedOutLocal);
		Partition(filterOpened, assetList, toDelete);

		// Now assetList contains assets to be reverted.
		RevertAssets(task, assetList);
		
		// Delete the just reverted files from disk since the revert -k leaves
		// the file as writable which will result in a "cannot clobber writable file" error
		// when trying to p4 delete it.
		DeleteFromFileSystem(assetList);

		// Assets that is not of added type but was open can be deleted.
		// Added type needs special handling because it may not be present
		// after we did the revert.
		StateFilter filterOpenedNotAdded(kAddedLocal | kMovedLocal);
		VersionedAssetList moreAssetsToDelete;
		Partition(filterOpenedNotAdded, assetList, moreAssetsToDelete);

		// Assets of add type are either move/add or plain add. A plain add being
		// reverted equals a delete. A move/add being revert needs the moved from to be
		// deleted here after the revert has been issued.
		StateFilter filterNewlyAdded(kAddedLocal, kMovedLocal);
		VersionedAssetList movedAssetsToDelete;
		Partition(filterNewlyAdded, assetList, movedAssetsToDelete);
		
		// Rename assets to their moved from counterparts
		PathToMovedPath(movedAssetsToDelete);
		
		// Just delete everything now
		toDelete.insert(toDelete.end(), moreAssetsToDelete.begin(), moreAssetsToDelete.end());
		toDelete.insert(toDelete.end(), movedAssetsToDelete.begin(), movedAssetsToDelete.end());

		string paths = ResolvePaths(toDelete, kPathWild | kPathRecursive);

		if (paths.empty())
		{
			DeleteFromFileSystem(incomingAssetList);
			RunAndSendStatus(task, incomingAssetList);
			Conn().EndResponse();
			return true;
		}
		
		string cmd = "delete ";
		cmd += " " + paths;
		
		task.CommandRun(cmd, this);

		// We just wrap up the communication here.
		Conn() << GetStatus();
		
		DeleteFromFileSystem(incomingAssetList);

		RunAndSendStatus(task, toDelete);
		
		Conn().EndResponse();

		return true;
	}

	void DeleteFromFileSystem( VersionedAssetList &incomingAssetList )
	{
		// Finally delete all directories since this is not done by perforce by default
		// Even moved dirs will work here because we reverted with the -k flag.
		for (VersionedAssetList::const_iterator i = incomingAssetList.begin(); 
			i != incomingAssetList.end(); ++i)
			DeleteRecursive(i->GetPath());
	}

	void RevertAssets(P4Task& task, const VersionedAssetList& assetList)
	{
		string paths = ResolvePaths(assetList, kPathWild | kPathSkipFolders);
	
		Conn().Log().Debug() << "Paths to revert before delete are: " << paths << Endl;
	
		if (paths.empty())
			return;
	
		// -k will revert only server meta data and not actual files on disk
		task.CommandRun("revert -k " + paths, this);
	}

} cDelete("delete");
