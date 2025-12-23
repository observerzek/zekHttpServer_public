#pragma once
#include <include/utils/utility.h>
#include <string>
#include <unordered_map>
#include <lib/zekWebServer/include/tcpConnection.h>
#include <include/tls/sslConnection.h>

namespace ZekHttpServer
{



class HttpRequest
{
public:
    #ifdef OVERLOAD_NEW_AND_DELETE

    void* operator new(size_t bytes){
        return MemoryPool::allocate(bytes);
    }


    void operator delete(void *ptr){
        MemoryPool::deallocate(ptr, sizeof(HttpRequest));
    }

    #endif

    enum Method{
        Invalid,
        Get,
        Post,
        Head,
        Put,
        Delete,
        Options
    };

    using KeyValueMap = std::unordered_map<std::string, std::string>;

private:
    Method m_method;

    ZekWebServer::TcpConnection *m_tcp_connection;

    SSLConnection *m_ssl_connection;

    std::string m_version;
    
    std::string m_path;

    KeyValueMap m_query_parameters;

    KeyValueMap m_request_headers;

    size_t m_receive_time;

    char *m_content;

    size_t m_content_capacity;

    size_t m_content_len;

private:

public:

    HttpRequest();

    ~HttpRequest();

    void setTcpConnection(ZekWebServer::TcpConnection *tcp_connection){
        m_tcp_connection = tcp_connection;
    }

    ZekWebServer::TcpConnection* getTcpConnection(){
        return m_tcp_connection;
    }

    void setSslConnection(SSLConnection *ssl_connection){
        m_ssl_connection = ssl_connection;
    }

    SSLConnection* getSslConnection(){
        return m_ssl_connection;
    }

    // void setMethod(Method method){
    //     m_method = method;
    // }
    void setMethod(const std::string &method);
    Method getMethod() const{return m_method;}

    void setVersion(const char *version){
        m_version = version;
    }
    std::string getVersion(){return m_version;}
    
    void setPath(const char *path){
        m_path = path;
    }
    std::string getPath() const{return m_path;}

    void addQuery(const std::string &key, const std::string &value){
        m_query_parameters[key] = value;
    }
    std::string getQueryValue(const std::string &key);

    void addHeader(const std::string &key, const std::string &value){
        m_request_headers[key] = value;
    }
    std::string getHeaderValue(const std::string &key) const;

    void setReceiveTime(size_t receive_time = 0);

    size_t getReceiveTime(){return m_receive_time;}

    void setContent(const char *content, size_t len);
    char* getContent(){return m_content;}

    size_t getContentLen(){return m_content_len;}

    void reset();
};




} // namespace ZekHttpServer