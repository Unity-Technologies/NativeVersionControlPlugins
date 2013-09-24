#ifndef AESCONNECTION_H
#define AESCONNECTION_H

#include "curl/curl.h"

class AESConnection {
public:
    AESConnection();
    ~AESConnection();
    
    bool Initialize();

private:
    CURL* m_handle;
};

#endif // AESCONNECTION_H