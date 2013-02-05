#pragma once
#include <algorithm>

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
		task.GetLog(result, "BASE:HEAD");

		Changelist cl;
		for (SvnLogResult::Entries::const_iterator i = result.entries.begin();
			 i != result.entries.end(); ++i)
		{
			cl.SetRevision(i->revision);
			cl.SetDescription(i->message);
			cl.SetTimestamp(i->timestamp);
			cl.SetCommitter(i->committer);
			resp.AddChangeSet(cl);
		}

		resp.Write();
		return true;
	}
};
