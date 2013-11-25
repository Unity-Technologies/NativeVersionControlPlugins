#ifndef XFERINTERFACE_H
#define XFERINTERFACE_H

#include <string>
#include <vector>
#include <map>

class TransferInterface
{
public:
    virtual bool Upload(const std::string& url, const std::string& path) = 0;
	virtual bool Download(const std::string& url, const std::string& path) = 0;
	virtual bool ApplyChanges(const std::string& url, std::map<std::string, std::string>* headers,
							  const std::map<std::string, std::string>& addOrModyFiles,
							  const std::vector<std::string>& deleteFiles,
							  const std::string& comment, std::string& response) = 0;
};

#endif // XFERINTERFACE_H
