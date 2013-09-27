#include "AESClient.h"

AESClient::AESClient() : m_handle(NULL)
{
}

AESClient::~AESClient()
{
    if (m_handle)
        curl_easy_cleanup(m_handle);
}

bool AESClient::Initialize()
{
    m_handle = curl_easy_init();
    return (m_handle != NULL);
}