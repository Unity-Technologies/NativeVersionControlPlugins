#include "AESClient.h"
#include "JSON.h"
#include "VersionedAsset.h"
#include "FileSystem.h"
#include "Utility.h"
#include <time.h>

#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

const char * strp_weekdays[] = { "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
const char * strp_monthnames[] = { "january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december"};
bool strp_atoi(const char * & s, int & result, int low, int high, int offset)
{
    bool worked = false;
    char * end;
    unsigned long num = strtoul(s, & end, 10);
    if (num >= (unsigned long)low && num <= (unsigned long)high)
    {
        result = (int)(num + offset);
        s = end;
        worked = true;
    }
    return worked;
}

char * strptime(const char *s, const char *format, struct tm *tm)
{
    bool working = true;
    while (working && *format && *s)
    {
        switch (*format)
        {
        case '%':
        {
            ++format;
            switch (*format)
            {
            case 'a':
            case 'A': // weekday name
                tm->tm_wday = -1;
                working = false;
                for (size_t i = 0; i < 7; ++ i)
                {
                    size_t len = strlen(strp_weekdays[i]);
                    if (!strnicmp(strp_weekdays[i], s, len))
                    {
                        tm->tm_wday = i;
                        s += len;
                        working = true;
                        break;
                    }
                    else if (!strnicmp(strp_weekdays[i], s, 3))
                    {
                        tm->tm_wday = i;
                        s += 3;
                        working = true;
                        break;
                    }
                }
                break;
            case 'b':
            case 'B':
            case 'h': // month name
                tm->tm_mon = -1;
                working = false;
                for (size_t i = 0; i < 12; ++ i)
                {
                    size_t len = strlen(strp_monthnames[i]);
                    if (!strnicmp(strp_monthnames[i], s, len))
                    {
                        tm->tm_mon = i;
                        s += len;
                        working = true;
                        break;
                    }
                    else if (!strnicmp(strp_monthnames[i], s, 3))
                    {
                        tm->tm_mon = i;
                        s += 3;
                        working = true;
                        break;
                    }
                }
                break;
            case 'd':
            case 'e': // day of month number
                working = strp_atoi(s, tm->tm_mday, 1, 31, -1);
                break;
            case 'D': // %m/%d/%y
            {
                const char * s_save = s;
                working = strp_atoi(s, tm->tm_mon, 1, 12, -1);
                if (working && *s == '/')
                {
                    ++ s;
                    working = strp_atoi(s, tm->tm_mday, 1, 31, -1);
                    if (working && *s == '/')
                    {
                        ++ s;
                        working = strp_atoi(s, tm->tm_year, 0, 99, 0);
                        if (working && tm->tm_year < 69)
                            tm->tm_year += 100;
                    }
                }
                if (!working)
                    s = s_save;
            }
                break;
            case 'H': // hour
                working = strp_atoi(s, tm->tm_hour, 0, 23, 0);
                break;
            case 'I': // hour 12-hour clock
                working = strp_atoi(s, tm->tm_hour, 1, 12, 0);
                break;
            case 'j': // day number of year
                working = strp_atoi(s, tm->tm_yday, 1, 366, -1);
                break;
            case 'm': // month number
                working = strp_atoi(s, tm->tm_mon, 1, 12, -1);
                break;
            case 'M': // minute
                working = strp_atoi(s, tm->tm_min, 0, 59, 0);
                break;
            case 'n': // arbitrary whitespace
            case 't':
                while (isspace((int)*s)) 
                    ++s;
                break;
            case 'p': // am / pm
                if (!strnicmp(s, "am", 2))
                { // the hour will be 1 -> 12 maps to 12 am, 1 am .. 11 am, 12 noon 12 pm .. 11 pm
                    if (tm->tm_hour == 12) // 12 am == 00 hours
                        tm->tm_hour = 0;
                }
                else if (!strnicmp(s, "pm", 2))
                {
                    if (tm->tm_hour < 12) // 12 pm == 12 hours
                        tm->tm_hour += 12; // 1 pm -> 13 hours, 11 pm -> 23 hours
                }
                else
                    working = false;
                break;
            case 'r': // 12 hour clock %I:%M:%S %p
            {
                const char * s_save = s;
                working = strp_atoi(s, tm->tm_hour, 1, 12, 0);
                if (working && *s == ':')
                {
                    ++ s;
                    working = strp_atoi(s, tm->tm_min, 0, 59, 0);
                    if (working && *s == ':')
                    {
                        ++ s;
                        working = strp_atoi(s, tm->tm_sec, 0, 60, 0);
                        if (working && isspace((int)*s))
                        {
                            ++ s;
                            while (isspace((int)*s)) 
                                ++s;
                            if (!strnicmp(s, "am", 2))
                            { // the hour will be 1 -> 12 maps to 12 am, 1 am .. 11 am, 12 noon 12 pm .. 11 pm
                                if (tm->tm_hour == 12) // 12 am == 00 hours
                                    tm->tm_hour = 0;
                            }
                            else if (!strnicmp(s, "pm", 2))
                            {
                                if (tm->tm_hour < 12) // 12 pm == 12 hours
                                    tm->tm_hour += 12; // 1 pm -> 13 hours, 11 pm -> 23 hours
                            }
                            else
                                working = false;
                        }
                    }
                }
                if (!working)
                    s = s_save;
            }
                break;
            case 'R': // %H:%M
            {
                const char * s_save = s;
                working = strp_atoi(s, tm->tm_hour, 0, 23, 0);
                if (working && *s == ':')
                {
                    ++ s;
                    working = strp_atoi(s, tm->tm_min, 0, 59, 0);
                }
                if (!working)
                    s = s_save;
            }
                break;
            case 'S': // seconds
                working = strp_atoi(s, tm->tm_sec, 0, 60, 0);
                break;
            case 'T': // %H:%M:%S
            {
                const char * s_save = s;
                working = strp_atoi(s, tm->tm_hour, 0, 23, 0);
                if (working && *s == ':')
                {
                    ++ s;
                    working = strp_atoi(s, tm->tm_min, 0, 59, 0);
                    if (working && *s == ':')
                    {
                        ++ s;
                        working = strp_atoi(s, tm->tm_sec, 0, 60, 0);
                    }
                }
                if (!working)
                    s = s_save;
            }
                break;
            case 'w': // weekday number 0->6 sunday->saturday
                working = strp_atoi(s, tm->tm_wday, 0, 6, 0);
                break;
            case 'Y': // year
                working = strp_atoi(s, tm->tm_year, 1900, 65535, -1900);
                break;
            case 'y': // 2-digit year
                working = strp_atoi(s, tm->tm_year, 0, 99, 0);
                if (working && tm->tm_year < 69)
                    tm->tm_year += 100;
                break;
            case '%': // escaped
                if (*s != '%')
                    working = false;
                ++s;
                break;
            default:
                working = false;
            }
        }
            break;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v':
            // zero or more whitespaces:
            while (isspace((int)*s))
                ++ s;
            break;
        default:
            // match character
            if (*s != *format)
                working = false;
            else
                ++s;
            break;
        }
        ++format;
    }
    return (working?(char *)s:0);
}
#endif

using namespace std;

time_t strtotime_t(const string& str)
{
	struct tm t;
	size_t tzPos = str.rfind("+");
	if (tzPos == string::npos)
		tzPos = str.rfind("-");

	time_t tzOffset = (time_t)0;
	if (tzPos == string::npos)
		strptime(str.c_str(), "%Y-%m-%dT%H:%M:%S", &t);
	else
	{
		string s = str.substr(0, tzPos);
		strptime(s.c_str(), "%Y-%m-%dT%H:%M:%S", &t);

		string tzStr = str.substr(tzPos);
		int h = 0;
		int m = 0;
		sscanf(tzStr.c_str(), "%d:%d", &h, &m);
		tzOffset = (time_t)(h*3600+m*60);
	}

	time_t ts = mktime(&t);
	ts -= tzOffset;
	return ts;
}

const string AESEntry::GetStateAsString() const
{
	string res = "";
	if ((m_State & kLocal) == kLocal)
		res.append("kLocal ");
	if ((m_State & kSynced) == kSynced)
		res.append("kSynced ");
	if ((m_State & kOutOfSync) == kOutOfSync)
		res.append("kOutOfSync ");
	if ((m_State & kMissing) == kMissing)
		res.append("kMissing ");
	if ((m_State & kCheckedOutLocal) == kCheckedOutLocal)
		res.append("kCheckedOutLocal ");
	if ((m_State & kCheckedOutRemote) == kCheckedOutRemote)
		res.append("kCheckedOutRemote ");
	if ((m_State & kDeletedLocal) == kDeletedLocal)
		res.append("kDeletedLocal ");
	if ((m_State & kDeletedRemote) == kDeletedRemote)
		res.append("kDeletedRemote ");
	if ((m_State & kAddedLocal) == kAddedLocal)
		res.append("kAddedLocal ");
	if ((m_State & kAddedRemote) == kAddedRemote)
		res.append("kAddedRemote ");
	if ((m_State & kConflicted) == kConflicted)
		res.append("kConflicted ");
	if ((m_State & kLockedLocal) == kLockedLocal)
		res.append("kLockedLocal ");
	if ((m_State & kLockedRemote) == kLockedRemote)
		res.append("kLockedRemote ");
	if ((m_State & kUpdating) == kUpdating)
		res.append("kUpdating ");
	if ((m_State & kReadOnly) == kReadOnly)
		res.append("kReadOnly ");
	if ((m_State & kMetaFile) == kMetaFile)
		res.append("kMetaFile ");
	if ((m_State & kMovedLocal) == kMovedLocal)
		res.append("kMovedLocal ");
	if ((m_State & kMovedRemote) == kMovedRemote)
		res.append("kMovedRemote ");
	return res;
}

AESClient::AESClient(const string& url)
{
    m_CURL.ParseUrl(url, &m_Server, &m_Path);
}

AESClient::~AESClient()
{
}

const string AESClient::GetLastMessage()
{
	if (!m_lastMessage.empty())
		return m_lastMessage;

	if (!m_CURL.GetErrorMessage().empty())
		return m_CURL.GetErrorMessage();

	return "HTTP Error " + IntToString(m_CURL.GetResponseCode());
}

bool AESClient::Ping()
{
	ClearLastMessage();
	string response = "";
    string url = m_Server + m_Path + "/current?info";

    if (!m_CURL.GetJSON(url, response))
    {
		if (!response.empty())
		{
			SetLastMessage(response);
		}
        return false;
    }

    bool res = false;
    bool isValid = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL && json->IsObject())
    {
		const JSONObject& info = json->AsObject();
		if (info.find("status") != info.end())
		{
			SetLastMessage(*(info.at("message")));
			isValid = true;
		}
		else if (info.find("revisionID") != info.end())
		{
			isValid = true;
			res = true;
		}
	}

	if (!isValid)
		SetLastMessage("Invalid JSON");

    if (json)
        delete json;
	
    return res;
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

bool AESClient::GetAvailableRepositories(vector<string>& repositories)
{
	ClearLastMessage();
    repositories.clear();
	string response = "";
    string url = m_Server + m_Path + "/files?level=1";

    if (!m_CURL.GetJSON(url, response))
    {
		SetLastMessage("GetAvailableRepositories failed for URL " + url);
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
				string name = *(child.at("name"));
				repositories.push_back(name);
			}
            res = true;
        }
    }

	if (!res)
	{
		if (json)
			SetLastMessage("GetAvailableRepositories Invalid JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("GetAvailableRepositories Not JSON: '" + response + "'");
	}

    if (json)
        delete json;
    return res;
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
			string hash = (child.find("hash") != child.end()) ? *(child.at("hash")) : "";
			int size = (int)(child.at("size")->AsNumber());
			string time = *(child.at("mtime"));
			time_t ts = strtotime_t(time);
			
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

bool AESClient::GetRevision(const string& revisionID, AESRevision& revision)
{
	ClearLastMessage();
	string response = "";
    string url = m_Server + m_Path + "/" + revisionID + "?info";

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
			SetLastMessage(*(info.at("message")));
			res = true;
		}
		else if (info.find("revInfo") != info.end())
		{
			const JSONObject& revInfo = *(info.at("revInfo"));
			const JSONObject& author = *(revInfo.at("author"));
			string name = *(author.at("name"));
			string email = *(author.at("email"));

			string time = *(revInfo.at("time"));
			time_t ts = strtotime_t(time);

			string comment = *(revInfo.at("comment"));
			string revisionID = *(revInfo.at("id"));

			revision.SetComitterName(name);
			revision.SetComitterEmail(email);
			revision.SetTimeStamp(ts);
			revision.SetComment(comment);
			revision.SetRevisionID(revisionID);

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

bool AESClient::GetLatestRevision(AESRevision& revision)
{
	return GetRevision("current", revision);
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
				time_t ts = strtotime_t(time);

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
	string parentPath = target.substr(0, target.find_last_of("/"));
	if (!EnsureDirectory(parentPath))
	{
		SetLastMessage("Unable to create directory " + parentPath);
		return false;
	}
	
	if (entry->IsDir())
	{
		return true;
	}
	
	if (PathExists(target))
	{
		DeleteRecursive(target);
	}
	
	if (!m_CURL.Download(remotePath, target))
	{
		SetLastMessage("Unable to download " + remotePath + " to " + target + " (" + m_CURL.GetErrorMessage()+ ")");
		return false;
	}
	else
	{
		if (!TouchAFile(target, entry->GetTimeStamp()))
		{
			SetLastMessage("Unable to set modification date of " + target);
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

static int ConvertEntryToMapOfFilesCallBack(void *data, const string& key, AESEntry *entry)
{
	ConvertEntryToMapOfFilesCallBackData* d = (ConvertEntryToMapOfFilesCallBackData*)data;
	if (!entry->IsDir())
	{
		string basePath = string(d->basePath);
		string path = basePath + "/" + key;
		path.resize(path.find_last_of("/"));
		
		string dest = path.size() > basePath.size() ? path.substr(basePath.size()+1) : "";
		d->files->insert(make_pair(basePath + "/" + key, dest));
	}
	return 0;
}

static int ConvertEntryToListOfFilesCallBack(void *data, const string& key, AESEntry *entry)
{
	vector<string>* l = (vector<string>*)data;
	if (!entry->IsDir())
	{
		l->push_back(key);
	}
	return 0;
}

bool AESClient::ApplyChanges(const string& basePath, TreeOfEntries& addOrUpdateEntries, TreeOfEntries& deleteEntries, const string& comment, int* succedeedEntries)
{
	string response = "";
    string url = m_Server + m_Path;
	if (succedeedEntries) *succedeedEntries = 0;
	
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
        const JSONObject& info = json->AsObject();
		if (info.find("status") != info.end())
		{
			string status = *(info.at("status"));
			if (status == "ok" && info.find("files") != info.end())
			{
				const JSONArray& files = *(info.at("files"));
				for (vector<JSONValue*>::const_iterator i = files.begin() ; i != files.end() ; i++)
				{
					const JSONObject& file = (*i)->AsObject();
					
					bool deleted = file.at("delete")->AsBool();
					string path = *(file.at("path"));
					
					if (!deleted)
					{
						string name = *(file.at("name"));
						path += "/" + name;
					}
					
					if (succedeedEntries) *succedeedEntries = *succedeedEntries + 1;
				}
			}
			res = true;
		}
	}
	
	if (json)
        delete json;
	return res;
}

bool AESClient::CreateRepository(const string& name, const string& type)
{
	string response = "";
    string url = m_Server + m_Path + "/files";

	string data = "name=";
	data.append(name);
	data.append("&type=");
	data.append(type);

	if (!m_CURL.Post(url, data, NULL, response))
	{
		SetLastMessage("CreateRepository failed");
        return false;
    }

	bool success = false;
    bool res = false;
    JSONValue* json = JSON::Parse(response.c_str());
    if (json != NULL && json->IsObject())
    {
        const JSONObject& info = json->AsObject();
		if (info.find("status") != info.end())
		{
			string status = *(info.at("status"));
			success = (status == "ok");
			if (!success)
				SetLastMessage(*(info.at("message")));

			res = true;
		}
    }

	if (!res)
	{
		if (json)
			SetLastMessage("CreateRepository Invalid JSON received: '" + json->Stringify() + "' from '" + response + "'");
		else
			SetLastMessage("CreateRepository Not JSON: '" + response + "'");
	}

    if (json)
        delete json;

    return (res && success);
}
