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
			task.GetStatus(result.entries[0].assets, statAssets, false);
			SvnCommand<IncomingRequest>::RemoveUpToDateAssets(statAssets, req.revision);
			resp.assets.swap(statAssets);
		}

		resp.Write();
		return true;
	}
};
