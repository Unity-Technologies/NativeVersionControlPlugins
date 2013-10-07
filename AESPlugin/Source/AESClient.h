#ifndef AESCLIENT_H
#define AESCLIENT_H

#include "CurlInterface.h"

#include <string>
#include <vector>

class AESEntry
{
public:
    AESEntry()
    {
        m_Reference = "";
        m_Hash = "";
        m_IsDirectory = false;
        m_Size = 0;
    }

    AESEntry(const std::string& reference, const std::string& hash, bool isDirectory, int size)
    {
        m_Reference = reference;
        m_Hash = hash;
        m_IsDirectory = isDirectory;
        m_Size = size;
    }
    
    const std::string GetReference() const { return m_Reference; }
    void SetReference(const std::string& reference) { m_Reference = reference; }
    
    const std::string GetHash() const { return m_Hash; }
    void SetHash(const std::string& hash) { m_Hash = hash; }
    
    bool IsDir() const { return m_IsDirectory; }
    void SetDir(bool isDirectory) { m_IsDirectory = isDirectory; }
    
    int GetSize() const { return m_Size; }
    void SetSize(int size) { m_Size = size; }
    
private:
    std::string m_Reference;
    std::string m_Hash;
    bool m_IsDirectory;
    int m_Size;
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
    
    bool Ping();
    bool Login(const std::string& userName, const std::string& password);
    
    bool Exists(const std::string& revision, const std::string& path, AESEntry* entry = NULL);
    
    bool GetRevisions(std::vector<AESRevision>& revisions);

private:

    std::string m_Server;
    std::string m_Path;
    std::string m_UserName;
    std::string m_Password;
    CurlInterface m_CURL;
};

#endif // AESCLIENT_H