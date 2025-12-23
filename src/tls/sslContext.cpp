#include <include/tls/sslContext.h>
#include <assert.h>

namespace ZekHttpServer
{


void initializeOpenSSL(){
    OPENSSL_init_ssl(
        OPENSSL_INIT_LOAD_SSL_STRINGS
        | OPENSSL_INIT_LOAD_CRYPTO_STRINGS
        | OPENSSL_INIT_ADD_ALL_CIPHERS
        | OPENSSL_INIT_ADD_ALL_DIGESTS,
        nullptr
    );
}

SSLContext::SSLContext(const std::string key_path, const std::string certification_path)
: m_key_path(key_path)
, m_certification_path(certification_path)
{

    initializeOpenSSL();

    const SSL_METHOD *method = TLS_server_method();

    m_ssl_ctx = SSL_CTX_new(method);

    setOptions();

    loadKeyAndCertification();

    setSessionCache();


    // 禁用TLS会话缓存
    // SSL_CTX_set_session_cache_mode(m_ssl_ctx, SSL_SESS_CACHE_OFF);

    // SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_TICKET);

    ZekAsyncLogger::LOG_INFO(
        "initial ssl context"
    );

}

SSLContext::~SSLContext(){
    if(m_ssl_ctx){
        SSL_CTX_free(m_ssl_ctx);
    }
    EVP_cleanup();
}




void SSLContext::setOptions(){
    long no_versions = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
    
    long options = SSL_OP_CIPHER_SERVER_PREFERENCE;

    options |= SSL_OP_NO_COMPRESSION;

    SSL_CTX_set_options(m_ssl_ctx, options | no_versions);

    SSL_CTX_set_min_proto_version(m_ssl_ctx, TLS1_2_VERSION);
    
    SSL_CTX_set_max_proto_version(m_ssl_ctx, TLS1_2_VERSION);

}

void SSLContext::loadKeyAndCertification(){
    SSL_CTX_use_certificate_file(m_ssl_ctx, m_certification_path.c_str(), SSL_FILETYPE_PEM);

    SSL_CTX_use_PrivateKey_file(m_ssl_ctx, m_key_path.c_str(), SSL_FILETYPE_PEM);

    int rt = SSL_CTX_check_private_key(m_ssl_ctx);

    assert(rt == 1);

}

void SSLContext::setSessionCache(){
    SSL_CTX_set_session_cache_mode(m_ssl_ctx, SSL_SESS_CACHE_SERVER);

    SSL_CTX_sess_set_cache_size(m_ssl_ctx, 512);

    SSL_CTX_set_timeout(m_ssl_ctx, 120);
}
    
    
    
} // namespace ZekHttpServer
