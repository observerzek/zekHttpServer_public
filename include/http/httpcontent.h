#pragma once

#include <include/http/httprequest.h>
#include <include/utils/utility.h>
#include <lib/zekWebServer/include/buffer.h>


namespace ZekHttpServer
{



class HttpContent{
public:

    #ifdef OVERLOAD_NEW_AND_DELETE

    void* operator new(size_t bytes){
        return MemoryPool::allocate(bytes);
    }


    void operator delete(void *ptr){
        MemoryPool::deallocate(ptr, sizeof(HttpContent));
    }

    #endif

    using Buffer = ZekWebServer::Buffer;

    enum HttpRequestParseState{
        ParseRequestLine,
        ParseHeader,
        ParseBody,
        Complete
    };

private:
    HttpRequestParseState m_parse_state;
    HttpRequest m_request;

private:
    bool parseRequestLine(const std::string &data);

    bool parseRequestHeaders(const std::string &data);

    bool parseRequestBody(Buffer *buffer, int body_len);

public:
    HttpContent();

    bool parseRequest(Buffer *buffer, size_t receive_time = 0);

    HttpRequestParseState getParseState(){return m_parse_state;}

    HttpRequest* getHttpRequest(){return &m_request;}

    void reset();

    bool isComplete(){return m_parse_state == Complete;}

};





} // namespace ZekHttpServer
