#include <include/http/httpresponse.h>


namespace ZekHttpServer
{

HttpResponse::HttpResponse()
: m_version("None")
, m_state_code(Code_000_UnKnow)
, m_send_time(0)
, m_content(nullptr)
, m_content_capacity(0)
, m_content_len(0)
{

}

HttpResponse::~HttpResponse(){
    if(m_content){
        #ifdef OVERLOAD_NEW_AND_DELETE
        MemoryPool::deallocate(m_content, m_content_capacity);
        #else
        free(m_content);
        #endif
    }
}

void HttpResponse::setSendTime(size_t send_time){
    if(send_time = 0){
        m_send_time = ZekAsyncLogger::getMillisecondeFromEpoch();
    }
    else{
        m_send_time = send_time;
    }
}

void HttpResponse::setContent(const char *data, size_t len){
    bool re_allocate = false;
    if(m_content){
        if(m_content_capacity - 1 < len){
            #ifdef OVERLOAD_NEW_AND_DELETE
            MemoryPool::deallocate(m_content, m_content_capacity);
            #else
            free(m_content);
            #endif
            m_content = nullptr;
            m_content_capacity = 0;
            m_content_len = 0;
            re_allocate = true;
        }
    }
    else{
        re_allocate = true;
    }

    if(re_allocate){
        #ifdef OVERLOAD_NEW_AND_DELETE
        m_content = (char*)MemoryPool::allocate(len + 1);
        #else
        m_content = (char*)malloc(len + 1);
        #endif
        m_content_capacity = len + 1;
    }
    memcpy(m_content, data, len);
    m_content_len = len;
}


void HttpResponse::reset(){
    m_version.clear();

    m_state_code = Code_000_UnKnow;

    m_state_message.clear();

    m_response_headers.clear();

    m_send_time = 0;

    if(m_content){
        #ifdef OVERLOAD_NEW_AND_DELETE
        MemoryPool::deallocate(m_content, m_content_capacity);
        #else
        free(m_content);
        #endif
    }

    m_content_capacity = 0;

    m_content_len = 0;
}

std::string HttpResponse::createResponse(){
    std::string response;
    response.reserve(1024);

    response += m_version + ' ';
    response += std::to_string(m_state_code) + ' ';
    response += m_state_message + "\r\n";

    for(auto &it : m_response_headers){
        response += it.first + ": " + it.second + "\r\n";
    }
    response += "\r\n";

    if(m_content){
        response += std::string(m_content, m_content_len);
    }
    return response;
}



} // namespace ZekHttpServer