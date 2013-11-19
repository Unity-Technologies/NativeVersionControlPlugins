#ifndef AESCLIENT_H
#define AESCLIENT_H

#include "CurlInterface.h"
#include "JSON.h"

#include <string>
#include <vector>

class AESEntry;
typedef std::vector<AESEntry> AESEntries;
typedef std::map<std::string, AESEntry> MapOfEntries;

class AESEntry
{
public:
    AESEntry()
    {
        m_Path = "";
		m_RevisionID = "current";
		m_Hash = "";
		m_State = 0;
		m_Size = 0;
        m_IsDirectory = false;
    }

    AESEntry(const std::string& path, const std::string& revisionsID, const std::string hash, int state = 0, int size = 0, bool isDirectory = false)
    {
        m_Path = path;
		m_RevisionID = revisionsID;
		m_Hash = hash;
		m_State = state;
		m_Size = size;
        m_IsDirectory = isDirectory;
    }
    
    const std::string GetPath() const { return m_Path; }
    void SetPath(const std::string& path) { m_Path = path; }

    const std::string GetRevisionID() const { return m_RevisionID; }
	void SetRevisionID(const std::string& revisionsID) { m_RevisionID = revisionsID; }
	
    const std::string GetHash() const { return m_Hash; }
	void SetHash(const std::string& hash) { m_Hash = hash; }
	
    bool IsDir() const { return m_IsDirectory; }
    void SetDir(bool isDirectory) { m_IsDirectory = isDirectory; }
    
    int GetState() const { return m_State; }
    void SetState(int state) { m_State = state; }
    
    int GetSize() const { return m_Size; }
    void SetSize(int size) { m_Size = size; }
    
    void AddChild(AESEntry child) { m_Children.push_back(child); }
    const AESEntries& GetChildren() const { return m_Children; }
    AESEntries& GetChildren() { return m_Children; }

private:
    std::string m_Path;
    std::string m_RevisionID;
    std::string m_Hash;
	int m_State;
	int m_Size;
    bool m_IsDirectory;
    AESEntries m_Children;
};

class AESRevision
{
public:
    AESRevision()
    {
        m_ComitterName = "";
        m_ComitterEmail = "";
        m_Comment = "";
        m_RevisionID = "";
        m_Reference = "";
        m_TimeStamp = (time_t)0;
    }
    
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
    void SetComitterName(const std::string& comitterName) { m_ComitterName = comitterName; }
    
    const std::string GetComitterEmail() const { return m_ComitterEmail; }
    void SetComitterEmail(const std::string& comitterEmail) { m_ComitterEmail = comitterEmail; }
    
    const std::string GetComment() const { return m_Comment; }
    void SetComment(const std::string& comment) { m_Comment = comment; }
    
    const std::string GetRevisionID() const { return m_RevisionID; }
    void SetRevisionID(const std::string& revisionID) { m_RevisionID= revisionID; }
    
    const std::string GetReference() const { return m_Reference; }
    void SetReference(const std::string& reference) { m_Reference = reference; }
    
    time_t GetTimeStamp() const { return m_TimeStamp; }
    void SetTimeStamp(time_t timeStamp) { m_TimeStamp = timeStamp; }
    
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
    inline const std::string GetLastMessage() { return m_lastMessage; }
    
    bool Ping();
    bool Login(const std::string& userName, const std::string& password);
    
    bool GetLatestRevision(std::string& revision);
    bool GetRevisions(std::vector<AESRevision>& revisions);
    bool GetRevision(const std::string& revisionID, AESEntries& entries);
    bool GetRevisionDelta(const std::string& revisionID, std::string compRevisionID, AESEntries& entries);
    
    bool Download(const AESEntry& entry, const std::string& path);
	
	bool ApplyChanges(const std::string& basePath, const AESEntries& addOrUpdateEntries, const AESEntries& deleteEntries, const std::string& comment);

private:
    void SetLastMessage(const std::string& message) { m_lastMessage = message; }
    
    std::string m_lastMessage;
    std::string m_Server;
    std::string m_Path;
    std::string m_UserName;
    std::string m_Password;
    CurlInterface m_CURL;
};

#endif // AESCLIENT_H