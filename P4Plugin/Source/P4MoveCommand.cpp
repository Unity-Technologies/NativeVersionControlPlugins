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
		Pipe().Log() << args[0] << "::Run()" << unityplugin::Endl;
		
		VersionedAssetList assetList;
		Pipe() >> assetList;
		
		if ( assetList.empty() ) 
		{
			Pipe().EndResponse();
			return true;
		}
		
		// Process two assets at a time ie. src,dest
		if ( assetList.size() % 2 ) 
		{
			Pipe().ErrorLine("uneven number of assets during move", MASystem);
			Pipe().EndResponse();
			return true;
		}
		
		VersionedAssetList::const_iterator b = assetList.begin();
		VersionedAssetList::const_iterator e = b;
		
		// Split into two steps. 1st make everything editable and 2nd do the move.
		// this makes changes more atomic.
		while (b != assetList.end())
		{
			e += 2;
			const VersionedAsset& src = *b;
			
			string paths = ResolvePaths(b, e, kPathWild | kPathRecursive);
			
			Pipe().Log() << "Ensure editable source " << paths << unityplugin::Endl;
			
			string err;
			bool editable = src.GetState() & (kCheckedOutLocal | kAddedLocal | kLockedLocal);
			
			if (!editable)
			{
				string srcPath = ResolvePaths(b, b+1, kPathWild | kPathRecursive);
				Pipe().Log() << "edit " << srcPath << unityplugin::Endl;
				if (!task.CommandRun("edit " + srcPath, this))
				{
					break;
				}
			}
			b = e;
		}

		b = assetList.begin();
		e = b;

		VersionedAssetList targetAssetList;

		while (!HasErrors() && b != assetList.end())
		{
			e += 2;
			
			targetAssetList.push_back(*(b+1));
			
			string paths = ResolvePaths(b, e, kPathWild | kPathRecursive);
			
			Pipe().Log() << "move " << paths << unityplugin::Endl;
			if (!task.CommandRun("move " + paths, this))
			{
				break;
			}
			
/*
			// Make the actual file system move
			if (dest.IsFolder())
			{
				// Make sure the target folder exists and that is all since
				// sub content will be moved separately
				if (EnsureDirectory(dest.GetPath()))
				{
					errorMessage += "Error creating "; 
					errorMessage += dest.GetPath();
					break;
				}
			}
			else 
			{
				// Move the file
				if (!MoveAFile(src.GetPath(), dest.GetPath()))
				{
					errorMessage += "Error moving file ";
					errorMessage += src.GetPath();
					errorMessage += " to ";
					errorMessage += dest.GetPath();
					break;
				}
			}
*/			
			b = e;
		}
		
		// We just wrap up the communication here.
		Pipe() << GetStatus();
		
		P4Command* statusCommand = RunAndSendStatus(task, targetAssetList);
		Pipe() << statusCommand->GetStatus();
		
		Pipe().EndResponse();

		return true;
	}
	
} cMove("move");

