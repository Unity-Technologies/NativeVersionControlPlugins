#ifndef XFERINTERFACE_H
#define XFERINTERFACE_H

#include <string>

class TransferInterface
{
public:
    virtual bool Upload(const std::string& url, const std::string& path) = 0;
	virtual bool Download(const std::string& url, const std::string& path) = 0;
};

#endif // XFERINTERFACE_H
