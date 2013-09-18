#include "P4Utility.h"
#include "Utility.h"

using namespace std;

int ActionToState(const string& action, const string& headAction,
				  const string& haveRev, const string& headRev)
{
	int state = kNone; // kLocal
	
	if (action == "add") state |= kAddedLocal;
	else if (action == "move/add") state |= kAddedLocal | kMovedLocal;
	else if (action == "edit") state |= kCheckedOutLocal;
	else if (action == "delete") state |= kDeletedLocal;
	else if (action == "move/delete") state |= kDeletedLocal | kMovedLocal;
	// else if (action == "local") state |= kLocal;
	/*
	 else if (action == "")
	 {
	 // No action means that we're not working with it locally.
	 
	 
	 return headAction == "delete" ? kLocal : kSynced;
	 }
	 */
	bool remoteUpdates = haveRev != headRev && !headRev.empty();
	
	if (remoteUpdates)
	{
		state |= kOutOfSync;
		if (headAction == "add") state |= kAddedRemote;
		else if (headAction == "move/add") state |= kAddedRemote | kMovedRemote;
		// else if (headAction == "edit") state |= kOutOfSync;
		else if (headAction == "delete")
		{
			if (haveRev.empty())
			{
				// Not in registered as in workspace and deleted remote ie. remove outofsync flag
				// This may happen deleting a file in vcs and creating a new file with the
				// same name.
				state = state & ~kOutOfSync;
			}
			else
			{
				state |= kDeletedRemote;
			}
		}
		else if (headAction == "move/delete") state |= kDeletedRemote | kMovedRemote;
		// else state |= kOutOfSync;
	}
	else if (headRev.empty())
	{
		// Mean haveRev is also empty and file not know to perforce. (might be addedLocal though)
	}
	else 
	{
		// HeadRev and HaveRev are there and equal. Things are in sync
		state |= kSynced;
	}

	return state;
}


string WildcardsAdd(const string& pathIn)
{
	// Perforce wildcards use hex values.  
	// The following characters below must be swapped for these
	string path = Replace (pathIn, "%", "%25"); // Must be 1st :)
	path = Replace (path, "#", "%23");
	path = Replace (path, "@", "%40");
	return Replace (path, "*", "%2A");
}


string WildcardsRemove (const string& pathIn)
{
	string path = Replace (pathIn, "%23", "#");
	path = Replace (path, "%40", "@");
	path = Replace (path, "%2A", "*");
	return Replace (path, "%25", "%"); // Must do this last or we could convert an actual % to another wildcard
}	


string ResolvedPath(const VersionedAsset& asset, int flags)
{
	string path = asset.GetPath();
	
	if (flags & kPathWild)
		path = WildcardsAdd(path);
	
	if (asset.IsFolder())
		path += (flags & kPathRecursive) ? "..." : "*";

	return path;
}


string ResolvePaths(VersionedAssetList::const_iterator b,
					VersionedAssetList::const_iterator e,
					int flags, const string& delim, const string& postfix)
{
	string paths;
	
	for (VersionedAssetList::const_iterator i = b; i != e; i++) 
	{
		if (!paths.empty())
			paths += delim;
		if ((flags & kPathSkipFolders) && !(flags & kPathRecursive) && i->IsFolder())
			continue;
		paths += "\"";
		paths += ResolvedPath(*i, flags);
		paths += postfix;
		paths += "\" ";
	}
	return paths;
}

void ResolvePaths(vector<string>& result, 
				  VersionedAssetList::const_iterator b,
				  VersionedAssetList::const_iterator e,
				  int flags, const string& delim)
{
	for (VersionedAssetList::const_iterator i = b; i != e; i++) 
	{
		if ((flags & kPathSkipFolders) && !(flags & kPathRecursive) && i->IsFolder())
			continue;
		result.push_back(ResolvedPath(*i, flags));
	}
}

string ResolvePaths(const VersionedAssetList& list, int flags, const string& delim, const string& postfix)
{
	return ResolvePaths(list.begin(), list.end(), flags, delim, postfix);
}

void ResolvePaths(vector<string>& result, const VersionedAssetList& list, int flags, const string& delim)
{
	ResolvePaths(result, list.begin(), list.end(), flags, delim);
}


string WorkspacePathToDepotPath(const string& root, const string& wp)
{
	return string("/") + wp.substr(root.length());
}
