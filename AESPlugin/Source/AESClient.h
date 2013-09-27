#ifndef AESCLIENT_H
#define AESCLIENT_H

#include "curl/curl.h"

class AESClient
{
public:
    AESClient();
    ~AESClient();
    
    bool Initialize();

private:
    CURL* m_handle;
};

#endif // AESCLIENT_H