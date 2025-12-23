#include <include/session/session.h>



namespace ZekHttpServer
{

Session::Session(const std::string &id, size_t expired_time)
: m_id(id)
, m_is_updated(false)
, m_expired_time(expired_time)
{
    m_cookie_head_value.reserve(32);
    refresh();
    m_cookie_attributes["create_time"] = ZekAsyncLogger::getNowTime();

}

Session::~Session(){
    while(!m_datas.empty()){
        auto it = m_datas.begin();

        removeData(it->first);

    }
}

void Session::setCookieAttribute(const std::string &key, const std::string &value){
    m_cookie_attributes[key] = value;
}

std::string Session::getCookieAttribute(const std::string &key){
    auto it = m_cookie_attributes.find(key);
    std::string result;

    if(it != m_cookie_attributes.end()){
        result = m_cookie_attributes[key];
    }

    return result;
}

std::string Session::returnCookieHeadValue(){
    std::string cookie_value;

    cookie_value.reserve(64);

    cookie_value += "session_id="+ m_id;
    cookie_value += "; ";
    cookie_value += "Max-Age=" + std::to_string(m_expired_time);

    auto it = m_cookie_attributes.begin();

    while(it != m_cookie_attributes.end()){
        cookie_value += "; ";
        cookie_value += it->first + "=" + it->second;
        it++;
    }

    return cookie_value;
}


bool Session::isExipred(){
    size_t now = ZekAsyncLogger::getSecondFromEpoch();
    return now >= m_absolute_expired_time ? true : false;
}

void Session::refresh(){
    m_absolute_expired_time = ZekAsyncLogger::getSecondFromEpoch() + m_expired_time;
}


void* Session::setData(const std::string &key, int len){
    // std::lock_guard<std::shared_mutex> lock(m_shared_mutex);

    auto it = m_datas.find(key);

    void *ptr = nullptr;

    if(it == m_datas.end()){
   
        ptr = (void*)new char[len]();

        memset(ptr, 0, len);
        
        m_datas[key] = (void *)ptr;
    }
    else{
        ptr = m_datas[key];
    }

    return ptr;

}

void* Session::getData(const std::string &key){
    // std::shared_lock<std::shared_mutex> lock(m_shared_mutex);

    auto it = m_datas.find(key);

    void *result = nullptr;

    if(it != m_datas.end()){
        result = it->second;
    }

    return result;
}

void Session::removeData(const std::string &key){
    // std::lock_guard<std::shared_mutex> lock(m_shared_mutex);

    auto it = m_datas.find(key);

    if(it != m_datas.end()){

        delete [] (char*)(it->second);

        m_datas.erase(it);
    }
}

} // namespace ZekHttpServer