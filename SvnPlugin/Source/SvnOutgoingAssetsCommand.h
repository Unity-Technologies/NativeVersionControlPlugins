#pragma once

template<>
class SvnCommand<OutgoingAssetsRequest>
{
public:
	bool Run(SvnTask& task, OutgoingAssetsRequest& req, OutgoingAssetsResponse& resp)
	{
		// Subversion does 
		req.conn.Log() << "Outgoing assets for list " << req.revision << unityplugin::Endl;

		std::vector<std::string> changelistAssoc;
		VersionedAssetList assets;

		// TODO: Optimise by limiting status to specified changelist only using -c
		task.GetStatusWithChangelists(assets, assets, changelistAssoc, true);
		
		bool foundList = false;
		ChangelistRevision rev = req.revision == kDefaultListRevision ? "" : req.revision;

		// Assets in the same changelist are consequtive
		size_t idx = 0;
		for (std::vector<std::string>::const_iterator it = changelistAssoc.begin(); 
			 it != changelistAssoc.end(); ++it, ++idx)
		{
			if (*it != rev)
			{
				if (foundList) break; // no more assets in list
				continue; // keep looking for the list
			}
			VersionedAsset& asset = assets[idx];
			if (asset.GetState() & 
				(kCheckedOutLocal | kDeletedLocal | kAddedLocal | kLockedLocal | kConflicted))
				resp.assets.push_back(asset);
		}
		
		resp.Write();
		return true;
	}
};
