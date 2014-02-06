#pragma once

#include <string>
#include <map>

class RESTInterface
{
public:
	static bool	ParseUrl(const std::string& url, std::string* server = NULL, std::string* path = NULL, std::map<std::string, std::string>* queryString = NULL);
    
	virtual bool GetJSON(const std::string& url, std::string& response) = 0;
	virtual bool PostJSON(const std::string& url, const std::string& json, std::string& response) = 0;
	virtual bool Post(const std::string& url, const std::string& postData, std::map<std::string, std::string>* headers, std::string& response) = 0;
	virtual bool Get(const std::string& url, std::map<std::string, std::string>* headers, std::string& response) = 0;
};
