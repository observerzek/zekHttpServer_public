#pragma once
#include <include/utils/utility.h>
#include <openssl/ssl.h>
#include <string>

namespace ZekHttpServer
{

class SSLContext{

private:
    std::string m_key_path;

    std::string m_certification_path;

    SSL_CTX *m_ssl_ctx;

public:

    SSLContext(const std::string key_path, const std::string certification_path);

    SSLContext() = delete;

    ~SSLContext();

    SSL_CTX *getCTX(){return m_ssl_ctx;}

    void setOptions();

    void loadKeyAndCertification();

    void setSessionCache();

};



}