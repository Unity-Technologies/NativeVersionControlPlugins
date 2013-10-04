#ifndef AESCLIENT_H
#define AESCLIENT_H

#include "CurlInterface.h"

#include <string>

class AESEntry
{
public:
    AESEntry()
    {
        Reference = "";
        Hash = "";
        IsDirectory = false;
        Size = 0;
    }

    AESEntry(const std::string& reference, const std::string& hash, bool isDirectory, int size)
    {
        Reference = reference;
        Hash = hash;
        IsDirectory = isDirectory;
        Size = size;
    }
    
    const std::string GetReference() { return Reference; }
    void SetReference(const std::string& reference) { Reference = reference; }
    
    const std::string GetHash() { return Hash; }
    void SetHash(const std::string& hash) { Hash = hash; }
    
    bool IsDir() { return IsDirectory; }
    void SetDir(bool isDirectory) { IsDirectory = isDirectory; }
    
    int GetSize() { return Size; }
    void SetSize(int size) { Size = size; }
    
private:
    std::string Reference;
    std::string Hash;
    bool IsDirectory;
    int Size;
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

private:

    std::string m_Server;
    std::string m_Path;
    std::string m_UserName;
    std::string m_Password;
    CurlInterface m_CURL;
};

#endif // AESCLIENT_H