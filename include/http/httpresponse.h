#pragma once
#include <include/utils/utility.h>
#include <unordered_map>

namespace ZekHttpServer
{


class HttpResponse{
public:
    #ifdef OVERLOAD_NEW_AND_DELETE

    void* operator new(size_t bytes){
    return MemoryPool::allocate(bytes);
    }


    void operator delete(void *ptr){
    MemoryPool::deallocate(ptr, sizeof(HttpResponse));
    }

    #endif

    enum StateCode{
        Code_000_UnKnow = 0,
        Code_200_Ok = 200,
        Code_204_NoContent = 204,
        Code_206_PartialContent = 206,
        Code_400_BadRequest = 400,
        Code_401_Unauthorized = 401,
        Code_403_Forbidden = 403,
        Code_404_NotFound = 404,
        Code_500_InternalError = 500
    };


    using KeyValueMap = std::unordered_map<std::string, std::string>;

private:
    std::string m_version;

    StateCode m_state_code;

    std::string m_state_message;

    KeyValueMap m_response_headers;

    size_t m_send_time;

    char *m_content;

    size_t m_content_capacity;

    size_t m_content_len;

private:

public:

    HttpResponse();

    ~HttpResponse();

    void setVersion(const char* version){m_version = version;}

    std::string getVersion(){return m_version;}

    void setStateCode(StateCode code){m_state_code = code;}

    int getStateCode(){return m_state_code;}

    void setStateMessage(const char* message){m_state_message = message;}

    std::string getStateMessage(){return m_state_message;}

    void addHeader(const std::string &key, const std::string &value){
        m_response_headers[key] = value;
    }

    std::string getHeaderValue(const std::string &key);

    void setSendTime(size_t send_time = 0);

    size_t getSendTime(){return m_send_time;}

    void setContent(const char *data, size_t len);

    char* getContent(){return m_content;}

    void reset();

    std::string createResponse();

};


} // namespace ZekHttpServer