#include <include/http/httpsproxy.h>
#include <new>
#include <cstring>
#include <algorithm>

namespace ZekHttpServer
{

int HttpsProxy::ProxyManager::createProxyNode(int proxy_id, ZekWebServer::TcpConnection *s, ZekWebServer::TcpConnection *c){

    if(!s || !c ) return -1;

    ProxyNode *node = new ProxyNode(s, c);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_proxy_to_node[proxy_id] = node;

    return proxy_id;
}

void HttpsProxy::ProxyManager::deleteProxyNode(int proxy_id, std::function<void ()> cb){

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_proxy_to_node.find(proxy_id);

    if(it != m_proxy_to_node.end()){

        assert(!it->second->client);

        assert(!it->second->server);

        delete it->second;

        m_proxy_to_node.erase(proxy_id);
        
        if(cb){
            cb();
        }
    }
}


HttpsProxy::ProxyNode* HttpsProxy::ProxyManager::getProxy(int proxy_id){

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_proxy_to_node.find(proxy_id);

    ProxyNode *proxy = nullptr;

    if(it != m_proxy_to_node.end()){
        proxy = m_proxy_to_node[proxy_id];
    }

    return proxy;
}



HttpsProxy::HttpsProxy(
    int thread_count, 
    int connect_port, const char *connect_ip,
    int listen_port, const char *listen_ip,
    bool use_ssl,
    const std::string &key_path, 
    const std::string &certification_path,
    const std::string &user_id,
    const std::string &user_password
)
: m_use_ssl(use_ssl)
, m_proxy_id_pool(1)
, m_user_id(user_id)
, m_user_password(user_password)
{
    m_proxy_ip = connect_ip;

    m_proxy_port = connect_port;


    if(use_ssl){
        m_ssl_ctx = new SSLContext(key_path, certification_path);
    }

    m_main_loop = new ZekWebServer::EventLoop();

    m_tcpserver = new ZekWebServer::TcpServer(
        thread_count,
        m_main_loop,
        listen_port,
        listen_ip
    );

    m_tcpclient = new ZekWebServer::TcpClient(
        thread_count,
        connect_port,
        connect_ip
    );
    
    bindEventCallBack();

}
    
HttpsProxy::~HttpsProxy(){
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

    if(m_tcpclient){
        delete m_tcpclient;
    }

}

void HttpsProxy::bindEventCallBack(){

    m_tcpserver->setConnectEventCB(
        std::bind(
            &HttpsProxy::handleServerConnect, this, std::placeholders::_1
        )
    );

    m_tcpserver->setReadEventCB(
        std::bind(
            &HttpsProxy::handleProxyHandshake, this, std::placeholders::_1
        )
    );

    m_tcpclient->setConnectEventCB(
        std::bind(
            &HttpsProxy::handleClientConnect, this, std::placeholders::_1
        )
    );

}

void HttpsProxy::bindProxyCallBack(int proxy_id, ZekWebServer::TcpConnection *server_connection, ZekWebServer::TcpConnection *client_connection){
    
    int client_fd = client_connection->getFd();

    int server_fd = server_connection->getFd();
    
    client_connection->setReadEventCB(
        std::bind(
            &HttpsProxy::handleClientRead, this, proxy_id, client_connection
        )
    );

    client_connection->setCloseEventCB(
        [=](){
            handleClientClose(proxy_id, client_connection);
            m_tcpclient->destroyTcpConnection(client_fd);
        }
    );

    server_connection->setReadEventCB(
        std::bind(
            &HttpsProxy::handleServerRead, this, proxy_id, server_connection
        )
    );

    // client_connection的EPOLLOUT事件的回调函数要延迟绑定
    // 因为要等它完成tcp连接后
    // 写事件的回调函数才能却换到这个handleClientWrite函数
    server_connection->setWriteEventCB(
        std::bind(
            &HttpsProxy::handleServerWrite, this, proxy_id, server_connection
        )
    );

    server_connection->setCloseEventCB(
        [=](){
            handleServerClose(proxy_id, server_connection);
            m_tcpserver->destroyTcpConnection(server_fd);
        }
    );
}


bool HttpsProxy::checkIdAndPassword(char *data, size_t length){
    BIO *b64 = BIO_new(BIO_f_base64());

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO *mem = BIO_new(BIO_s_mem());

    BIO_write(mem, data, length);

    BIO_push(b64, mem);

    char result[64];

    size_t size = 0;

    size = BIO_read(b64, result, 64);
    
    result[size] = '\0';

    std::string decrypted_data = result;

    size_t spilt = decrypted_data.find(":");

    std::string id = decrypted_data.substr(0, spilt);

    std::string password = decrypted_data.substr(spilt + 1);

    return (id == m_user_id && password == m_user_password) ? true : false;

}


int HttpsProxy::checkAuthorized(ZekWebServer::TcpConnection *server_connection, char *data, size_t length){

    HttpsProxyRequest* content = (HttpsProxyRequest*)server_connection->getUniqueSpace();

    char* authorize_head = strstr(data, "Authorization: ");

    std::string http_response;
    
    http_response = ""
    "HTTP/1.1 401 Unauthorized\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 3\r\n"
    "Data: Tue, 17 Nov 2025 19:00:00\r\n"
    "X-Content-Type-Options: nosniff\r\n"
    "Www-Authenticate: Basic realm=\"Restricted\"\r\n"
    "Content-Type: text/plain; charset=UTF-8\r\n"
    "\r\n"
    "zek"
    "";

    int result = 0;

    if(!authorize_head){

        if(m_use_ssl){

            SSLConnection *ssl_connection = m_ssl_connections[server_connection->getFd()];
            
            ssl_connection->encryptData(server_connection, http_response.c_str(), http_response.size());
        }
        else{
            server_connection->send(http_response.c_str(), http_response.size());
        }

        result = 1;
    }
    else{
        char *id_and_password_head = authorize_head + 15 + 6;

        char *id_and_password_end = strstr(id_and_password_head, "\r\n");

        bool correct = checkIdAndPassword(id_and_password_head, id_and_password_end - id_and_password_head);

        if(!correct){

            content->addErrorTime();

            result = 1;

            if(content->getErrorTime() >= 3){
                http_response = ""
                "HTTP/1.1 403 Forbidden\r\n"
                "Date: Fri, 31 Oct 2025 12:00:00 GMT\r\n"
                "Server: Apache/2.4.41 (Ubuntu)\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Content-Length: 172\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>403 Forbidden</title>"
                "</head>"
                "<body>"
                "<h1>Forbidden</h1>"
                "<p>You don't have permission to access this resource on this server.</p>"
                "</body>"
                "</html>"
                "";
                result = -1;
            }

            if(m_use_ssl){
                
                SSLConnection *ssl_connection = m_ssl_connections[server_connection->getFd()];
                
                ssl_connection->encryptData(server_connection, http_response.c_str(), http_response.size());
            }
            else{

                server_connection->send(http_response.c_str(), http_response.size());
            }
        }
        else{
            content->setAuthorization(true);
        }
    }
    return result;
}

void HttpsProxy::handleServerConnect(ZekWebServer::TcpConnection *server_connection){
    ZekAsyncLogger::LOG_INFO(
        "server id : %d successful connect", server_connection->getConnectId()
    );
}

void HttpsProxy::handleServerClose(int proxy_id, ZekWebServer::TcpConnection *server_connection){
    ZekAsyncLogger::LOG_INFO(
        "server id : %d close", server_connection->getConnectId()
    );

    ProxyNode *proxy = m_proxys.getProxy(proxy_id);

    std::lock_guard<std::mutex> lock(proxy->mutex);

    proxy->server = nullptr;

    int server_fd = server_connection->getFd();

    if(m_use_ssl){
        auto it = m_ssl_connections.find(server_fd);
        if(it != m_ssl_connections.end()){
            delete it->second;
            m_ssl_connections.erase(it);
        }
    }

    if(!proxy->client){
        m_proxys.deleteProxyNode(proxy_id);
    }
    else{
        // 触发client TcpConnection的写事件
        // 从而触发它的关闭流程
        proxy->client->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);
    }

}

void HttpsProxy::handleServerWrite(int proxy_id, ZekWebServer::TcpConnection *server_connection){
    
    ProxyNode *proxy = m_proxys.getProxy(proxy_id);

    std::lock_guard<std::mutex> lock(proxy->mutex);

    // 通过这个判断client端的 TcpConnection是否关闭
    // 如果为nullptr，则表示已经关闭
    // 那么就开始触发本连接的关闭流程
    if(!proxy->client){

        ZekAsyncLogger::LOG_INFO(
            "server id : %d close, triggered by client connection", server_connection->getConnectId()
        );

        int server_id = server_connection->getFd();

        proxy->server = nullptr;

        if(m_use_ssl){
            auto it = m_ssl_connections.find(server_id);
            if(it != m_ssl_connections.end()){
                delete it->second;
                m_ssl_connections.erase(it);
            }
        }

        m_tcpserver->destroyTcpConnection(server_id);

        m_proxys.deleteProxyNode(proxy_id);
    }
    
}

int HttpsProxy::handleTLSHandshare(SSLConnection *ssl_connection, ZekWebServer::TcpConnection *connection){

    int fd = connection->getFd();

    int rt = ssl_connection->doTLSHandshake(connection);

    ZekAsyncLogger::LOG_INFO(
        "server id : %d handle TLS handshake", connection->getConnectId()
    );

    if(rt == 0){
        m_ssl_connections.erase(fd);

        delete ssl_connection;

        m_tcpserver->destroyTcpConnection(fd);

        ZekAsyncLogger::LOG_INFO(
            "server id : %d stop TLS handshake", connection->getConnectId()
        );

    }
    else if(rt == 1 || rt == 2){
        ZekAsyncLogger::LOG_INFO(
            "server id : %d successful complete TLS handshake", connection->getConnectId()
        );
    }

    if(rt != 2) return -1;

    ZekAsyncLogger::LOG_INFO("server id : %d has stuck", connection->getConnectId());

    return 2;
}

void HttpsProxy::handleProxyHandshake(ZekWebServer::TcpConnection *server_connection){

    if(!server_connection->haveUniqueSpace()){
        
        server_connection->setUniqueSpace(sizeof(HttpsProxyRequest));

        ::new(server_connection->getUniqueSpace()) HttpsProxyRequest(
            server_connection->getConnectId(), m_proxy_ip, m_proxy_port
        );
    }

    int server_fd = server_connection->getFd();

    SSLConnection *ssl_connection = nullptr;

    if(m_use_ssl){

        auto it = m_ssl_connections.find(server_fd);
                
        if(it == m_ssl_connections.end()){
            ssl_connection = new SSLConnection(m_ssl_ctx, server_fd);
            m_ssl_connections[server_fd] = ssl_connection;
        }
        
        ssl_connection = m_ssl_connections[server_fd];
        
        if(!ssl_connection->isTLSConnected()){
            int rt = handleTLSHandshare(ssl_connection, server_connection);
            
            if(rt != 2){
                return;
            }
        }

    }

    int proxy_id = m_proxy_id_pool++;

    std::function<void(ZekWebServer::TcpConnection*)> client_write_cb = std::bind(
        &HttpsProxy::handleClientWrite,
        this,
        proxy_id,
        std::placeholders::_1
    );

    // 这个设计非常精妙
    // 我想破脑袋都都想不出来
    // 通过这个方法
    // 实现延迟绑定该TcpConnection EPOLLOUT事件的回调函数
    int relevant_id = server_connection->getConnectId();
    int client_fd = m_tcpclient->connectServer(relevant_id, client_write_cb);

    // 判断能否建立连接
    if(client_fd < 0){
        for(int i = 0; i < 5; i++){
            client_fd = m_tcpclient->connectServer(relevant_id, client_write_cb);
            if(client_fd > 0) break;
        }

        if(client_fd < 0){
            if(m_use_ssl){

                m_ssl_connections.erase(server_fd);

                delete ssl_connection;
            }
    
            m_tcpserver->destroyTcpConnection(server_fd);
    
            return;
        }
    }

    ZekWebServer::TcpConnection *client_connection = m_tcpclient->getConnection(client_fd);

    assert(client_connection);
    
    m_proxys.createProxyNode(proxy_id, server_connection, client_connection);

    bindProxyCallBack(proxy_id, server_connection, client_connection);

    handleServerRead(proxy_id, server_connection);

}

void HttpsProxy::handleServerRead(int proxy_id, ZekWebServer::TcpConnection *sever_connection){

    ProxyNode *proxy = m_proxys.getProxy(proxy_id);
    
    std::unique_lock<std::mutex> lock(proxy->mutex);
    
    ZekWebServer::TcpConnection *client_connection = proxy->client;

    int server_fd = sever_connection->getFd();

    SSLConnection *ssl_connection = nullptr;
    
    if(m_use_ssl){
        ssl_connection = m_ssl_connections[server_fd];
    }

    // client_connection 为nullptr
    // 表示这个连接已经关闭了
    if(!client_connection){

        if(m_use_ssl){

            m_ssl_connections.erase(server_fd);

            delete ssl_connection;
        }

        m_tcpserver->destroyTcpConnection(server_fd);

        proxy->server = nullptr;

        m_proxys.deleteProxyNode(proxy_id);

        ZekAsyncLogger::LOG_INFO(
            "server id : %d close", sever_connection->getConnectId()
        );

        return;
    }
    
    int client_fd = client_connection->getFd();

    int rt = 0;

    ZekWebServer::Buffer *recv_buffer = nullptr;

    if(m_use_ssl){
        rt= ssl_connection->decryptData(sever_connection);

        if(rt == 0){

            m_ssl_connections.erase(server_fd);
    
            delete ssl_connection;
    
            m_tcpserver->destroyTcpConnection(server_fd);
    
            proxy->server = nullptr;
    
            ZekAsyncLogger::LOG_INFO(
                "server id : %d close", sever_connection->getConnectId()
            );

            // 注册client端的读事件
            // 触发它的关闭流程
            client_connection->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);
    
            return;
        }

        recv_buffer = ssl_connection->getDecryptedBuffer();

    }
    else{
        recv_buffer = &(sever_connection->m_read_buffer);
    }

    int recv_length = recv_buffer->getReadableSize();

    char recv_data[recv_length];

    recv_buffer->getBufferData(recv_data, recv_length);

    HttpsProxyRequest *proxy_buffer = (HttpsProxyRequest*)sever_connection->getUniqueSpace();

    if(!proxy_buffer->isAuthorized()){
        int check_state = checkAuthorized(sever_connection, recv_data, recv_length);
        if(check_state == 1){
            return;
        }
        else if(check_state == -1){

            sever_connection->getEventLoop()->insertTask(
                [=](){
                    if(m_use_ssl){
        
                        m_ssl_connections.erase(server_fd);
                        
                        delete ssl_connection;
            
                    }
                        
                    m_tcpserver->destroyTcpConnection(server_fd);
            
                    proxy->server = nullptr;
            
                    ZekAsyncLogger::LOG_INFO(
                        "server id : %d close, forbid  visit", sever_connection->getConnectId()
                    );
            
                    // 注册client端的读事件
                    // 触发它的关闭流程
                    client_connection->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);
                }
            );

            return;
        }
    }
    
    proxy_buffer->append(recv_data, recv_length);

    int send_length = proxy_buffer->parseHttpRequest();

    int error; 

    socklen_t len = sizeof(error);

    getsockopt(client_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    std::shared_ptr<char[]> real_s(new char[send_length]());

    proxy_buffer->getBufferData(real_s.get(), send_length);


    if(error == EINPROGRESS){

        client_connection->m_write_buffer.append(real_s.get(), send_length);
        
        client_connection->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);

        ZekAsyncLogger::LOG_INFO(
            "server id : %d, client connection preceeding, send data :%d save in buffer"
            , sever_connection->getConnectId(), send_length
        );
        
    }
    else if(error == 0){

        client_connection->send(real_s, send_length);

        ZekAsyncLogger::LOG_INFO(
            "server id : %d, send data :%d"
            , sever_connection->getConnectId(), send_length
        );
    }
    else {
        
        if(m_use_ssl){

            m_ssl_connections.erase(server_fd);
            
            delete ssl_connection;

        }
            
        m_tcpserver->destroyTcpConnection(server_fd);

        proxy->server = nullptr;

        ZekAsyncLogger::LOG_INFO(
            "server id : %d close", sever_connection->getConnectId()
        );

        // 注册client端的读事件
        // 触发它的关闭流程
        client_connection->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);

        return;
    }

}


void HttpsProxy::handleClientConnect(ZekWebServer::TcpConnection *client_connection){
    ZekAsyncLogger::LOG_INFO(
        "client id : %d successful connect", client_connection->getConnectId()
    );
}

void HttpsProxy::handleClientClose(int proxy_id, ZekWebServer::TcpConnection *client_connection){
    ZekAsyncLogger::LOG_INFO(
        "client id : %d close", client_connection->getConnectId()
    );

    ProxyNode *proxy = m_proxys.getProxy(proxy_id);

    std::lock_guard<std::mutex> lock(proxy->mutex);

    proxy->client = nullptr;

    if(!proxy->server){
        m_proxys.deleteProxyNode(proxy_id);
    }
    else{
        // proxy->server不为nullptr
        // 那么就触发它的读事件
        // 开启它的关闭流程
        proxy->server->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);
    }
}


void HttpsProxy::handleClientWrite(int proxy_id, ZekWebServer::TcpConnection *client_connection){
    ProxyNode *proxy = m_proxys.getProxy(proxy_id);

    std::lock_guard<std::mutex> lock(proxy->mutex);

    // 通过这个判断server端的 TcpConnection是否关闭
    // 如果为nullptr，则表示已经关闭
    // 那么就开始触发本连接的关闭流程
    if(!proxy->server){

        ZekAsyncLogger::LOG_INFO(
            "client id : %d close, triggered by server connection", client_connection->getConnectId()
        );
        
        int client_fd = client_connection->getFd();
    
        proxy->client = nullptr;
    
        m_tcpclient->destroyTcpConnection(client_fd);
        
        m_proxys.deleteProxyNode(proxy_id);
    }
    
}


void HttpsProxy::handleClientRead(int proxy_id, ZekWebServer::TcpConnection *client_connection){

    ProxyNode *proxy = m_proxys.getProxy(proxy_id);
    
    std::lock_guard<std::mutex> lock(proxy->mutex);
  
    ZekWebServer::TcpConnection *server_connection = proxy->server;

    int client_fd = client_connection->getFd();

    if(!server_connection){

        m_tcpclient->destroyTcpConnection(client_fd);

        proxy->client = nullptr;

        ZekAsyncLogger::LOG_INFO(
            "client id : %d close", client_connection->getConnectId()
        );

        m_proxys.deleteProxyNode(proxy_id);

        return;
    }

    int server_fd = server_connection->getFd();

    SSLConnection *ssl_connection = nullptr;

    if(m_use_ssl){

        ssl_connection = m_ssl_connections[server_fd];

    }

    int error; 

    socklen_t len = sizeof(error);

    getsockopt(server_fd, SOL_SOCKET, SO_ERROR, &error, &len);

    int length = client_connection->m_read_buffer.getReadableSize();

    std::shared_ptr<char[]> real_s(new char[length]());

    client_connection->m_read_buffer.getBufferData(real_s.get(), length);
    
    if(error == 0){
        
        int rt = 0; 
        
        if(m_use_ssl){

            rt = ssl_connection->encryptData(server_connection, real_s.get(), length);

        }
        else{

            server_connection->send(real_s , length);

        }

        ZekAsyncLogger::LOG_INFO(
            "client id : %d, recv data : %d, decrypt data : %d"
            , client_connection->getConnectId()
            , length
            , rt
        );

    }
    else{

        m_tcpclient->destroyTcpConnection(client_fd);

        proxy->client = nullptr;

        ZekAsyncLogger::LOG_INFO(
            "client id : %d close", client_connection->getConnectId()
        );


        if(proxy->server){
            server_connection->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);
        }
    }

}

void HttpsProxy::start(){

    m_tcpclient->start();

    m_tcpserver->start();
}


void HttpsProxy::stop(){

    m_tcpclient->stop();
    
    m_tcpserver->stop();
}


HttpsProxyRequest::HttpsProxyRequest(int server_id, const std::string &proxy_ip, int proxy_port
, ZekWebServer::SockAdd *source_info)
: m_server_id(server_id)
, m_proxy_ip(proxy_ip)
, m_proxy_port(proxy_port)
{
    // 由于我通过frp连接服务器
    // 所以建立socket连接的 sockaddr_in消息
    // 它的ip地址都是127.0.0.1
    if(!source_info){
        m_source_ip = "8.138.214.123";
        m_source_port = 7777;
    }
    else{
        m_source_ip = source_info->getIP();
        m_source_port = source_info->getPort();
    }

}

int HttpsProxyRequest::parseHead(std::string &src, std::string &dest, size_t start, size_t end, int &read_send){

    std::string source_ip_port = m_source_ip + ":" + std::to_string(m_source_port);

    std::string proxy_ip_port = m_proxy_ip + ":" + std::to_string(m_proxy_port);

    auto search_start = src.begin() + start;

    auto search_end = src.begin() + end;

    if(!m_is_web_socket){

        std::string upgrade_head = "Connection: Upgrade";

        std::string websocket_head = "Upgrade: websocket";

        auto upgrade_head_result = std::search(
            search_start,
            search_end,
            upgrade_head.begin(),
            upgrade_head.end()
        );

        if(upgrade_head_result != search_end){
            auto websocket_head_result = std::search(
                search_start,
                search_end,
                websocket_head.begin(),
                websocket_head.end()
            );
            if(websocket_head_result != search_end){
                m_is_web_socket = true;
            }
        }
    }

    auto search_split = std::search(
        search_start,
        search_end,
        source_ip_port.begin(),
        source_ip_port.end()
    );

    int old_dest_size = dest.size();

    size_t split_start = 0;

    size_t split_end = 0;

    size_t segment_start = start;

    while(search_split != search_end){

        split_start = start + search_split - search_start;

        split_end = split_start + source_ip_port.size();

        dest.append(src, segment_start, split_start - segment_start);
        
        dest += proxy_ip_port;

        segment_start = split_end;

        search_split = std::search(
            search_start + split_end,
            search_end,
            source_ip_port.begin(),
            source_ip_port.end()
        );

    }

    dest.append(src, segment_start, end - segment_start);

    int new_dest_size = dest.size();

    read_send += new_dest_size - old_dest_size;

    std::string content_head = "Content-Length: ";

    auto it_content_head = std::search(
        search_start,
        search_end,
        content_head.begin(),
        content_head.end()
    );

    int length = 0;

    if(it_content_head != search_end){

        size_t length_start = it_content_head - src.begin() + content_head.size();

        size_t length_end = src.find("\r\n", length_start);

        length = std::stoi(
            src.substr(
                length_start, 
                length_end - length_start
            )
        );

    }
    return length;

}


int HttpsProxyRequest::parseHttpRequest(bool replace_ip_and_port){

    int length_original = getReadableSize();

    if(m_is_web_socket) return length_original;

    std::string data_original = getBufferAllData();

    int length_ready_send = 0;

    if(length_original < m_rest_body_length){
        
        append(data_original.c_str(), length_original);
        
        m_rest_body_length -= length_original;

        length_ready_send += length_original;
    }
    else{
        std::string data_change_ip_port;
        
        data_change_ip_port.reserve(length_original);

        data_change_ip_port.append(data_original, 0, m_rest_body_length);
        
        length_ready_send += m_rest_body_length;
        
        size_t start = m_rest_body_length;
        
        size_t end = 0;
        
        m_rest_body_length = 0;

        int length_content = 0;

        while(1){
            
            end = data_original.find("\r\n\r\n", start);

            if(end != std::string::npos){
                length_content = parseHead(data_original, data_change_ip_port, start, end, length_ready_send);

                data_change_ip_port += "\r\n\r\n";

                length_ready_send += 4;

                end += 4;

                start = end ;
            }
            else{
                data_change_ip_port.append(data_original, start);

                length_ready_send += length_original - start;

                break;
            }

            if(length_content > 0){
                if(length_original - end > length_content){
                    data_change_ip_port.append(data_original, end, length_content);

                    m_rest_body_length = 0;

                    start += length_content;

                    length_ready_send += length_content;

                }
                else{
                    data_change_ip_port.append(data_original, end, length_original - end);

                    m_rest_body_length = length_content - (length_original - end);

                    length_ready_send += length_original - end;

                    break;
                }
            }


        }
        append(data_change_ip_port.c_str(), data_change_ip_port.size());

    }

    return length_ready_send;


}


}