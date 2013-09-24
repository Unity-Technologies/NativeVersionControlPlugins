#include "AESConnection.h"

AESConnection::AESConnection() : m_handle(NULL)
{
}

AESConnection::~AESConnection()
{
    if (m_handle)
        curl_easy_cleanup(m_handle);
}

int AESConnection::Initialize()
{
    m_handle = curl_easy_init();
    if (m_handle)
    {
        
    }
}