/*
 * File system related functions
 */
#pragma once
#include "VersionedAsset.h"

const int kPathWild        = 1 << 0;
const int kPathRecursive   = 1 << 1;
const int kPathSkipFolders = 1 << 2;

// Helper to figure out state from action and revisions
int ActionToState(const std::string& action, const std::string& headAction, 
				  const std::string& haveRev, const std::string& headRev);

// Add wildcards to a path so perforce can deal with it
std::string WildcardsAdd (const std::string& path);

// Remove wildcards to a path, returning it to normal
std::string WildcardsRemove (const std::string& path);

// Construct path from asset and flags kPathWild, kPathRecursive
std::string ResolvedPath(const VersionedAsset& asset, int flags);
std::string ResolvePaths(VersionedAssetList::const_iterator b,
						 VersionedAssetList::const_iterator e,
						 int flags, const std::string& delim = "", const std::string& postfix = "");
void ResolvePaths(std::vector<std::string>& result, VersionedAssetList::const_iterator b,
						 VersionedAssetList::const_iterator e,
						 int flags, const std::string& delim = "");

std::string ResolvePaths(const VersionedAssetList& list, int flags, const std::string& delim = "",
						 const std::string& postfix = "");
void ResolvePaths(std::vector<std::string>& result, 
				  const VersionedAssetList& list, int flags, const std::string& delim = "");

// Translates a workspace absolute path to p4 depot path
std::string WorkspacePathToDepotPath(const std::string& root, const std::string& wp);

void PathToMovedPath(VersionedAssetList& l);


// For filtering assets by state
struct StateFilter
{
	StateFilter(int statesToInclude, int statesToExclude = kNone)
		: m_StatesToInclude(statesToInclude), m_StatesToExlude(statesToExclude)
	{
	}

	inline bool operator()(const VersionedAsset& a) const
	{
		return a.HasState(m_StatesToInclude) && !a.HasState(m_StatesToExlude);
	}

	int m_StatesToInclude;
	int m_StatesToExlude;
};

// Partitions an asset list into two list using a filter
void Partition(const StateFilter& filter, VersionedAssetList& l1_InOut,
	VersionedAssetList& l2_Out);

