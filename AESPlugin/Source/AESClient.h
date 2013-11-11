#ifndef AESCLIENT_H
#define AESCLIENT_H

#include "CurlInterface.h"

#include <string>
#include <vector>

class AESEntry;
typedef std::vector<AESEntry> AESEntries;

class AESEntry
{
public:
    AESEntry()
    {
        m_Name = "";
        m_Path = "";
        m_Reference = "";
        m_Hash = "";
        m_IsDirectory = false;
        m_Size = 0;
    }

    AESEntry(const std::string& name, const std::string& path, const std::string& reference, const std::string& hash, bool isDirectory, int size)
    {
        m_Name = name;
        m_Path = path;
        m_Reference = reference;
        m_Hash = hash;
        m_IsDirectory = isDirectory;
        m_Size = size;
    }
    
    AESEntry(const std::string& name, const std::string& path, bool isDirectory = false)
    {
        m_Name = name;
        m_Path = path;
        m_Reference = "";
        m_Hash = "";
        m_IsDirectory = isDirectory;
        m_Size = -1;
	}
	
    const std::string GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }
    
    const std::string GetPath() const { return m_Path; }
    void SetPath(const std::string& path) { m_Path = path; }
    
    const std::string GetReference() const { return m_Reference; }
    void SetReference(const std::string& reference) { m_Reference = reference; }
    
    const std::string GetHash() const { return m_Hash; }
    void SetHash(const std::string& hash) { m_Hash = hash; }
    
    bool IsDir() const { return m_IsDirectory; }
    void SetDir(bool isDirectory) { m_IsDirectory = isDirectory; }
    
    int GetSize() const { return m_Size; }
    void SetSize(int size) { m_Size = size; }
    
    void AddChild(AESEntry child) { m_Children.push_back(child); }
    const AESEntries& GetChildren() const { return m_Children; }
    AESEntries& GetChildren() { return m_Children; }
    
private:
    std::string m_Name;
    std::string m_Path;
    std::string m_Reference;
    std::string m_Hash;
    bool m_IsDirectory;
    int m_Size;
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
    
    AESRevision(const std::string& comitterName, const std::string& comitterEmail, const std::string& comment, const std::string& revisionID, const std::string& reference, time_t timStamp)
    {
        m_ComitterName = comitterName;
        m_ComitterEmail = comitterEmail;
        m_Comment = comment;
        m_RevisionID = revisionID;
        m_Reference = reference;
        m_TimeStamp = timStamp;
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
    void SetTimeStamp(time_t timStamp) { m_TimeStamp = timStamp; }
    
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
    
    bool Exists(const std::string& revision, const std::string& path, AESEntry* entry = NULL);
    
    bool GetRevisions(std::vector<AESRevision>& revisions);
    bool GetRevision(std::string revisionID, AESEntries& entries);
    
    bool Download(const AESEntry& entry, std::string path);
    bool Download(const AESEntries& entries, std::string basePath);
	
	bool ApplyChanges(const AESEntries& addOrUpdateEntries, const AESEntries& deleteEntries, const std::string& comment);

private:
    void SetLastMessage(const std::string message) { m_lastMessage = message; }
    
    std::string m_lastMessage;
    std::string m_Server;
    std::string m_Path;
    std::string m_UserName;
    std::string m_Password;
    CurlInterface m_CURL;
};

#endif // AESCLIENT_H