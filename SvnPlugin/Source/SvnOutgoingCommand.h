#pragma once
#include <algorithm>

template<>
class SvnCommand<OutgoingRequest>
{
public:
	bool Run(SvnTask& task, OutgoingRequest& req, OutgoingResponse& resp)
	{
		bool recursive = req.args.size() > 1;
		Changes changes;
		GetChanges(task, changes);
		resp.changeSets.swap(changes);
		resp.Write();
		return true;
	}

	static void GetChanges(SvnTask& task, Changes& changes)
	{
		changes.clear();
		VersionedAssetList empty;
		VersionedAssetList result;

		const bool recursive = true;
		const bool queryRemote = false;
		task.GetStatusWithChangelists(empty, result, recursive, queryRemote);

		std::set<std::string> names;
		for (VersionedAssetList::const_iterator i = result.begin(); i != result.end(); ++i)
			names.insert(i->GetChangeListID());

		for (std::set<std::string>::const_iterator i = names.begin(); i != names.end(); ++i)
		{
			Changelist cl;
			if (i->empty())
			{
				cl.SetDescription("default");
				cl.SetRevision(kDefaultListRevision);
			}					  
			else
			{
				cl.SetDescription(*i);
				cl.SetRevision(*i);
			}					  
			changes.push_back(cl);
		}
	}
};
