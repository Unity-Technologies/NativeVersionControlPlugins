#pragma once
#include <string>
#include <vector>
#include <iostream>

typedef std::string ChangelistRevision;
extern const char * kDefaultListRevision;
extern const char * kNewListRevision;

class Changelist
{
public:
	Changelist();
//	Changelist(std::string const& description);
	
	ChangelistRevision GetRevision() const;
	void SetRevision(ChangelistRevision const& revison);

	std::string GetDescription() const;
	void SetDescription(const std::string& description);	

	std::string GetTimestamp() const;
	void SetTimestamp(const std::string& timestamp);
	
	std::string GetCommitter() const;
	void SetCommitter(const std::string& committer);

private:
	ChangelistRevision m_Revision;
	std::string m_Description;	
	std::string m_Timestamp;
	std::string m_Committer;
};

typedef std::vector<Changelist> Changes;
typedef std::vector<ChangelistRevision> ChangelistRevisions;

struct UnityPipe;
//UnityPipe& operator<<(UnityPipe& p, ChangelistRevision revision);
UnityPipe& operator>>(UnityPipe& p, ChangelistRevision& revision);
UnityPipe& operator<<(UnityPipe& p, const Changelist& changelist);
UnityPipe& operator>>(UnityPipe& p, Changelist& changelist);
