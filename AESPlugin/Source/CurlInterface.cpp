#include "CurlInterface.h"
#include "Utility.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

const char* USERAGENT = "Unity/AESPlugin (http://unity3d.com)";

static string EscapedURL(const string& url)
{
	string ignoredChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~-_./:?=&@";
	string urlCopy;
	urlCopy.reserve(url.length());
	
	char hex[3];
	memset(hex, 0x00, sizeof(hex));
	for (unsigned int i = 0; i < url.length(); i++)
	{
		if (ignoredChars.find((char)url[i]) != string::npos)
		{
			urlCopy += (char)url[i];
		}
		else
		{
			sprintf(hex, "%02X", (char)url[i]);
			urlCopy.append("%");
			urlCopy.append(hex);
		}
	}
	
	return urlCopy;
}

size_t CurlInterface::HeaderCallback(void *data, size_t size, size_t elements, void *callback)
{
	CurlInterface *pThis = static_cast<CurlInterface *>(callback);
	string header = "";
	header.append((char *)data, size*elements);
	header = Trim(header,'\n');
	header = Trim(header,'\r');
	
	if (!header.empty())
    {
        vector<string> parts;
        if (header.find("HTTP/1.1") != string::npos)
		{
            Tokenize(parts, header, " ");
			if (parts.size() > 1)
			{
				pThis->m_ResponseCode = ::atoi(parts[1].c_str());
			}
		}
        else
        {
            Tokenize(parts, header, ":");
            if (parts.size() > 1)
            {
                string key = parts[0];
                string value = Join(parts.begin()+1, parts.end(), ":");
                value = Trim(value,'\n');
                value = Trim(value,'\r');
                value = Trim(value);
                pThis->m_ResponseHeaders.insert(make_pair(key, value));
            }
        }
	}
    return size*elements;
}

size_t CurlInterface::WriteMemoryCallback(void *data, size_t size, size_t elements, void *callback)
{
	CurlInterface *pThis = static_cast<CurlInterface *>(callback);
	pThis->m_ResponseString.append((char *)data, size*elements);
	return size*elements;
}

size_t CurlInterface::WriteFileCallback(void *data, size_t size, size_t elements, void *callback)
{
	CurlInterface *pThis = static_cast<CurlInterface *>(callback);
    size_t nwrite = fwrite(data, size, elements, pThis->m_File);
    return nwrite;
}

size_t CurlInterface::ReadFileCallback(void *data, size_t size, size_t elements, void *callback)
{
	CurlInterface *pThis = static_cast<CurlInterface *>(callback);
    size_t nread = fread(data, size, elements, pThis->m_File);
    return nread;
}

CurlInterface::CurlInterface(int timeout) : m_ResponseString(""), m_ResponseCode(0), m_Headers(NULL), m_Form(NULL), m_ConnectTimeout(timeout), m_File(NULL)
{
	m_CurlErrorBuffer[0] = '\0';
	m_Cookies.clear();
	m_ResponseHeaders.clear();
}

CurlInterface::~CurlInterface()
{
    if (m_Headers)
    {
        curl_slist_free_all(m_Headers);
		m_Headers = NULL;
    }
    
    if (m_File)
    {
        fflush(m_File);
        fclose(m_File);
		m_File = NULL;
    }
}

void CurlInterface::SetErrorMessage(const char* c)
{
    strncpy(m_CurlErrorBuffer, c, CURL_ERROR_SIZE);
}

bool CurlInterface::GetJSON(const string& url, string& response)
{
    map<string, string> headers;
    bool res = Get(url, &headers, response);
    return res;
}

bool CurlInterface::PostJSON(const string& url, const string& json, string& response)
{
    return Post(url, json, NULL, response);
}

bool CurlInterface::Get(const string& url, map<string, string>* headers, string& response)
{
	return DoCurl(kGET, url, NULL, headers, response);
}

bool CurlInterface::Post(const string& url, const string& postData, map<string, string>* headers, string& response)
{
    string data = postData;
	return DoCurl(kPOST, url, &data, headers, response);
}

bool CurlInterface::Upload(const string& url, const string& path)
{
    string response = "";
    string data = path;
	return DoCurl(kUPLOAD, url, &data, NULL, response);
}

bool CurlInterface::Download(const string& url, const string& path)
{
    string response = "";
    string data = path;
	return DoCurl(kDOWNLOAD, url, &data, NULL, response);
}

#define CHECKCURLFORM(x) res = (x); if (res != CURL_FORMADD_OK) { SetErrorMessage("Fill FORM failed"); return false; }
bool CurlInterface::ApplyChanges(const string& url, map<string, string>* headers, const map<string, string>& addOrModyFiles, const vector<string>& deleteFiles, const string& comment, string& response)
{
	if (m_Form != NULL)
	{
		curl_formfree(m_Form);
	}
	
	CURLFORMcode res = CURL_FORMADD_OK;
	struct curl_httppost* last = NULL;
	
	int pos = 0;
	for (map<string, string>::const_iterator i = addOrModyFiles.begin() ; i != addOrModyFiles.end() ; i++)
	{
		string file = i->first;
		string name = string("file.") + IntToString(pos);
		CHECKCURLFORM(curl_formadd(&m_Form, &last, CURLFORM_COPYNAME, name.c_str(), CURLFORM_FILE, file.c_str(), CURLFORM_END));
		
		string dest = i->second;
		name = string("path.") + IntToString(pos);
		CHECKCURLFORM(curl_formadd(&m_Form, &last, CURLFORM_COPYNAME, name.c_str(), CURLFORM_COPYCONTENTS, dest.c_str(), CURLFORM_END));
		
		pos++;
	}
	
	pos = 0;
	for (vector<string>::const_iterator i = deleteFiles.begin() ; i != deleteFiles.end() ; i++)
	{
		string file = (*i);
		string name = string("delete.") + IntToString(pos);
		CHECKCURLFORM(curl_formadd(&m_Form, &last, CURLFORM_COPYNAME, name.c_str(), CURLFORM_COPYCONTENTS, file.c_str(), CURLFORM_END));
		pos++;
	}
	
	CHECKCURLFORM(curl_formadd(&m_Form, &last, CURLFORM_COPYNAME, "comment", CURLFORM_COPYCONTENTS, comment.c_str(), CURLFORM_END));
	
	return DoCurl(kFORM, url, NULL, headers, response);
}

#define CHECKCURL(x) res = (x); if (res != CURLE_OK) { SetErrorMessage(curl_easy_strerror(res)) ; curl_easy_cleanup(handle); return false; }
bool CurlInterface::DoCurl(CurlMethod method, const string& url, string* data, map<string, string>* headers, string& response)
{
    if (m_Headers != NULL)
    {
        curl_slist_free_all(m_Headers);
    }
    m_Headers = NULL;
    if (headers != NULL)
    {
        for (map<string, string>::const_iterator i = headers->begin(); i != headers->end() ; i++)
        {
            string headerValue = i->first;
            headerValue += ": ";
            headerValue += i->second;
            m_Headers = curl_slist_append(m_Headers, headerValue.c_str());
        }
    }
    
    string cookieValue = "";
    if (this->m_Cookies.size() > 0)
    {
        cookieValue += "Cookie: ";
        for(CookieJar::iterator i = this->m_Cookies.begin(); i != m_Cookies.end(); i++)
        {
            cookieValue += i->first;
            cookieValue += "=";
            cookieValue += i->second;
        }
        m_Headers = curl_slist_append(m_Headers, cookieValue.c_str());
    }
    
    CURLcode res = CURLE_OK;
    CURL* handle = curl_easy_init();
    if (handle == NULL)
    {
        SetErrorMessage("Cannot initialize CURL handle");
        return false;
    }
    
	string encodedUrl = EscapedURL(url);
	
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, m_CurlErrorBuffer));
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_URL, encodedUrl.c_str()));
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_USERAGENT, USERAGENT));
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0));
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, CurlInterface::HeaderCallback));
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_HEADERDATA, this));
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1));
    if (m_Headers != NULL)
    {
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_HTTPHEADER, m_Headers));
    }
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0));
	CHECKCURL(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1));
    CHECKCURL(curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, m_ConnectTimeout));
    //CHECKCURL(curl_easy_setopt(handle, CURLOPT_VERBOSE, 1));
    
    if (method == kGET)
    {
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlInterface::WriteMemoryCallback));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEDATA, this));
    }
    else if (method == kPOST)
    {
        if (data == NULL)
        {
            SetErrorMessage("Missing POST data");
            curl_easy_cleanup(handle);
            return false;
        }
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlInterface::WriteMemoryCallback));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEDATA, this));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_POST, 1));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data->c_str()));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, data->size()));
    }
    else if (method == kUPLOAD)
    {
        if (data == NULL)
        {
            SetErrorMessage("Missing file name");
            curl_easy_cleanup(handle);
            return false;
        }
        
        if (m_File != NULL) fclose(m_File);
        m_File = fopen(data->c_str(), "rb");
        if (m_File == NULL)
        {
            string msg = "Unable to open file ";
            msg += (*data);
            msg += " for reading";
            SetErrorMessage(msg.c_str());
            curl_easy_cleanup(handle);
            return false;
        }
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_READFUNCTION, CurlInterface::ReadFileCallback));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_READDATA, this));
    }
    else if (method == kDOWNLOAD)
    {
        if (data == NULL)
        {
            SetErrorMessage("Missing file name");
            curl_easy_cleanup(handle);
            return false;
        }
        
        if (m_File != NULL) fclose(m_File);
        m_File = fopen(data->c_str(), "wb");
        if (m_File == NULL)
        {
            string msg = "Unable to open file ";
            msg += (*data);
            msg += " for writting";
            SetErrorMessage(msg.c_str());
            curl_easy_cleanup(handle);
            return false;
        }
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlInterface::WriteFileCallback));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEDATA, this));
    }
	else if (method == kFORM)
    {
		if (m_Form == NULL)
		{
			SetErrorMessage("Missing form data");
            curl_easy_cleanup(handle);
            return false;
		}
		
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlInterface::WriteMemoryCallback));
        CHECKCURL(curl_easy_setopt(handle, CURLOPT_WRITEDATA, this));
		CHECKCURL(curl_easy_setopt(handle, CURLOPT_HTTPPOST, m_Form));
	}
    
    m_ResponseString = "";
    m_ResponseHeaders.clear();
    m_ResponseCode = 0;
    
    CHECKCURL(curl_easy_perform(handle));
    curl_easy_cleanup(handle);

	if ((method == kDOWNLOAD || method == kUPLOAD) && m_File != NULL)
	{
		fflush(m_File);
		fclose(m_File);
		m_File = NULL;
	}
	
	if (method == kFORM && m_Form != NULL)
	{
		curl_formfree(m_Form);
		m_Form = NULL;
	}
    
    string location = "";
    map<string,string>::const_iterator i = m_ResponseHeaders.find("Location");
    if (i != m_ResponseHeaders.end())
    {
        location = i->second;
    }
    
    i = m_ResponseHeaders.find("Set-Cookie");
    if (i != m_ResponseHeaders.end())
    {
        vector<string> parts;
        Tokenize(parts, i->second, ";");
        if (parts.size() > 0 )
        {
            // we are only interested in the key/value
            string line = parts[0];
            string key = line.substr(0,line.find_first_of("="));
            string value = line.substr(line.find_first_of("=") + 1);
            m_Cookies[key] = value;
        }
    }
    
	// handle redirects
	if (m_ResponseCode == 302 && location.length() > 0 )
	{
		// follow redirect
		string server;
		RESTInterface::ParseUrl(url, &server);
		
        string followUrl = server + location;
        map<string, string> backupHeaders = m_ResponseHeaders;
		bool res = Get(followUrl, headers, response);
        if (headers != NULL)
        {
            for (map<string, string>::const_iterator i = backupHeaders.begin() ; i != backupHeaders.end() ; i++)
            {
                headers->insert(*i);
            }
        }
        return res;
	}
    
    response = m_ResponseString;
    if (headers != NULL)
    {
        for (map<string, string>::const_iterator i = m_ResponseHeaders.begin() ; i != m_ResponseHeaders.end() ; i++)
        {
            headers->insert(*i);
        }
    }
    
	return true;
}