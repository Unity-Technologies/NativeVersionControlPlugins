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
		std::vector<std::string> changelistAssoc;
		VersionedAssetList dummy;

		task.GetStatusWithChangelists(dummy, dummy, changelistAssoc, true);

		std::set<std::string> names(changelistAssoc.begin(), changelistAssoc.end());
		
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
