#pragma once
#include <include/utils/utility.h>
#include <lib/zekWebServer/include/tcpServer.h>
#include "./httpcontent.h"
#include "./httprequest.h"
#include "./httpresponse.h"
#include <include/router/router.h>
#include <unordered_map>
#include <include/tls/sslConnection.h>
#include <include/session/sessionmanager.h>
#include <include/middleware/middlewarechain.h>

namespace ZekHttpServer
{

class HttpServer : public Router{

public:


private:
    ZekWebServer::TcpServer *m_tcpserver;
    ZekWebServer::EventLoop *m_main_loop;
    // Router m_router;

    bool m_use_ssl = false;

    SSLContext *m_ssl_ctx;
    
    std::unordered_map<int, SSLConnection*> m_ssl_connections;

    bool m_use_cookie = false;

    SessionManager m_session_manager;

    MiddleWareChain m_middle_ware_chain;

private:
    void handleConnect(ZekWebServer::TcpConnection *);

    void handleReqeust(ZekWebServer::TcpConnection *);

    void handleClose(ZekWebServer::TcpConnection *);

    void bindEventCallBack();

    void sendHttpResponse(ZekWebServer::TcpConnection *connection, HttpContent *content);

    int handleTLSHandshare(SSLConnection *ssl_connection, ZekWebServer::TcpConnection *connection);

    void addMiddleWare();

public:
    HttpServer(int thread_count, int port, const char *ip = nullptr);

    HttpServer(bool use_ssl, const std::string &key_path, const std::string &certification_path
        ,int thread_count, int port, const char *ip = nullptr);


    ~HttpServer();

    void start();

    void stop();

};

    

} // namespace ZekHttpServer