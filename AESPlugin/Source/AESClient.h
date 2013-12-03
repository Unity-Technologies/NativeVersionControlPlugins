#ifndef AESCLIENT_H
#define AESCLIENT_H

#include "CurlInterface.h"
#include "JSON.h"
#include "RadixTree.h"

#include <string>
#include <vector>
#include <stdlib.h>

#define AES_REVISION_SIZE	64
#define AES_HASH_SIZE		64

class AESEntry
{
public:
    AESEntry(const std::string& revisionsID, const std::string& hash, int state = 0, uint64_t size = 0, bool isDirectory = false, time_t ts = 0L)
    {
		strncpy(m_RevisionID, revisionsID.c_str(), sizeof(m_RevisionID));
		strncpy(m_Hash, hash.c_str(), sizeof(m_Hash));
		m_State = state;
		m_Size = size;
        m_IsDirectory = isDirectory;
		m_TimeStamp = ts;
    }
    
    AESEntry(const AESEntry& other)
    {
		strncpy(m_RevisionID, other.GetRevisionID().c_str(), sizeof(m_RevisionID));
		strncpy(m_Hash, other.GetHash().c_str(), sizeof(m_Hash));
		m_State = other.GetState();
		m_Size = other.GetSize();
        m_IsDirectory = other.IsDir();
		m_TimeStamp = other.GetTimeStamp();
    }
    
    const std::string GetRevisionID() const { return m_RevisionID; }
    const std::string GetHash() const { return m_Hash; }
    bool IsDir() const { return m_IsDirectory; }
    int GetState() const { return m_State; }
    void SetState(int state) { m_State = state; }
    uint64_t GetSize() const { return m_Size; }
    void SetSize(uint64_t size) { m_Size = size; }
    time_t GetTimeStamp() const { return m_TimeStamp; }
    void SetTimeStamp(time_t ts) { m_TimeStamp = ts; }
    
private:
    char m_RevisionID[AES_REVISION_SIZE];
    char m_Hash[AES_HASH_SIZE];
	int m_State;
	uint64_t m_Size;
    bool m_IsDirectory;
	time_t m_TimeStamp;
};

typedef RadixTree<AESEntry> TreeOfEntries;

class AESRevision
{
public:
    AESRevision(const std::string& comitterName, const std::string& comitterEmail, const std::string& comment, const std::string& revisionID, const std::string& reference, time_t timeStamp)
    {
        m_ComitterName = comitterName;
        m_ComitterEmail = comitterEmail;
        m_Comment = comment;
        m_RevisionID = revisionID;
        m_Reference = reference;
        m_TimeStamp = timeStamp;
    }

    const std::string GetComitterName() const { return m_ComitterName; }
    const std::string GetComitterEmail() const { return m_ComitterEmail; }
    const std::string GetComment() const { return m_Comment; }
    const std::string GetRevisionID() const { return m_RevisionID; }
    const std::string GetReference() const { return m_Reference; }
    time_t GetTimeStamp() const { return m_TimeStamp; }
    
private:
    std::string m_ComitterName;
    std::string m_ComitterEmail;
    std::string m_Comment;
    std::string m_RevisionID;
    std::string m_Reference;
    time_t m_TimeStamp;
};

class AESClient
{
public:
    AESClient(const std::string& url);
    ~AESClient();
    
    inline const std::string GetLastError() { return m_CURL.GetErrorMessage(); }
    inline const std::string GetLastResponse() { return m_CURL.GetResponse(); }
    inline const std::string GetLastMessage() { return m_lastMessage.empty() ? m_CURL.GetErrorMessage() : m_lastMessage; }
    
    bool Ping();
    bool Login(const std::string& userName, const std::string& password);
    
    bool GetLatestRevision(std::string& revision);
    bool GetRevisions(std::vector<AESRevision>& revisions);
    bool GetRevision(const std::string& revisionID, TreeOfEntries& entries);
    bool GetRevisionDelta(const std::string& revisionID, std::string compRevisionID, TreeOfEntries& entries);
    
    bool Download(AESEntry* entry, const std::string& path, const std::string& target);
	
	bool ApplyChanges(const std::string& basePath, TreeOfEntries& addOrUpdateEntries, TreeOfEntries& deleteEntries, const std::string& comment);

private:
    void ClearLastMessage() { m_lastMessage.clear(); }
    void SetLastMessage(const std::string& message) { m_lastMessage = message; }
    
    std::string m_lastMessage;
    std::string m_Server;
    std::string m_Path;
    std::string m_UserName;
    std::string m_Password;
    CurlInterface m_CURL;
};

#endif // AESCLIENT_H