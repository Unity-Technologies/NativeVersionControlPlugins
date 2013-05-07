#pragma once

template<>
class SvnCommand<IncomingAssetsRequest>
{
public:
	bool Run(SvnTask& task, IncomingAssetsRequest& req, IncomingAssetsResponse& resp)
	{
		// Subversion does 
		SvnLogResult result;
		req.conn.Log().Info() << "Incoming assets for revision " << req.revision << unityplugin::Endl;
		task.GetLog(result, req.revision, req.revision, true);

		if (result.entries.empty())
		{
			req.conn.Pipe().WarnLine("No assets for svn revision");
		}
		else
		{
			VersionedAssetList statAssets;
			const bool recursive = false;
			const bool queryRemote = false;
			task.GetStatus(result.entries[0].assets, statAssets, recursive, queryRemote);
			SvnCommand<IncomingRequest>::RemoveUpToDateAssets(statAssets, req.revision);
			
			// The state of the assets is the one from the status call. Use the states from
			// changelog.
			VersionedAssetSet aset(result.entries[0].assets.begin(), result.entries[0].assets.end());
			for (VersionedAssetList::iterator i = statAssets.begin(); i != statAssets.end(); ++i)
			{
				VersionedAssetSet::const_iterator j = aset.find(*i);
				if (j == aset.end())
					req.conn.Log().Notice() << "Couldn't get state of file in incoming changelist. Skipping " << j->GetPath() << unityplugin::Endl;
				else
					*i = *j;
			}

			resp.assets.swap(statAssets);
		}

		resp.Write();
		return true;
	}
};
