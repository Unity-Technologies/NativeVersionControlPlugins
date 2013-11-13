#include "AESClient.h"
#include "JSON.h"

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
        bool isDir = *(info.find("directory")->second);
        string ref = *(info.find("url")->second);
        string name = *(info.find("name")->second);
        ref.resize(ref.find("?"));
        
        if (isDir)
        {
            const JSONArray subChildren = *(info.find("children")->second);
            entries.push_back(AESEntry(name, path + name + "/", ref + "/", "", true, 0));
            DirectoryToEntries(path + name + "/", subChildren, entries.back().GetChildren());
        }
        else
        {
			string hash = *(info.find("hash")->second);
			double size = *(info.find("size")->second);
            entries.push_back(AESEntry(name, path + name, ref, hash, false, (int)size));
        }
    }
}

bool AESClient::GetRevision(string revisionID, AESEntries& entries)
{
    entries.clear();
	string response = "";
    string url = m_Server + m_Path + "/" + revisionID + "?info&level=-1";
	
    if (!m_CURL.GetJSON(url, response))
    {
		SetLastMessage("GetRevision failed");
        return false;
    }
    
    bool res = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL && json->IsObject())
    {
        const JSONObject& info = json->AsObject();
        if (info.find("status") != info.end())
        {
            SetLastMessage(*(info.find("message")->second));
            res = true;
        }
        else if (info.find("children") != info.end())
        {
            const JSONArray& children = *(info.find("children")->second);
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

bool AESClient::GetRevisionDelta(std::string revisionID, std::string compRevisionID, AESEntries& entries)
{
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
				SetLastMessage(*(info.find("message")->second));
				res = true;
			}
		}
		else if (json->IsArray())
		{
			const JSONArray& children = json->AsArray();
            for (vector<JSONValue*>::const_iterator i = children.begin() ; i != children.end() ; i++)
            {
				const JSONObject& child = (*i)->AsObject();
				string oldPath = *(child.find("oldPath")->second);
				string newPath = *(child.find("newPath")->second);
				string status = *(child.find("status")->second);
				
				int size = 0;
				if (status == "added") size = 1;
				else if (status == "deleted") size = -1;

				string name = newPath;
				size_t pos = name.find_last_of('/');
				if (pos != string::npos)
				{
					name = name.substr(pos+1);
				}
				string ref = m_Server + m_Path + "/" + revisionID + "/" + newPath;
				entries.push_back(AESEntry(name, newPath, ref, "", false, size));
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

bool AESClient::GetRevisions(vector<AESRevision>& revisions)
{
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
			SetLastMessage(*(info.find("message")->second));
			res = true;
		}
        else if (info.find("children") != info.end())
        {
            const JSONArray& children = *(info.find("children")->second);
            for (vector<JSONValue*>::const_iterator i = children.begin() ; i != children.end() ; i++)
            {
                const JSONObject& child = (*i)->AsObject();
                /*
                const JSONObject& author = *(child.find("author")->second);
                string name = *(author.find("name")->second);
                string email = *(author.find("email")->second);
                
                time_t tSecs = (time_t)((double)*(child.find("time")->second));
                time_t tOffset = (time_t)((double)*(child.find("timeZone")->second));
                tSecs += tOffset*60;

                const JSONObject& refChildren = *(child.find("children")->second);
                string ref = *(refChildren.find("ref")->second);
                
				string comment = *(child.find("comment")->second);
				string revisionID = *(child.find("revisionID")->second);
                revisions.push_back(AESRevision(name, email, comment, revisionID, ref, tSecs));
				 */
				string comment = *(child.find("comment")->second);
				string revisionID = *(child.find("revisionID")->second);
                revisions.push_back(AESRevision("me", "me@me.om", comment, revisionID, "", (time_t)0));
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

bool AESClient::Download(const AESEntry& entry, string path)
{
    SetLastMessage("Downloading " + entry.GetReference() + " to " + path);
    return true;
}

bool AESClient::Download(const AESEntries& entries, string basePath)
{
    string lastMsg = GetLastMessage();
    bool res = true;
    for (AESEntries::const_iterator i = entries.begin() ; i != entries.end() && res; i++)
    {
        const AESEntry& entry = (*i);
        if (entry.IsDir())
        {
            string path = basePath + entry.GetName() + "/";
            res = Download(entry.GetChildren(), path);
        }
        else
        {
            res = Download(entry, basePath);
        }
        SetLastMessage(lastMsg + "\n" + GetLastMessage());
    }
    return res;
}

void EntriesToFiles(const AESEntries& entries, map<string, string>& files)
{
	for (AESEntries::const_iterator i = entries.begin() ; i != entries.end() ; i++)
	{
		const AESEntry& entry = (*i);
		if (entry.IsDir())
		{
			EntriesToFiles(entry.GetChildren(), files);
		}
		else
		{
			string path = entry.GetName();
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
		if (entry.IsDir())
		{
			EntriesToFiles(entry.GetChildren(), files);
		}
		else
		{
			files.push_back(entry.GetName());
		}
	}
}

bool AESClient::ApplyChanges(const AESEntries& addOrUpdateEntries, const AESEntries& deleteEntries, const std::string& comment)
{
	string response = "";
    string url = m_Server + m_Path;
	
	map<string, string> addOrUpdateFiles;
	vector<string> deleteFiles;
	
	EntriesToFiles(addOrUpdateEntries, addOrUpdateFiles);
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