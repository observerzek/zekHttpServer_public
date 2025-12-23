#pragma once
#include <include/utils/utility.h>
#include <openssl/ssl.h>
#include <lib/zekWebServer/include/buffer.h>
#include <include/tls/sslContext.h>
#include <lib/zekWebServer/include/tcpConnection.h>

namespace ZekHttpServer
{


class SSLConnection{

public:

    #ifdef OVERLOAD_NEW_AND_DELETE

    void* operator new(size_t bytes){
        return MemoryPool::allocate(bytes);
    }

    void operator delete(void *ptr){
        MemoryPool::deallocate(ptr, sizeof(SSLConnection));
    }

    #endif


private:
    SSL *m_ssl;

    bool m_is_connected;

    BIO *m_in_bio;

    BIO *m_out_bio;

    ZekWebServer::Buffer m_encrypted_buffer;

    ZekWebServer::Buffer m_decrypted_buffer;

    int m_fd;


public:

    SSLConnection(SSLContext *ctx, int fd);

    ~SSLConnection();

    int getFd(){return m_fd;}

    bool isTLSConnected(){return m_is_connected;}

    ZekWebServer::Buffer* getEncryptedBuffer(){return &m_encrypted_buffer;}

    ZekWebServer::Buffer* getDecryptedBuffer(){return &m_decrypted_buffer;}

    int doTLSHandshake(ZekWebServer::TcpConnection *connection);

    int encryptData(ZekWebServer::TcpConnection *connection, const char *data, int len);

    int decryptData(ZekWebServer::TcpConnection *connection);



};

} // namespace ZekHttpServer