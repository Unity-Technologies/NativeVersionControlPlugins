#pragma once
#include <string>
#include <vector>
#include <iostream>

typedef std::string ChangelistRevision;
extern const char * kDefaultListRevision;

class Changelist
{
public:
	Changelist();
//	Changelist(std::string const& description);
	
	std::string GetDescription() const;
	void SetDescription(std::string const& description);
	
	ChangelistRevision GetRevision() const;
	void SetRevision(ChangelistRevision const& revison);

private:
	ChangelistRevision m_Revision;
	std::string m_Description;	
};


typedef std::vector<Changelist> Changes;
typedef std::vector<ChangelistRevision> ChangelistRevisions;

struct UnityPipe;
UnityPipe& operator<<(UnityPipe& p, ChangelistRevision revision);
UnityPipe& operator>>(UnityPipe& p, ChangelistRevision& revision);
UnityPipe& operator<<(UnityPipe& p, const Changelist& changelist);
UnityPipe& operator>>(UnityPipe& p, Changelist& changelist);
