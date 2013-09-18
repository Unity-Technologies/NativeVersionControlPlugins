#pragma once
#include <string>
#include <iostream>

enum State
{
	kNone = 0,
	kLocal = 1 << 0, // Asset in on local disk. If none of the sync flags below are set it means vcs doesn't know about this local file.
	kSynced = 1 << 1, // Asset is known to vcs and in sync locally
	kOutOfSync = 1 << 2, // Asset is known to vcs and outofsync locally
	kMissing = 1 << 3, 
	kCheckedOutLocal = 1 << 4,
	kCheckedOutRemote = 1 << 5,
	kDeletedLocal = 1 << 6,
	kDeletedRemote = 1 << 7,
	kAddedLocal = 1 << 8,
	kAddedRemote = 1 << 9,
	kConflicted = 1 << 10,
	kLockedLocal = 1 << 11,
	kLockedRemote = 1 << 12,
	kUpdating = 1 << 13,
	kReadOnly = 1 << 14,
	kMetaFile = 1 << 15,
	kMovedLocal = 1 << 16, // only used plugin side for perforce.
	kMovedRemote = 1 << 17, // only used plugin side for perforce.
};

class VersionedAsset
{
public:

	VersionedAsset();
	VersionedAsset(const std::string& path);
	VersionedAsset(const std::string& path, int state, const std::string& revision = "");

	int GetState() const;
	void SetState(int newState);
	void AddState(State state);
	void RemoveState(State state);
	bool HasState(State state) { return m_State & state; }

	const std::string& GetPath() const;
	void SetPath(const std::string& path);
	const std::string& GetMovedPath() const;
	void SetMovedPath(const std::string& path);
	const std::string& GetRevision() const;
	void SetRevision(const std::string& r);
	const std::string& GetChangeListID() const;
	void SetChangeListID(const std::string& r);

	void Reset();
	bool IsFolder() const;
	bool IsMeta() const { return m_State & kMetaFile; }

	bool operator<(const VersionedAsset& other) const;

private:
	int m_State;
	std::string m_Path;
	std::string m_MovedPath; // Only used for moved files. May be src or dst file depending on the kDeletedLocal/kAddedLocal flag
	std::string m_Revision;
	std::string m_ChangeListID; // Some VCS doesn't support this so it is optional
};



#include <vector>
#include <set>
typedef std::vector<VersionedAsset> VersionedAssetList;
typedef std::set<VersionedAsset> VersionedAssetSet;

std::vector<std::string> Paths(const VersionedAssetList& assets);

class Connection;
Connection& operator<<(Connection& p, const VersionedAsset& v);
Connection& operator>>(Connection& p, VersionedAsset& v);
