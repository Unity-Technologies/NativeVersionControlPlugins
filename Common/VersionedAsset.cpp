#include "VersionedAsset.h"
#include "UnityPipe.h"

using namespace std;

VersionedAsset::VersionedAsset() : m_State(kLocal | kReadOnly)
{ 
	SetPath(""); 
}

VersionedAsset::VersionedAsset(const std::string& path) : m_State(kReadOnly) 
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

void VersionedAsset::Reset() 
{ 
	m_State = kLocal | kReadOnly; 
	SetPath(""); 
}

bool VersionedAsset::IsFolder() const 
{
	return m_Path[m_Path.size()-1] == '/';
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
