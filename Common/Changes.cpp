#include "Changes.h"
#include "UnityPipe.h"
#include <algorithm>

using namespace std;

const char * kDefaultListRevision = "-1";

Changelist::Changelist() { }
// Changelist(std::string const& description);

std::string Changelist::GetDescription() const 
{
	return m_Description;
}

void Changelist::SetDescription(std::string const& description) 
{
	m_Description = description;
} 

ChangelistRevision Changelist::GetRevision() const 
{
	return m_Revision;
}

void Changelist::SetRevision(const ChangelistRevision& revison) 
{
	m_Revision = revison;
}	

UnityPipe& operator<<(UnityPipe& p, ChangelistRevision revision)
{
	p << revision.c_str();
	return p;
}

UnityPipe& operator>>(UnityPipe& p, ChangelistRevision& revision)
{
	string line;
	p.ReadLine(line);
	revision = line;
	return p;
}

UnityPipe& operator<<(UnityPipe& p, const Changelist& changelist)
{
	p.OkLine(changelist.GetRevision().c_str());
	p.OkLine(changelist.GetDescription());
	return p;
}

UnityPipe& operator>>(UnityPipe& p, Changelist& cl)
{
	string line;
	p.ReadLine(line);
	cl.SetRevision(line);
	p.ReadLine(line); 
	cl.SetDescription(line);
	return p;
}
