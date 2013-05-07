#pragma once

template<>
class SvnCommand<OutgoingAssetsRequest>
{
public:
	bool Run(SvnTask& task, OutgoingAssetsRequest& req, OutgoingAssetsResponse& resp)
	{
		// Subversion does 
		req.conn.Log().Info() << "Outgoing assets for list " << req.revision << unityplugin::Endl;

		VersionedAssetList empty;
		VersionedAssetList assets;

		// TODO: Optimise by limiting status to specified changelist only using -c
		const bool recursive = true;
		const bool queryRemote = false;
		task.GetStatusWithChangelists(empty, assets, recursive, queryRemote);
		
		bool foundList = false;
		ChangelistRevision rev = req.revision == kDefaultListRevision ? "" : req.revision;

		// Assets in the same changelist are consecutive
		for (VersionedAssetList::const_iterator it = assets.begin(); 
			 it != assets.end(); ++it)
		{
			if (it->GetChangeListID() != rev)
			{
				if (foundList) break; // no more assets in list
				continue; // keep looking for the list
			}
			foundList = true;
			if (it->GetState() & 
				(kCheckedOutLocal | kDeletedLocal | kAddedLocal | kLockedLocal | kConflicted))
				resp.assets.push_back(*it);
		}
		
		resp.Write();
		return true;
	}
};
