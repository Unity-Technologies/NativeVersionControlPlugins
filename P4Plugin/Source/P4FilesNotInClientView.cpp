#include <cmath>
#include <ctime>
#include <map>
#include <string>
#include <Utility.h>
#include "P4FilesNotInClientView.h"

typedef std::string AssetPath;
typedef std::map<AssetPath, std::time_t> ExcludedAssetMap;
typedef ExcludedAssetMap::value_type ExcludedAssetEntry;

static const double ExcludedAssetExpiryInSeconds = 10.0;
static const std::string FilesNotInClientView = " - file(s) not in client view.\n";

static ExcludedAssetMap excludedAssets;

static const std::time_t Now()
{
	std::time_t now;
	std::time(&now);
	return now;
}

void P4FilesNotInClientView::Filter(VersionedAssetList& assetList)
{
	const std::time_t now = Now();

	for (VersionedAssetList::iterator assetItr = assetList.begin(); assetItr != assetList.end();)
	{
		const std::string& assetPath = assetItr->GetPath();

		const ExcludedAssetMap::iterator excludedItr = excludedAssets.find(assetPath);
		const bool isNotExcluded = excludedItr == excludedAssets.end();

		if (isNotExcluded)
		{
			++assetItr;
			continue;
		}

		// `assetPath` was excluded in the past,
		// check `timestamp` for expiration
		const ExcludedAssetEntry& excludedAssetEntry = *excludedItr;
		const std::time_t timestamp = excludedAssetEntry.second;

		const double elapsedSeconds = std::fabs(std::difftime(now, timestamp));
		const bool exclusionHasExpired = elapsedSeconds > ExcludedAssetExpiryInSeconds;

		if (exclusionHasExpired)
		{
			excludedAssets.erase(excludedItr);
			++assetItr;
			continue;
		}

		// `assetPath` exclusion has not expired,
		// remove it from the list of assets for which the Perforce status
		// will be queried from the server
		assetItr = assetList.erase(assetItr);
	}
}

VCSStatus& P4FilesNotInClientView::Update(VCSStatus& status)
{
	const std::time_t now = Now();

	for (VCSStatus::iterator statusItr = status.begin(); statusItr != status.end();)
	{
		const VCSStatus::iterator statusToCheck = statusItr;

		++statusItr; // increment `statusItr` before it is invalidated by `erase()`

		const size_t indexOfFilesNotInClientView = statusToCheck->message.find(FilesNotInClientView);
		const bool foundFilesNotInClientView = indexOfFilesNotInClientView != std::string::npos;

		if (foundFilesNotInClientView)
		{
			const std::string assetPath = statusToCheck->message.substr(0, indexOfFilesNotInClientView);

			const ExcludedAssetEntry excludedAssetEntry(assetPath, now);
			excludedAssets.insert(excludedAssetEntry);
			status.erase(statusToCheck);
		}
	}
	return status;
}