#pragma once
#include <set>
#include <string>
#include <Status.h>
#include <Utility.h>
#include <VersionedAsset.h>

struct P4FilesNotInClientView
{

	static void Filter(VersionedAssetList& assetList)
	{
		std::set<std::string>& set = Set();
		for (VersionedAssetList::iterator itr = assetList.begin(); itr != assetList.end();)
		{
			const std::string& assetPath = itr->GetPath();
			if (set.count(assetPath))
			{
				itr = assetList.erase(itr);
			}
			else
			{
				++itr;
			}
		}
	}

	static VCSStatus& Update(VCSStatus& status)
	{
		std::set<std::string>& set = Set();
		for (VCSStatus::iterator itr = status.begin(); itr != status.end();)
		{
			if (~itr->message.find(" - file(s) not in client view."))
			{
				const std::string assetPath = Replace(itr->message, " - file(s) not in client view.\n", "");
				set.insert(assetPath);
				status.erase(itr++);
			}
			else
			{
				++itr;
			}
		}
		return status;
	}

private:
	static std::set<std::string>& Set()
	{
		static std::set<std::string> set;
		return set;
	}
};


