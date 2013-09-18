#include "VersionedAsset.h"
#include "Connection.h"
#include <algorithm>
#include <functional>

using namespace std;

VersionedAsset::VersionedAsset() : m_State(kNone)
{ 
	SetPath(""); 
}

VersionedAsset::VersionedAsset(const std::string& path) : m_State(kNone) 
{ 
	SetPath(path); 
}

VersionedAsset::VersionedAsset(const std::string& path, int state, const std::string& revision)
	: m_State(state), m_Revision(revision) 
{ 
	SetPath(path); 
}

int VersionedAsset::GetState() const 
{ 
	return m_State; 
}

void VersionedAsset::SetState(int newState) 
{ 
	m_State = newState; 
}

void VersionedAsset::AddState(State state) 
{ 
	m_State |= state; 
};

void VersionedAsset::RemoveState(State state) 
{ 
	m_State &= ~state; 
};

const std::string& VersionedAsset::GetPath() const 
{
	return m_Path; 
}

void VersionedAsset::SetPath(std::string const& path) 
{ 
	m_Path = path;
	if (path.length() > 5 && path.substr(path.length() - 5, 5) == ".meta") 
		AddState(kMetaFile);
}

const std::string& VersionedAsset::GetMovedPath() const 
{
	return m_MovedPath; 
}

void VersionedAsset::SetMovedPath(std::string const& path) 
{ 
	m_MovedPath = path;
}

const std::string& VersionedAsset::GetRevision() const
{
	return m_Revision;
}

void VersionedAsset::SetRevision(const std::string& r)
{
	m_Revision = r;
}

const std::string& VersionedAsset::GetChangeListID() const
{
	return m_ChangeListID;
}

void VersionedAsset::SetChangeListID(const std::string& c)
{
	m_ChangeListID = c;
}

void VersionedAsset::Reset() 
{ 
	m_State = kNone; 
	SetPath(""); 
	m_Revision.clear();
}

bool VersionedAsset::IsFolder() const 
{
	return m_Path[m_Path.size()-1] == '/';
}

bool VersionedAsset::operator<(const VersionedAsset& other) const
{
	return GetPath() < other.GetPath();
}

vector<string> Paths(const VersionedAssetList& assets)
{
	vector<string> result;
	result.reserve(assets.size());

	for (VersionedAssetList::const_iterator i = assets.begin(); i != assets.end(); ++i)
	{
		string p = i->GetPath();
		if (i->IsFolder()) 
			p.resize(p.size() - 1 ); // strip ending /
		result.push_back(p);
	}
	return result;
}

Connection& operator<<(Connection& p, const VersionedAsset& asset)
{
	p.DataLine(asset.GetPath());
	p.DataLine(asset.GetState());
	return p;
}

Connection& operator>>(Connection& p, VersionedAsset& v)
{
    string line;
	p.ReadLine(line);
	v.SetPath(line);
	p.ReadLine(line);
	int state = atoi(line.c_str());
	v.SetState(state);
	return p;
}
