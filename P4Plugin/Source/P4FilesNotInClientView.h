#pragma once
#include <ctime>
#include <map>
#include <string>
#include <Status.h>
#include <Utility.h>
#include <VersionedAsset.h>

struct P4FilesNotInClientView
{

	typedef std::map<std::string, std::time_t> ExcludedAssetMap;

	typedef ExcludedAssetMap::value_type ExcludedAssetEntry;

	static void Filter(VersionedAssetList& assetList)
	{
		static const double ExcludedAssetExpiryInSeconds = 1;

		ExcludedAssetMap& excludedAssets = ExcludedAssets();
		for (VersionedAssetList::iterator itr = assetList.begin(); itr != assetList.end();)
		{
			const std::string& assetPath = itr->GetPath();
			const ExcludedAssetMap::iterator excludedItr = excludedAssets.find(assetPath);
			if (excludedItr == excludedAssets.end())
			{
				// assetPath is not excluded
				++itr;
				continue;
			}

			// assetPath was excluded in the past, check timestamp for expiry
			const ExcludedAssetEntry& excludedEntry = *excludedItr;
			const std::time_t timestamp = excludedEntry.second;
			const double elapsedSeconds = std::difftime(Now(), timestamp);
			if (elapsedSeconds > ExcludedAssetExpiryInSeconds)
			{
				// assetPath exclusion has expired
				excludedAssets.erase(excludedItr);
				++itr;
				continue;
			}

			// assetPath exclusion has not expired
			itr = assetList.erase(itr);
		}
	}

	static VCSStatus& Update(VCSStatus& status)
	{
		static const std::string filesNotInClientView = " - file(s) not in client view.\n";

		ExcludedAssetMap& excludedAssets = ExcludedAssets();
		for (VCSStatus::iterator itr = status.begin(); itr != status.end(); ++itr)
		{
			if (itr->message.find(filesNotInClientView) != std::string::npos)
			{
				// message contains " - file(s) not in client view.\n"
				const std::string assetPath = Replace(itr->message, filesNotInClientView, "");

				const ExcludedAssetEntry excludedEntry(assetPath, Now());
				excludedAssets.insert(excludedEntry);
			}
		}
		return status;
	}

private:
	static ExcludedAssetMap& ExcludedAssets()
	{
		static ExcludedAssetMap excludedAssets;
		return excludedAssets;
	}

	static const std::time_t Now()
	{
		std::time_t now;
		std::time(&now);
		return now;
	}
};


