#include <include/tls/sslConnection.h>
#include <assert.h>
#include <openssl/err.h>

namespace ZekHttpServer
{
    
SSLConnection::SSLConnection(SSLContext *ctx, int fd)
: m_is_connected(false)
, m_fd(fd)
{
    m_ssl = SSL_new(ctx->getCTX());

    m_in_bio = BIO_new(BIO_s_mem());

    m_out_bio = BIO_new(BIO_s_mem());

    SSL_set_bio(m_ssl, m_in_bio, m_out_bio);

}

SSLConnection::~SSLConnection(){
    
    SSL_free(m_ssl);
}


int SSLConnection::doTLSHandshake(ZekWebServer::TcpConnection *connection){

    if(SSL_get_state(m_ssl) == TLS_ST_OK){
        m_is_connected = true;
    }

    if(!m_is_connected){

        int rt = 0;
        
        int recv_data_len = connection->m_read_buffer.getReadableSize();

        char recv_data[recv_data_len];

        connection->m_read_buffer.getBufferData(recv_data, recv_data_len);


        BIO_write(m_in_bio, recv_data, recv_data_len);

        rt = SSL_accept(m_ssl);

        if(rt == 1){
            m_is_connected = true;

            int send_data_len = BIO_pending(m_out_bio);

            if(send_data_len > 0){
                char send_data[send_data_len];
                
                int real_len = BIO_read(m_out_bio, send_data, send_data_len);

                connection->send(send_data, real_len);
                
            }

            int recv_rest_data = BIO_pending(m_in_bio);

            if(recv_rest_data > 0){
                int fd = connection->getFd();
                ZekAsyncLogger::LOG_ERROR("fd : %d, connect id : %d, send data : %d", fd, connection->getConnectId(),send_data_len);
                ZekAsyncLogger::LOG_ERROR("fd : %d, connect id : %d, rest data : %d", fd, connection->getConnectId(),recv_rest_data);

                memset(recv_data, 0, recv_data_len);

                int t = BIO_read(m_in_bio, recv_data, recv_data_len);

                connection->m_read_buffer.append(recv_data, t);

                rt = 2;
            }

        }
        else if(rt == 0){

        }
        else if(rt == -1){
            int err = SSL_get_error(m_ssl, rt);

            if(err == SSL_ERROR_WANT_READ){
                int send_data_len = BIO_pending(m_out_bio);

                char send_data[send_data_len];

                int real_len = BIO_read(m_out_bio, send_data, send_data_len);

                connection->send(send_data, real_len);

            }
            else{
                rt = 0;
            }
        }
        return rt;
    }
    return 1;

}

// 该函数很多地方需要优化
int SSLConnection::decryptData(ZekWebServer::TcpConnection *connection){
    int encrypted_data_len = connection->m_read_buffer.getReadableSize();

    char encrypted_data[encrypted_data_len];

    connection->m_read_buffer.copyBufferData(encrypted_data, encrypted_data_len);

    BIO_write(m_in_bio, encrypted_data, encrypted_data_len);

    int decrypted_data_len = BIO_pending(m_in_bio);

    if (decrypted_data_len < 1024 * 4){
        decrypted_data_len = 1024 * 4;
    }
    
    char decrypted_data[decrypted_data_len];

    int n = 0;

    bool continued = true;

    bool is_error = false;

    while(continued){

        int rt = SSL_read(m_ssl, decrypted_data, decrypted_data_len);

        if(rt > 0){
            n += rt;

            m_decrypted_buffer.append(decrypted_data, rt);

            memset(decrypted_data, 0, decrypted_data_len);

        }

        else if(rt == 0){
            continued = false;
            is_error = true;

            int err = SSL_get_error(m_ssl, rt);
            if(err == SSL_ERROR_ZERO_RETURN){
                SSL_shutdown(m_ssl);

                int close_data_len = BIO_pending(m_out_bio);

                char close_data[close_data_len];

                BIO_read(m_out_bio, close_data, close_data_len);

                connection->send(close_data, close_data_len);
                
            }
        }
        else if(rt == -1){
            continued = false;

            int err = SSL_get_error(m_ssl, rt);

            if(err == SSL_ERROR_WANT_READ){

            }
            else if(err == SSL_ERROR_SSL){
                SSL_shutdown(m_ssl);

                int close_data_len = BIO_pending(m_out_bio);

                char close_data[close_data_len];

                BIO_read(m_out_bio, close_data, close_data_len);

                connection->send(close_data, close_data_len);

                is_error = true;
            }
        }
    }

    connection->m_read_buffer.getBufferData(encrypted_data_len);

    return is_error ? -1 : n;
}

int SSLConnection::encryptData(ZekWebServer::TcpConnection *connection, const char *data, int len){
    SSL_write(m_ssl, data, len);

    int encryted_data_len = BIO_pending(m_out_bio);

    char encryted_data[encryted_data_len];

    int rt = BIO_read(m_out_bio, encryted_data, encryted_data_len);

    if(connection->getEventLoop()->ownThread()){

        connection->send(encryted_data, encryted_data_len);

    }
    else{
        
        std::shared_ptr<char []> encryted_data_s(new char[encryted_data_len]());
        
        memcpy(encryted_data_s.get(), encryted_data, encryted_data_len);
        
        connection->send(encryted_data_s, encryted_data_len);


        
    }
    


    return rt;
}


} // namespace ZekHttpServer