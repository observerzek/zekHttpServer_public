#include <iostream>
#include <fstream>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/bioerr.h>

using std::cout;
using std::cin;
using std::endl;


std::string *DEMO;

std::string KEY_PATH = "../sslFiles/server.key";

std::string CERTIFICATION_PATH = "../sslFiles/server.crt";

bool STOP = false;


std::string RESPONSE = ""
"HTTP/1.1 200 OK\r\n"
;

void openDemo(){
    DEMO = new std::string();
    DEMO->reserve(4096);
    std::fstream fs("../demo.html", std::ios::in);
    std::string line;
    while(getline(fs, line)){
        *DEMO += line;
    }
    RESPONSE += "Content-Length: " + DEMO->size();
    RESPONSE += "\r\n\r\n" + *DEMO;
}

SSL_CTX* OpenSSLInitialization(){
    OPENSSL_init_ssl(
        OPENSSL_INIT_LOAD_SSL_STRINGS
        | OPENSSL_INIT_LOAD_CRYPTO_STRINGS
        | OPENSSL_INIT_ADD_ALL_CIPHERS
        | OPENSSL_INIT_ADD_ALL_DIGESTS,
        nullptr
    );

    SSL_load_error_strings();

    const SSL_METHOD *method = TLS_server_method();

    SSL_CTX *ctx = SSL_CTX_new(method);

    long no_versions = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
    
    long options = SSL_OP_CIPHER_SERVER_PREFERENCE;

    options |= SSL_OP_NO_COMPRESSION;

    SSL_CTX_set_options(ctx, options | no_versions);

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    
    SSL_CTX_set_max_proto_version(ctx, TLS1_2_VERSION);

    // 加载私钥和证书
    SSL_CTX_use_certificate_file(ctx, CERTIFICATION_PATH.c_str(), SSL_FILETYPE_PEM);

    SSL_CTX_use_PrivateKey_file(ctx, KEY_PATH.c_str(), SSL_FILETYPE_PEM);

    int rt = SSL_CTX_check_private_key(ctx);

    assert(rt == 1);

    // 设置缓存相关操作
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER);

    SSL_CTX_sess_set_cache_size(ctx, 512);

    SSL_CTX_set_timeout(ctx, 120);

    return ctx;
}


int initializeSocket(){
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_data;
    server_data.sin_family = AF_INET;
    server_data.sin_addr.s_addr = INADDR_ANY;
    server_data.sin_port = htons(6666);

    bind(listen_fd, (sockaddr *)&server_data, sizeof(server_data));
    
    listen(listen_fd, 10);

    timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int on = 1;

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    return listen_fd;
}

int handleConnection(epoll_event &event, int listen_fd){
    sockaddr_in client_data;
    socklen_t client_data_len = sizeof(client_data);

    int connect_fd = accept(listen_fd, (sockaddr *)&client_data, &client_data_len);

    fcntl(connect_fd, F_SETFL, O_NONBLOCK);

    return connect_fd;
}


struct SSLfd{
    SSL *ssl;
    int fd;
    int hand_shake_time = 0;

    SSLfd(SSL *ssl_, int fd_){
        ssl = ssl_;
        fd = fd_;
    }
};

struct SSLfdMemBIO{
    SSL *ssl;
    int fd;
    BIO *in_bio;
    BIO *out_bio;
    int hand_shake_time = 0;

    SSLfdMemBIO(SSL *ssl_, int fd_){
        ssl = ssl_;
        fd = fd_;

        in_bio = BIO_new(BIO_s_mem());
        out_bio = BIO_new(BIO_s_mem());

        SSL_set_bio(ssl, in_bio, out_bio);
    }

    ~SSLfdMemBIO(){
        // BIO_free(in_bio);
        // BIO_free(out_bio);
    }
};

void singleSSLTest(){
    openDemo();

    int listen_fd = initializeSocket();

    int epoll_fd = epoll_create(1);

    epoll_event listen_event;
    listen_event.events = EPOLLIN;
    listen_event.data.fd = listen_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_event);

    SSL_CTX *ctx = OpenSSLInitialization();

    while(!STOP){

        epoll_event events[20];

        int n = epoll_wait(epoll_fd, events, 20, 5000);


        for(int i = 0; i < n; i++){
            epoll_event event = events[i];

            if(event.data.fd == listen_fd){
                int connect_fd = handleConnection(event, listen_fd);


                cout << "fd : " << connect_fd << ". successfully connect..." << endl;

                SSL *ssl = SSL_new(ctx);

                SSL_set_fd(ssl, connect_fd);

                SSLfd *ssl_fd = new SSLfd(ssl, connect_fd);

                epoll_event connect_event;
                connect_event.events = EPOLLIN;
                connect_event.data.ptr = ssl_fd;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &connect_event);

            }
            else{
                SSLfd *ssl_fd = (SSLfd*)event.data.ptr;
                int connect_fd = ssl_fd->fd;
                SSL *ssl = ssl_fd->ssl;

                int state = SSL_get_state(ssl);

                if(state != TLS_ST_OK){
                    int rt = SSL_accept(ssl);

                    ssl_fd->hand_shake_time++;

                    if(rt == 1){
                        cout << "fd : " << connect_fd << ". successfully handle TLS handshake.." 
                             << " hand shake time : " << ssl_fd->hand_shake_time << endl;
                    }
                    else if(rt == 0){
                        SSL_shutdown(ssl);

                        SSL_free(ssl);

                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_fd, &event);
    
                        shutdown(connect_fd, SHUT_WR);
                        
                        delete ssl_fd;
                    }
                    else if(rt == -1){
                        int err = SSL_get_error(ssl, rt);

                        cout << "fd : " << connect_fd << ". error code : " 
                             << err << " hand shake time : " << ssl_fd->hand_shake_time << endl;

                        if(err == SSL_ERROR_SSL || err == SSL_ERROR_ZERO_RETURN){
                            SSL_shutdown(ssl);

                            SSL_free(ssl);
    
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_fd, &event);
        
                            shutdown(connect_fd, SHUT_WR);
                            
        
                            delete ssl_fd;
                        }
                        
                    }

                }
                else{
                    int len = 4096;
                    char data[4096];
    
                    SSL_read(ssl, data, len);
    
                    int send_len = RESPONSE.size();
                    SSL_write(ssl, RESPONSE.c_str(), send_len);

                    SSL_shutdown(ssl);

                    SSL_free(ssl);

                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_fd, &event);

                    shutdown(connect_fd, SHUT_WR);
                    

                    delete ssl_fd;
                }

            }
            
        }

    }


}

void singleSSLTestWithMemBIO(){
    openDemo();

    int listen_fd = initializeSocket();

    int epoll_fd = epoll_create(1);

    epoll_event listen_event;
    listen_event.events = EPOLLIN;
    listen_event.data.fd = listen_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_event);

    SSL_CTX *ctx = OpenSSLInitialization();

    while(!STOP){

        epoll_event events[20];

        int n = epoll_wait(epoll_fd, events, 20, 5000);


        for(int i = 0; i < n; i++){
            epoll_event event = events[i];

            if(event.data.fd == listen_fd){
                int connect_fd = handleConnection(event, listen_fd);


                cout << "fd : " << connect_fd << ". successfully connect..." << endl;

                SSL *ssl = SSL_new(ctx);

                // SSL_set_fd(ssl, connect_fd);

                SSLfdMemBIO *ssl_fd = new SSLfdMemBIO(ssl, connect_fd);

                epoll_event connect_event;
                connect_event.events = EPOLLIN;
                connect_event.data.ptr = ssl_fd;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &connect_event);

            }
            else{
                SSLfdMemBIO *ssl_fd = (SSLfdMemBIO*)event.data.ptr;
                int connect_fd = ssl_fd->fd;
                SSL *ssl = ssl_fd->ssl;
                BIO *in_bio = ssl_fd->in_bio;
                BIO *out_bio = ssl_fd->out_bio;
        

                int state = SSL_get_state(ssl);

                char recv_buffer[4096] = {};

                char send_buffer[4096] = {};

                int n = 0;

                n = read(connect_fd, recv_buffer, 4096);

                if(n == 0){
                    SSL_free(ssl);

                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_fd, &event);

                    shutdown(connect_fd, SHUT_WR);
                    
                    delete ssl_fd;

                    continue;
                }

                cout << "\n" << n << endl;

                n = BIO_write(in_bio, recv_buffer, n);

                cout << "fd : "  << connect_fd << " state code : " << state << endl;

                if(state != TLS_ST_OK){

                    int rt = SSL_accept(ssl);

                    ssl_fd->hand_shake_time++;

                    if(rt == 1){
                        
                        state = SSL_get_state(ssl); 

                        cout << "fd : "  << connect_fd << " state code : " << state << endl;

                        n = BIO_read(out_bio, send_buffer, 4096);

                        n = write(connect_fd, send_buffer, n);

                        cout << "fd : " << connect_fd << ". successfully handle TLS handshake.." 
                             << " hand shake time : " << ssl_fd->hand_shake_time << endl;
                    }
                    else if(rt == 0){

                        SSL_free(ssl);

                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_fd, &event);
    
                        shutdown(connect_fd, SHUT_WR);
                        
                        delete ssl_fd;
                    }
                    else if(rt == -1){
                        int err = SSL_get_error(ssl, rt);

                        cout << "fd : " << connect_fd << ". error code : " 
                             << err << " hand shake time : " << ssl_fd->hand_shake_time << endl;

                        if(err == SSL_ERROR_SSL || err == SSL_ERROR_ZERO_RETURN){

                            SSL_free(ssl);
    
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_fd, &event);
        
                            shutdown(connect_fd, SHUT_WR);
                            
        
                            delete ssl_fd;
                        }
                        else {

                            state = SSL_get_state(ssl); 

                            cout << "fd : "  << connect_fd << " state code : " << state << endl;

                            n = BIO_read(out_bio, send_buffer, 4096);

                            n = write(connect_fd, send_buffer, n);

                        }
                        
                    }

                }
                else{
                    int len = 4096;
                    char data[4096];
    
                    SSL_read(ssl, data, len);
    
                    int send_len = RESPONSE.size();
                    SSL_write(ssl, RESPONSE.c_str(), send_len);

                    n = BIO_read(out_bio, send_buffer, 4096);

                    n = write(connect_fd, send_buffer, n);

                    SSL_shutdown(ssl);

                    n = BIO_read(out_bio, send_buffer, 4096);

                    n = write(connect_fd, send_buffer, n);

                    SSL_free(ssl);

                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_fd, &event);

                    shutdown(connect_fd, SHUT_WR);
                    

                    delete ssl_fd;
                }

            }
            
        }

    }


}


int main(){
    std::thread thread(singleSSLTestWithMemBIO);

    std::string op;

    // cout << "is stop : " << std::flush;
    // cin >> op;

    // STOP = true;

    if(thread.joinable()){
        thread.join();
    }

    return 0;
}