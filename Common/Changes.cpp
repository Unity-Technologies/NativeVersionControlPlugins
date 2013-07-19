#include "Changes.h"
#include "Connection.h"
#include <algorithm>

using namespace std;

const char * kDefaultListRevision = "-1";
const char * kNewListRevision = "-2";

Changelist::Changelist() { }
// Changelist(std::string const& description);

ChangelistRevision Changelist::GetRevision() const 
{
	return m_Revision;
}

void Changelist::SetRevision(const ChangelistRevision& revison) 
{
	m_Revision = revison;
}	

std::string Changelist::GetDescription() const 
{
	return m_Description;
}

void Changelist::SetDescription(const std::string& description) 
{
	m_Description = description;
} 

std::string Changelist::GetTimestamp() const
{
	return m_Timestamp;
}

void Changelist::SetTimestamp(const std::string& timestamp)
{
	m_Timestamp = timestamp;
}

std::string Changelist::GetCommitter() const
{
	return m_Committer;
}

void Changelist::SetCommitter(const std::string& committer)
{
	m_Committer = committer;
}

//Connection& operator<<(Connection& p, ChangelistRevision revision)
//{
//	p << revision.c_str();
//	return p;
//}

Connection& operator>>(Connection& p, ChangelistRevision& revision)
{
	string line;
	p.ReadLine(line);
	revision = line;
	return p;
}

Connection& operator<<(Connection& p, const Changelist& changelist)
{
	p.DataLine(changelist.GetRevision().c_str());
	p.DataLine(changelist.GetDescription());
	return p;
}

Connection& operator>>(Connection& p, Changelist& cl)
{
	string line;
	p.ReadLine(line);
	cl.SetRevision(line);
	p.ReadLine(line); 
	cl.SetDescription(line);
	return p;
}
