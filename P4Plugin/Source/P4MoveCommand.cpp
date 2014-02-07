#include "FileSystem.h"
#include "P4Command.h"
#include "P4Task.h"
#include "P4Utility.h"

using namespace std;

class P4MoveCommand : public P4Command
{
public:
	P4MoveCommand(const char* name) : P4Command(name) {}
	virtual bool Run(P4Task& task, const CommandArgs& args)
	{
		ClearStatus();
		Conn().Log().Info() << args[0] << "::Run()" << Endl;
		
		bool noLocalFileMove = args.size() > 1 && args[1] == "noLocalFileMove";

		VersionedAssetList assetList;
		Conn() >> assetList;
		
		if ( assetList.empty() ) 
		{
			Conn().EndResponse();
			return true;
		}
		
		// Process two assets at a time ie. src,dest
		if ( assetList.size() % 2 ) 
		{
			Conn().WarnLine("uneven number of assets during move", MASystem);
			Conn().EndResponse();
			return true;
		}

		VersionedAssetList::const_iterator b = assetList.begin();
		VersionedAssetList::const_iterator e = b;
		
		// Split into two steps. 1st make everything editable and 2nd do the move.
		// this makes changes more atomic.
		string editPaths;

		while (b != assetList.end())
		{
			e += 2;
			const VersionedAsset& src = *b;
   			bool editable = (src.GetState() & (kCheckedOutLocal | kAddedLocal | kLockedLocal)) != 0;
			
			if (editable)
			{
				Conn().Log().Debug() << "Already editable source " << src.GetPath() << Endl;
			}
			else
			{
				editPaths += " ";
				editPaths += ResolvePaths(b, b+1, kPathWild | kPathRecursive);
			}
			b = e;
		}

		if (!editPaths.empty())
		{
			task.CommandRun("edit " + editPaths, this);
		}

		b = assetList.begin();
		e = b;

		VersionedAssetList targetAssetList;
		string noLocalFileMoveFlag = noLocalFileMove ? " -k " : "";
		while (!HasErrors() && b != assetList.end())
		{
			e += 2;
			
			const VersionedAsset& src = *b;
			const VersionedAsset& dest = *(b+1);

			targetAssetList.push_back(dest);
			
			string paths = ResolvePaths(b, e, kPathWild | kPathRecursive);
			
			if (!task.CommandRun("move " + noLocalFileMoveFlag + paths, this))
			{
				break;
			}
			
			// Make the actual file system move if perforce didn't do it ie. in
			// the case of an empty folder rename or a non versioned asset/folder move/rename
			if (!PathExists(dest.GetPath()))
			{
				// Move the file
				if (!MoveAFile(src.GetPath(), dest.GetPath()))
				{
					string errorMessage = "Error moving file ";
					errorMessage += src.GetPath();
					errorMessage += " to ";
					errorMessage += dest.GetPath();
					Conn().WarnLine(errorMessage);
				}
			}

			// Delete move folder src since perforce leaves around empty folders.
			// This only works because unity will not send embedded moves.
			if (src.IsFolder() && IsDirectory(src.GetPath()))
			{
				DeleteRecursive(src.GetPath());
			}

			b = e;
		}
		
		// We just wrap up the communication here.
		Conn() << GetStatus();
		
		RunAndSendStatus(task, targetAssetList);
		
		Conn().EndResponse();

		return true;
	}

} cMove("move");

