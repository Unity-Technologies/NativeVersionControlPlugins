#include "AESClient.h"
#include "JSON.h"
#include "VersionedAsset.h"
#include "FileSystem.h"
#include <time.h>

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

void DirectoryToEntries(const string& path, const JSONArray& children, TreeOfEntries& entries)
{
    for (vector<JSONValue*>::const_iterator i = children.begin() ; i != children.end() ; i++)
    {
        const JSONObject& child = (*i)->AsObject();
        bool isDir = *(child.at("directory"));
        string revisionID = *(child.at("revisionID"));
        string name = *(child.at("name"));
		if (isDir)
        {
            const JSONArray subChildren = *(child.at("children"));
			string key = path + name + "/";
			
			entries.insert(key, AESEntry(revisionID, "", kAddedRemote, 0, true));
            DirectoryToEntries(key, subChildren, entries);
        }
        else
        {
			struct tm t;

			string hash = (child.find("hash") != child.end()) ? *(child.at("hash")) : "";
			int size = (int)(child.at("size")->AsNumber());
			string time = *(child.at("mtime"));
			strptime(time.c_str(), "%Y-%m-%dT%H:%M:%S%z", &t);
			time_t ts = mktime(&t);
			
			string key = path + name;
            
			entries.insert(key, AESEntry(revisionID, hash, kAddedRemote, size, false, ts));
        }
    }
}

bool AESClient::GetRevision(const string& revisionID, TreeOfEntries& entries)
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

bool AESClient::GetRevisionDelta(const string& revisionID, string compRevisionID, TreeOfEntries& entries)
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
				
				int state = kNone;
				if (status == "added") state = kAddedRemote;
				else if (status == "deleted") state = kDeletedRemote;

				entries.insert(newPath, AESEntry(revisionID, "", state));
            }
            res = true;
		}
    }
    
	if (!res)
	{
		if (json)
			SetLastMessage("GetRevisionDelta Invalid JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetRevisionDelta Not JSON: '" + response + "'");
	}
    
    if (json)
        delete json;
    return res;
}

bool AESClient::GetLatestRevision(string& revision)
{
	ClearLastMessage();
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
			SetLastMessage("GetLatestRevision Invalid JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetLatestRevision Not JSON: '" + response + "'");
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
                
				string time = *(child.at("time"));
				struct tm t;
				strptime(time.c_str(), "%Y-%m-%dT%H:%M:%S%z", &t);
				time_t ts = mktime(&t);
				
                const JSONObject& refChildren = *(child.at("children"));
                string ref = *(refChildren.at("ref"));
                
				string comment = *(child.at("comment"));
				string revisionID = *(child.at("revisionID"));
                revisions.push_back(AESRevision(name, email, comment, revisionID, ref, ts));
            }
            res = true;
        }
    }
    
	if (!res)
	{
		if (json)
			SetLastMessage("GetRevisions Invalid JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetRevisions Not JSON: '" + response + "'");
	}
    
    if (json)
        delete json;
    return res;
}

bool AESClient::Download(AESEntry* entry, const string& path, const string& target)
{
	ClearLastMessage();
	string remotePath = m_Server + m_Path + "/" + entry->GetRevisionID() + "/" + path;
	string localPath = target + "/" + path;
	string parentPath = localPath.substr(0, localPath.find_last_of('/'));
	if (!EnsureDirectory(parentPath))
	{
		SetLastMessage("Unable to create directory " + parentPath);
		return false;
	}
	
	if (entry->IsDir())
	{
		return true;
	}
	
	if (PathExists(localPath))
	{
		DeleteRecursive(localPath);
	}
	
	if (!m_CURL.Download(remotePath, localPath))
	{
		SetLastMessage("Unable to download " + remotePath + " to " + localPath + " (" + m_CURL.GetErrorMessage()+ ")");
		return false;
	}
	else
	{
		if (!TouchAFile(localPath, entry->GetTimeStamp()))
		{
			SetLastMessage("Unable to set modification date of " + localPath);
			return false;
		}
	}
	
	return true;
}

typedef struct
{
	char basePath[2048];
	map<string, string>* files;
} ConvertEntryToMapOfFilesCallBackData;

static int ConvertEntryToMapOfFilesCallBack(void *data, const std::string& key, AESEntry *entry)
{
	ConvertEntryToMapOfFilesCallBackData* d = (ConvertEntryToMapOfFilesCallBackData*)data;
	if (!entry->IsDir())
	{
		string basePath = string(d->basePath);
		string path = basePath + "/" + key;
		path.resize(path.find_last_of('/'));
		
		string dest = path.size() > 1 ? path.substr(basePath.size()+1) : "";
		d->files->insert(make_pair(basePath + "/" + key, dest));
	}
	return 0;
}

static int ConvertEntryToListOfFilesCallBack(void *data, const std::string& key, AESEntry *entry)
{
	vector<string>* l = (vector<string>*)data;
	if (!entry->IsDir())
	{
		l->push_back(key);
	}
	return 0;
}

bool AESClient::ApplyChanges(const string& basePath, TreeOfEntries& addOrUpdateEntries, TreeOfEntries& deleteEntries, const string& comment)
{
	string response = "";
    string url = m_Server + m_Path;
	
	map<string, string> addOrUpdateFiles;
	vector<string> deleteFiles;
	
	ConvertEntryToMapOfFilesCallBackData data;
	strcpy(data.basePath, basePath.c_str());
	data.files = &addOrUpdateFiles;
	
	addOrUpdateEntries.iterate(ConvertEntryToMapOfFilesCallBack, &data);
	deleteEntries.iterate(ConvertEntryToListOfFilesCallBack, &deleteFiles);
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