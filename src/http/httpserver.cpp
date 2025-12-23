#include <include/http/httpserver.h>
#include <new>
#include <include/middleware/cookiemiddleware.h>
#include <include/middleware/authorizationmiddleware.h>

namespace ZekHttpServer
{

void logForConnect(ZekWebServer::TcpConnection *connection, const char * data){
    int connect_id = connection->getConnectId();
    std::string total_data = "fd : %d ";
    total_data += "connect id : %d ";
    total_data += data;
    total_data += " IP %s: , Port : %d";
    ZekAsyncLogger::LOG_INFO(
        total_data.c_str(),
        connection->getFd(), 
        connect_id,
        connection->getRemoteInfo().getIP().c_str(),
        connection->getRemoteInfo().getPort()
    );
}


HttpServer::HttpServer(int thread_count, int port, const char *ip)
{

    m_main_loop = new ZekWebServer::EventLoop();

    m_tcpserver = new ZekWebServer::TcpServer(
        thread_count,
        m_main_loop,
        port,
        ip
    );
    bindEventCallBack();

    m_use_cookie = true;

    addMiddleWare();

}

HttpServer::HttpServer(bool use_ssl, const std::string &key_path, const std::string &certification_path
    ,int thread_count, int port, const char *ip)
: m_use_ssl(use_ssl)
{
    if(use_ssl){
        m_ssl_ctx = new SSLContext(key_path, certification_path);
    }

    m_main_loop = new ZekWebServer::EventLoop();

    m_tcpserver = new ZekWebServer::TcpServer(
        thread_count,
        m_main_loop,
        port,
        ip
    );
    bindEventCallBack();

    m_use_cookie = true;

    addMiddleWare();

}
    

void HttpServer::addMiddleWare(){
    if(m_use_cookie){
        std::unique_ptr<BaseMiddleWare> middle_ware (new CookieMiddleWare());
        m_middle_ware_chain.addMiddleWare(std::move(middle_ware));
    }

    std::unique_ptr<BaseMiddleWare> authorization_middle_ware(new AuthorizationMiddleWare());
    m_middle_ware_chain.addMiddleWare(std::move(authorization_middle_ware));


}

HttpServer::~HttpServer(){
    if(m_tcpserver){
        delete m_tcpserver;
    }
    if(m_main_loop){
        delete m_main_loop;
    }

    if(m_use_ssl){

        while(!m_ssl_connections.empty()){
            auto it = m_ssl_connections.begin();
            delete it->second;
            m_ssl_connections.erase(it->first);
        }

        delete m_ssl_ctx;
    }

}

void HttpServer::bindEventCallBack(){

    m_tcpserver->setConnectEventCB(
        std::bind(
            &HttpServer::handleConnect, this, std::placeholders::_1
        )
    );

    m_tcpserver->setReadEventCB(
        std::bind(
            &HttpServer::handleReqeust, this, std::placeholders::_1
        )
    );

    m_tcpserver->setCloseEventCB(
        std::bind(
            &HttpServer::handleClose, this, std::placeholders::_1
        )
    );
}

void HttpServer::handleConnect(ZekWebServer::TcpConnection *connection){
    logForConnect(connection, "successful connect.");
}

void HttpServer::handleClose(ZekWebServer::TcpConnection *connection){
    logForConnect(connection, "close.");

    if(m_use_ssl){
        int fd = connection->getFd();

        auto it = m_ssl_connections.find(fd);

        if(it != m_ssl_connections.end()){
            SSLConnection *ssl_connection = it->second;

            m_ssl_connections.erase(fd);

            delete ssl_connection;
        }
    }
}


int HttpServer::handleTLSHandshare(SSLConnection *ssl_connection, ZekWebServer::TcpConnection *connection){

    int fd = connection->getFd();

    logForConnect(connection, "handle TLS handshake");

    int rt = ssl_connection->doTLSHandshake(connection);


    if(rt == 0){

        connection->close();

    }
    else if(rt == 1 || rt == 2){
        logForConnect(connection, "successful complete TLS handshake");

    }

    if(rt != 2) return -1;

    ZekAsyncLogger::LOG_INFO("fd : %d connect id : %d, has stuck", fd, connection->getConnectId());

    // rt == 2
    // 表示发生了粘包
    // 需要把数据重新写入到TCP缓存里
    return 2;
}


void HttpServer::handleReqeust(ZekWebServer::TcpConnection *connection){
    if(!connection->haveUniqueSpace()){
        connection->setUniqueSpace(sizeof(HttpContent));
        // 在这个TcpConnection实例独有的空间上
        // 执行HttpContent的构造函数
        ::new(connection->getUniqueSpace()) HttpContent();

    }

    ZekWebServer::Buffer *recv_buffer  = &(connection->m_read_buffer);
    
    int fd = connection->getFd();

    if(m_use_ssl){

        auto it = m_ssl_connections.find(fd);

        SSLConnection *ssl_connection = nullptr;

        if(it == m_ssl_connections.end()){
            ssl_connection = new SSLConnection(m_ssl_ctx, fd);
            m_ssl_connections[fd] = ssl_connection;
        }

        ssl_connection = m_ssl_connections[fd];

        if(!ssl_connection->isTLSConnected()){
            int rt = handleTLSHandshare(ssl_connection, connection);

            if(rt != 2){
                return;
            }
        }

        // -1 表示读取数据出错
        int rt = ssl_connection->decryptData(connection);

        if(rt == -1){

            ZekAsyncLogger::LOG_FATAL("fd : %d , coonect id : %d, decrypt data error.", fd, connection->getConnectId());

            connection->close();

            return;
        }

        recv_buffer = ssl_connection->getDecryptedBuffer();
    }

    HttpContent *content = (HttpContent*)connection->getUniqueSpace();

    logForConnect(connection, "handle request.");

    // 输出收到的HTTP报文
    // std::string test = recv_buffer->copyBufferData(recv_buffer->getReadableSize());
    // std::cout << test << std::endl;

    if(!content->parseRequest(recv_buffer) && content->isComplete()){

        const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";

        if(m_use_ssl){
            SSLConnection *ssl_connection = m_ssl_connections[fd];

            ssl_connection->encryptData(connection, response, strlen(response));

        }
        else{
            connection->send(response, strlen(response));
        }

        content->reset();

        connection->close();

        // 2025.10.03
        // 这里好像有点问题
        // 如果前面的400响应报文因为TCP发送缓冲区满了
        // 没发出去
        // 这里直接关闭连接，会导致后续这个报文发送失败
        // m_tcpserver->destroyTcpConnection(fd);

    }

    if(content->isComplete()){
        sendHttpResponse(connection, content);
    }

}

void HttpServer::sendHttpResponse(ZekWebServer::TcpConnection *connection, HttpContent *content){
    HttpResponse response;

    HttpRequest *request = content->getHttpRequest();

    int fd = connection->getFd();

    SSLConnection *ssl_connection = nullptr;

    request->setTcpConnection(connection);

    if(m_use_ssl){
        ssl_connection = m_ssl_connections[fd];

        if(!request->getSslConnection()){
            request->setSslConnection(ssl_connection);
        }
    }

    Session *session = nullptr;

    if(m_use_cookie){
        std::string name_id = request->getHeaderValue("Cookie");

        std::string id = name_id.substr(name_id.find('=') + 1);

        session = m_session_manager.findSession(id);
        
        if(id.empty()){
            id = session->getId();
        }

    }


    bool continued = true;

    int layer = 0;

    // forward返回值表示接下来执行中间层的位置
    layer = m_middle_ware_chain.forward(request, &response, session);

    bool rt = false;

    std::string real_response;

    // layer表示的是接下来要执行中间层的位置
    // layer == 中间层数量 + 1 表示所有中间层已经执行完毕
    // 接下来要执行 route 层
    // 如果layer不等于中间层的数量 + 1
    // 表示某些中间层执行失败
    // 此时不应该继续执行原有逻辑
    if(layer == m_middle_ware_chain.getLayerSize() + 1){

        rt = route(request, &response);

        if(rt){
            // real_response = response.createResponse();
            logForConnect(connection, "successful create http response.");
        }
        else{
            logForConnect(connection, "method or path error, 404 not found.");
            real_response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
    }

    // backward应该从最后执行的中间层开始
    // layer - 1就表示这个位置
    m_middle_ware_chain.backward(layer - 1, request, &response, session);

    if(real_response.empty()){
        real_response = response.createResponse();
    }

    if(m_use_ssl){

        ssl_connection->encryptData(connection, real_response.c_str(), real_response.size());
    }
    else{
        connection->send(real_response.c_str(), real_response.size());
    }

    content->reset();

    bool is_close = false;

    if(request->getHeaderValue("Connection") == "close"){
        is_close = true;
    }

    if(!rt || is_close){

        connection->close();
        
    }

}


void HttpServer::start(){
    m_tcpserver->start();
}


void HttpServer::stop(){
    m_tcpserver->stop();
}

}