#include "AESConnection.h"

AESConnection::AESConnection() : m_handle(NULL)
{
}

AESConnection::~AESConnection()
{
    if (m_handle)
        curl_easy_cleanup(m_handle);
}

bool AESConnection::Initialize()
{
    m_handle = curl_easy_init();
    return (m_handle != NULL);
}