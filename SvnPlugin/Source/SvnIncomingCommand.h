#pragma once

class SvnLogResult
{
public:
	struct Entry
	{
		VersionedAssetList assets;
		std::string revision;
		std::string committer;
		std::string timestamp;
		std::string message;
	};
	typedef std::vector<Entry> Entries;
	Entries entries;

	Entry& CreateEntry() 
	{ 
		entries.push_back(Entry());
		return entries.back();
	}
};

template<>
class SvnCommand<IncomingRequest>
{
public:
	bool Run(SvnTask& task, IncomingRequest& req, IncomingResponse& resp)
	{
		// Subversion does 
		SvnLogResult result;
		task.GetLog(result, "BASE", "HEAD", true);

		Changelist cl;
		for (SvnLogResult::Entries::const_iterator i = result.entries.begin();
			 i != result.entries.end(); ++i)
		{
			
			VersionedAssetList statAssets;
			req.conn.Log().Debug() << "D rev: " << i->revision << unityplugin::Endl;
			task.GetStatus(i->assets, statAssets, false);
			for (VersionedAssetList::const_iterator j = statAssets.begin();
				 j != statAssets.end(); ++j)
			{
				req.conn.Log().Debug() << "DBG: " << j->GetPath() << " " << j->GetState() << unityplugin::Endl;
			}

			RemoveUpToDateAssets(statAssets, i->revision);

			if (statAssets.empty())
				continue; // no changed files

			cl.SetRevision(i->revision);
			cl.SetDescription(i->message);
			cl.SetTimestamp(i->timestamp);
			cl.SetCommitter(i->committer);
			resp.AddChangeSet(cl);
		}

		resp.Write();
		return true;
	}

	// Since svn allows for mixed versions a version may have no changes
	// to apply event though it has not been updated for the local repos.
	// Also just some of the changed files in a changeset may been out of date
	// This method gets the actual out of date files for a revision
	// in the local repos.
	static void RemoveUpToDateAssets(VersionedAssetList& assets, const std::string& revision)
	{
		VersionedAssetList res;
		int rev = atoi(revision.c_str());

		for (VersionedAssetList::const_iterator i = assets.begin();
			 i != assets.end(); ++i)
		{
			if ((i->GetState() & kOutOfSync) && rev > atoi(i->GetRevision().c_str()) )
			{
				res.push_back(*i);
			}
		}
		assets.swap(res);
	}
};
