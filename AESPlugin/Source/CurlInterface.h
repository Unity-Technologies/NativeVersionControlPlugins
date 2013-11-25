#ifndef CURLINTERFACE_H
#define CURLINTERFACE_H

#include "RESTInterface.h"
#include "TransferInterface.h"

#include <string>
#include <vector>
#include <map>
#include "curl/curl.h"

class CurlInterface: public RESTInterface, public TransferInterface
{
public:
    CurlInterface(int timeout = 0);
    ~CurlInterface();
    
	bool GetJSON(const std::string& url, std::string& response);
	bool PostJSON(const std::string& url, const std::string& json, std::string& response);
	bool Post(const std::string& url, const std::string& postData, std::map<std::string, std::string>* headers, std::string& response);
	bool Get(const std::string& url, std::map<std::string, std::string>* headers, std::string& response);
	
    bool Upload(const std::string& url, const std::string& path);
	bool Download(const std::string& url, const std::string& path);
	bool ApplyChanges(const std::string& url, std::map<std::string, std::string>* headers,
					  const std::map<std::string, std::string>& addOrModyFiles,
					  const std::vector<std::string>& deleteFiles,
					  const std::string& comment, std::string& response);

	inline std::string GetErrorMessage() {
		return std::string(m_CurlErrorBuffer);
	}
    inline int GetResponseCode() { return m_ResponseCode; }
    inline const std::string& GetResponse() { return m_ResponseString; }

private:
    enum CurlMethod { kGET, kPOST, kDOWNLOAD, kUPLOAD, kFORM };
	static size_t WriteMemoryCallback(void *data, size_t size, size_t elements, void *callback);
	static size_t WriteFileCallback(void *data, size_t size, size_t elements, void *callback);
	static size_t ReadFileCallback(void *data, size_t size, size_t elements, void *callback);
	static size_t HeaderCallback(void *data, size_t size, size_t elements, void *callback);

	void SetErrorMessage(const char* c);
    bool DoCurl(CurlMethod method, const std::string& url, std::string* postData, std::map<std::string, std::string>* headers, std::string& response);
	
    std::map<std::string, std::string> m_ResponseHeaders;
    std::string m_ResponseString;
    int m_ResponseCode;

	typedef std::map<std::string, std::string> CookieJar;
	CookieJar m_Cookies;
    
    char m_CurlErrorBuffer[CURL_ERROR_SIZE];
    struct curl_slist* m_Headers;
	struct curl_httppost* m_Form;
    int m_ConnectTimeout;
    
    FILE* m_File;
};

#endif // CURLINTERFACE_H
