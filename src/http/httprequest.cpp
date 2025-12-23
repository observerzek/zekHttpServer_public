#include <include/http/httprequest.h>
#include <string.h>

namespace ZekHttpServer
{

HttpRequest::HttpRequest()
: m_method(Method::Invalid)
, m_version("None")
, m_receive_time(0)
, m_content(nullptr)
, m_content_capacity(0)
, m_content_len(0)
, m_tcp_connection(nullptr)
, m_ssl_connection(nullptr)
{

}

HttpRequest::~HttpRequest(){
    if(m_content){
        MemoryPool::deallocate(m_content, m_content_capacity);
    }
}

void HttpRequest::setMethod(const std::string &method){

    if(method == "GET"){
        m_method = Get;
    }
    else if(method == "POST"){
        m_method = Post;
    }
    else if(method == "HEAD"){
        m_method = Head;
    }
    else if(method == "PUT"){
        m_method = Put;
    }
    else if(method == "DELETE"){
        m_method = Delete;
    }
    else if(method == "OPTIONS"){
        m_method = Options;
    }
    else{
        m_method = Invalid;
    }
}


std::string HttpRequest::getQueryValue(const std::string &key){
    auto it = m_query_parameters.find(key);
    std::string value;
    if(it != m_query_parameters.end()){
        value = it->second;
    }
    return value;
}

std::string HttpRequest::getHeaderValue(const std::string &key) const{
    auto it = m_request_headers.find(key);
    std::string value;
    if(it != m_request_headers.end()){
        value = it->second;
    }
    return value;
}

void HttpRequest::setReceiveTime(size_t receive_time){
    if(receive_time > 0){
        m_receive_time = receive_time;
    }
    else{
        m_receive_time = ZekAsyncLogger::getMillisecondeFromEpoch();
    }
}

void HttpRequest::setContent(const char *content, size_t len){
    if(m_content_capacity < len){
        if(m_content){
            MemoryPool::deallocate(m_content, m_content_capacity);
        }
        m_content = (char*)MemoryPool::allocate(len + 1);
        m_content_capacity = len + 1;
    }
    memset(m_content, 0, m_content_capacity);
    memcpy(m_content, content, len);
    m_content_len = len;
    
    char *end_postion = m_content + len + 1;
    end_postion = 0;
}

void HttpRequest::reset(){
    m_method = Invalid;

    m_version = "None";

    m_path.clear();

    m_query_parameters.clear();

    m_request_headers.clear();

    m_receive_time = 0;

    if(m_content){
        MemoryPool::deallocate(m_content, m_content_capacity);
        m_content = nullptr;
    }

    m_tcp_connection = nullptr;

    m_ssl_connection = nullptr;

    m_content_capacity = 0;

    m_content_len = 0;
}


} // namespace ZekHttpServer
