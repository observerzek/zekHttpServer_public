#pragma once
#include <include/utils/utility.h>
#include <lib/zekWebServer/include/tcpServer.h>
#include <lib/zekWebServer/include/tcpClient.h>
#include "./httpcontent.h"
#include "./httprequest.h"
#include "./httpresponse.h"
#include <unordered_map>
#include <include/tls/sslConnection.h>
#include <include/session/sessionmanager.h>
#include <atomic>
#include <functional>
#include <mutex>


namespace ZekHttpServer
{

class HttpsProxy{

public:

    struct ProxyNode{
        ZekWebServer::TcpConnection *server = nullptr;
        
        ZekWebServer::TcpConnection *client = nullptr;

        std::mutex mutex;

        ProxyNode(ZekWebServer::TcpConnection *s, ZekWebServer::TcpConnection *c){
            server = s;
            client = c;
        }

    };

    class ProxyManager{
    private:
        std::unordered_map<int, ProxyNode*> m_proxy_to_node;

        std::mutex m_mutex;


    public:

        ProxyManager(){}

        int createProxyNode(int proxy_id, ZekWebServer::TcpConnection *s, ZekWebServer::TcpConnection *c);

        void deleteProxyNode(int proxy_id, std::function<void ()> cb = nullptr);

        ProxyNode* getProxy(int proxy_id);

    };


private:

    std::string m_proxy_ip;

    int m_proxy_port;

    ZekWebServer::TcpServer *m_tcpserver;

    ZekWebServer::EventLoop *m_main_loop;

    ZekWebServer::TcpClient *m_tcpclient;

    bool m_use_ssl;

    SSLContext *m_ssl_ctx;
    
    std::unordered_map<int, SSLConnection*> m_ssl_connections;

    std::atomic<int> m_proxy_id_pool;

    ProxyManager m_proxys;

    std::string m_user_id;

    std::string m_user_password;

private:

    void bindEventCallBack();

    void bindProxyCallBack(int proxy_id, ZekWebServer::TcpConnection *s, ZekWebServer::TcpConnection *c);

    void handleProxyHandshake(ZekWebServer::TcpConnection *);

    // TcpServer回调函数的设置
    void handleServerConnect(ZekWebServer::TcpConnection *);
    
    void handleServerRead(int proxy_id, ZekWebServer::TcpConnection *);

    void handleServerWrite(int proxy_id, ZekWebServer::TcpConnection *);

    void handleServerClose(int proxy_id, ZekWebServer::TcpConnection *);

    // TcpClient回调函数的设置
    void handleClientConnect(ZekWebServer::TcpConnection *);
    
    void handleClientRead(int proxy_id, ZekWebServer::TcpConnection *);

    void handleClientWrite(int proxy_id, ZekWebServer::TcpConnection *);

    void handleClientClose(int proxy_id, ZekWebServer::TcpConnection *);

    int handleTLSHandshare(SSLConnection *ssl_connection, ZekWebServer::TcpConnection *connection);

    // 返回值为-1 表示出错
    // 返回值为 0，表示成功
    // 返回值为 1，待认证
    int checkAuthorized(ZekWebServer::TcpConnection *server_connection, char *data, size_t length);

    bool checkIdAndPassword(char *data, size_t length);

public:

    HttpsProxy(
        int thread_count, 
        int connect_port, const char *connect_ip,
        int listen_port, const char *listen_ip = nullptr,
        bool use_ssl = false,
        const std::string &key_path = nullptr, 
        const std::string &certification_path = nullptr,
        const std::string &user_id = nullptr,
        const std::string &user_password = nullptr
    );

    ~HttpsProxy();

    void start();

    void stop();



};

class HttpsProxyRequest : public ZekWebServer::Buffer{
public:

    #ifdef OVERLOAD_NEW_AND_DELETE

    void* operator new(size_t bytes){
        return MemoryPool::allocate(bytes);
    }

    void operator delete(void *ptr){
        MemoryPool::deallocate(ptr, sizeof(HttpsProxyRequest));
    }

    #endif

private:
    int m_server_id;

    std::string m_source_ip;

    int m_source_port;

    int m_proxy_fd;

    std::string m_proxy_ip;

    int m_proxy_port;

    int m_rest_body_length = 0;

    bool m_is_web_socket = false;

    bool m_is_authorized = false;

    int m_error_times = 0;

private:

    int parseHead(std::string &src, std::string &dest, size_t start, size_t end, int &read_send);

public:

    HttpsProxyRequest(int server_id,const std::string &proxy_ip, int proxy_port, ZekWebServer::SockAdd *source_info = nullptr);

    int parseHttpRequest(bool replace_ip_and_port = true);

    void setAuthorization(bool on){m_is_authorized = on;}

    bool isAuthorized(){return m_is_authorized;}

    void addErrorTime(){m_error_times++;}

    int getErrorTime(){return m_error_times;}
};

} // namespace ZekHttpServer 