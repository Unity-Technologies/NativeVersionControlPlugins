#pragma once
#include <Status.h>
#include <VersionedAsset.h>

struct P4FilesNotInClientView
{

	static void Filter(VersionedAssetList& assetList);

	static VCSStatus& Update(VCSStatus& status);

};