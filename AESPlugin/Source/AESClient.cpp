#include "AESClient.h"
#include "JSON.h"
#include "VersionedAsset.h"
#include "FileSystem.h"

using namespace std;

AESClient::AESClient(const string& url)
{
    m_CURL.ParseUrl(url, &m_Server, &m_Path);
}

AESClient::~AESClient()
{
}

bool AESClient::Ping()
{
    string response = "";
    if (!m_CURL.Get(m_Server, NULL, response))
    {
        m_UserName = "";
        m_Password = "";
        return false;
    }
    
    return !response.empty();
}

bool AESClient::Login(const string& userName, const string& password)
{
    /*
	map<string,string> headers;
	headers.insert(make_pair("Content-Type", "application/x-www-form-urlencoded"));
    
	string loginUrl = m_Server + "/login";
	string response = "";
	string postData = "username=" + userName;
    postData += "&password=" + password;
    
    if (!m_CURL.Post(loginUrl, postData, &headers, response))
    {
        m_UserName = "";
        m_Password = "";
        return false;
    }
    
    map<string,string>::const_iterator i = headers.find("Location");
    if (i == headers.end() || i->second.empty() || i->second.find("index.htm") == string::npos)
    {
        m_UserName = "";
        m_Password = "";
        return false;
    }
    */
    
    m_UserName = userName;
    m_Password = password;
    return true;
}

void DirectoryToEntries(const string& path, const JSONArray& children, AESEntries& entries)
{
    for (vector<JSONValue*>::const_iterator i = children.begin() ; i != children.end() ; i++)
    {
        const JSONObject& info = (*i)->AsObject();
        bool isDir = *(info.at("directory"));
        string revisionID = *(info.at("revisionID"));
        string name = *(info.at("name"));
        
        if (isDir)
        {
            const JSONArray subChildren = *(info.at("children"));
			entries.push_back(AESEntry(path + name + "/", revisionID, "", kAddedRemote, 0, true));
            DirectoryToEntries(path + name + "/", subChildren, entries);
        }
        else
        {
			//string storeID = *(info.at("storeID"));
			string hash = (info.find("hash") != info.end()) ? *(info.at("hash")) : "";
			int size = (int)(info.at("size")->AsNumber());
			
            entries.push_back(AESEntry(path + name, revisionID, hash, kAddedRemote, size, false));
        }
    }
}

bool AESClient::GetRevision(const string& revisionID, AESEntries& entries)
{
	ClearLastMessage();
    entries.clear();
	string response = "";
    string url = m_Server + m_Path + "/" + revisionID + "?level=-1";
	
    if (!m_CURL.GetJSON(url, response))
    {
		SetLastMessage("GetRevision failed");
        return false;
    }
    
    bool res = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL)
    {
		if (json->IsObject())
		{
			const JSONObject& info = json->AsObject();
			if (info.find("status") != info.end())
			{
				SetLastMessage(*(info.at("message")));
				res = true;
			}
		}
		else if (json->IsArray())
		{
			const JSONArray& children = json->AsArray();
            DirectoryToEntries("", children, entries);
            res = true;
        }
    }
	
	if (!res)
	{
		if (json)
			SetLastMessage("GetRevision Invalid JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetRevision Not JSON: '" + response + "'");
	}
    
    if (json)
        delete json;
    return res;
}

bool AESClient::GetRevisionDelta(const string& revisionID, string compRevisionID, AESEntries& entries)
{
	ClearLastMessage();
    entries.clear();
	string response = "";
    string url = m_Server + m_Path + "/" + revisionID + "/delta?compareTo=" + compRevisionID;
    
    if (!m_CURL.GetJSON(url, response))
    {
		SetLastMessage("GetRevisionDelta failed");
        return false;
    }
    
    bool res = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL)
    {
		if (json->IsObject())
		{
			const JSONObject& info = json->AsObject();
			if (info.find("status") != info.end())
			{
				SetLastMessage(*(info.at("message")));
				res = true;
			}
		}
		else if (json->IsArray())
		{
			const JSONArray& children = json->AsArray();
            for (vector<JSONValue*>::const_iterator i = children.begin() ; i != children.end() ; i++)
            {
				const JSONObject& child = (*i)->AsObject();
				string oldPath = *(child.at("oldPath"));
				string newPath = *(child.at("newPath"));
				string status = *(child.at("status"));
				
				int state = 0;
				if (status == "added") state = kAddedRemote;
				else if (status == "deleted") state = kDeletedRemote;

				entries.push_back(AESEntry(newPath, revisionID, "", state));
            }
            res = true;
		}
    }
    
	if (!res)
	{
		if (json)
			SetLastMessage("GetRevisionDeltaInvalid JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetRevisionDeltaNot JSON: '" + response + "'");
	}
    
    if (json)
        delete json;
    return res;
}

bool AESClient::GetLatestRevision(string& revision)
{
	ClearLastMessage();
	revision.clear();
	string response = "";
    string url = m_Server + m_Path + "/current?info";
    
    if (!m_CURL.GetJSON(url, response))
    {
		SetLastMessage("GetLatestRevision failed");
        return false;
    }
    
    bool res = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL && json->IsObject())
    {
		const JSONObject& info = json->AsObject();
		if (info.find("status") != info.end())
		{
			SetLastMessage(*(info.at("message")));
			res = true;
		}
		else if (info.find("revisionID") != info.end())
		{
			revision = info.at("revisionID")->AsString();
			res = true;
		}
	}
	
	if (!res)
	{
		if (json)
			SetLastMessage("GetLatestRevision JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetLatestRevision JSON: '" + response + "'");
	}
    
    if (json)
        delete json;
    return res;
}

bool AESClient::GetRevisions(vector<AESRevision>& revisions)
{
	ClearLastMessage();
    revisions.clear();
	string response = "";
    string url = m_Server + m_Path + "?info&level=2";
    
    if (!m_CURL.GetJSON(url, response))
    {
		SetLastMessage("GetRevisions failed");
        return false;
    }
    
    bool res = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL && json->IsObject())
    {
        const JSONObject& info = json->AsObject();
		if (info.find("status") != info.end())
		{
			SetLastMessage(*(info.at("message")));
			res = true;
		}
        else if (info.find("children") != info.end())
        {
            const JSONArray& children = *(info.at("children"));
            for (vector<JSONValue*>::const_iterator i = children.begin() ; i != children.end() ; i++)
            {
                const JSONObject& child = (*i)->AsObject();
                const JSONObject& author = *(child.at("author"));
                string name = *(author.at("name"));
                string email = *(author.at("email"));
                
                time_t tSecs = (time_t)((child.at("time"))->AsNumber());
                time_t tOffset = (time_t)((child.at("timeZone"))->AsNumber());
                tSecs += tOffset*60;

                const JSONObject& refChildren = *(child.at("children"));
                string ref = *(refChildren.at("ref"));
                
				string comment = *(child.at("comment"));
				string revisionID = *(child.at("revisionID"));
                revisions.push_back(AESRevision(name, email, comment, revisionID, ref, tSecs));
            }
            res = true;
        }
    }
    
	if (!res)
	{
		if (json)
			SetLastMessage("GetRevisions JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetRevisions JSON: '" + response + "'");
	}
    
    if (json)
        delete json;
    return res;
}

bool AESClient::Download(const AESEntry& entry, const string& path)
{
	ClearLastMessage();
	string remotePath = m_Server + m_Path + "/" + entry.GetRevisionID() + "/" + entry.GetPath();
	string localPath = path + "/" + entry.GetPath();
	string parentPath = localPath.substr(0, localPath.find_last_of('/'));
	if (!EnsureDirectory(parentPath))
	{
		SetLastMessage("Unable to create directory " + parentPath);
		return false;
	}
	
	if (entry.IsDir())
	{
		return true;
	}
	
	if (!m_CURL.Download(remotePath, localPath))
	{
		SetLastMessage("Unable to download " + remotePath + " to " + localPath + " (" + m_CURL.GetErrorMessage()+ ")");
		return false;
	}
	return true;
}

void EntriesToFiles(const string& basePath, const AESEntries& entries, map<string, string>& files)
{
	for (AESEntries::const_iterator i = entries.begin() ; i != entries.end() ; i++)
	{
		const AESEntry& entry = (*i);
		if (!entry.IsDir())
		{
			string path = basePath + "/" + entry.GetPath();
			path.resize(path.find_last_of('/'));
			files.insert(make_pair(entry.GetPath(), path));
		}
	}
}

void EntriesToFiles(const AESEntries& entries, vector<string>& files)
{
	for (AESEntries::const_iterator i = entries.begin() ; i != entries.end() ; i++)
	{
		const AESEntry& entry = (*i);
		if (!entry.IsDir())
		{
			files.push_back(entry.GetPath());
		}
	}
}

bool AESClient::ApplyChanges(const string& basePath, const AESEntries& addOrUpdateEntries, const AESEntries& deleteEntries, const string& comment)
{
	string response = "";
    string url = m_Server + m_Path;
	
	map<string, string> addOrUpdateFiles;
	vector<string> deleteFiles;
	
	EntriesToFiles(basePath, addOrUpdateEntries, addOrUpdateFiles);
	EntriesToFiles(deleteEntries, deleteFiles);
	if (!m_CURL.ApplyChanges(url, NULL, addOrUpdateFiles, deleteFiles, comment, response))
		return false;
	
	bool res = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL && json->IsObject())
    {
		string data = json->Stringify();
		res = true;
	}
	
	if (json)
        delete json;
	return res;
}