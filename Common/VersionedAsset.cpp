#include "VersionedAsset.h"
#include "UnityPipe.h"
#include <algorithm>
#include <functional>

using namespace std;

VersionedAsset::VersionedAsset() : m_State(kLocal | kReadOnly)
{ 
	SetPath(""); 
}

VersionedAsset::VersionedAsset(const std::string& path) : m_State(kReadOnly) 
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

const std::string& VersionedAsset::GetRevision() const
{
	return m_Revision;
}

void VersionedAsset::SetRevision(const std::string& r)
{
	m_Revision = r;
}

void VersionedAsset::Reset() 
{ 
	m_State = kLocal | kReadOnly; 
	SetPath(""); 
	m_Revision.clear();
}

bool VersionedAsset::IsFolder() const 
{
	return m_Path[m_Path.size()-1] == '/';
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

UnityPipe& operator<<(UnityPipe& p, const VersionedAsset& asset)
{
	p.OkLine(asset.GetPath());
	p.OkLine(asset.GetState());
	return p;
}

UnityPipe& operator>>(UnityPipe& p, VersionedAsset& v)
{
    string line;
	p.ReadLine(line);
	v.SetPath(line);
	p.ReadLine(line);
	int state = atoi(line.c_str());
	v.SetState(state);
	return p;
}
