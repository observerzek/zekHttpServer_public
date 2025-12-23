#pragma once
#include <include/utils/utility.h>
#include <string>
#include <shared_mutex>


namespace ZekHttpServer
{

class Session{
public:
    #ifdef OVERLOAD_NEW_AND_DELETE

    void* operator new(size_t bytes){
        return MemoryPool::allocate(bytes);
    }

    void operator delete(void *ptr){
        MemoryPool::deallocate(ptr, sizeof(Session));
    }

    #endif

private:
    // 对于会话而言的唯一标识符
    std::string m_id;

    // 绝对过期时间
    // 单位是秒
    size_t m_absolute_expired_time;

    // 设定的过期时间
    // 单位是秒
    size_t m_expired_time;

    // http响应里Set-Cookie响应头的例子如下
    // Set-Cookie: <cookie-name>=<cookie-value>[; 属性1=值1][; 属性2=值2]...
    // 这些属性值发送给浏览器
    // 影响浏览器使用cookie的策略
    std::unordered_map<std::string, std::string> m_cookie_attributes;

    std::string m_cookie_head_value;

    bool m_is_updated;

    // 每个会话拥有的独立的东西
    // 方便程序能基于会话，执行相应内容
    std::unordered_map<std::string, void *> m_datas;

public:

    // 一个会话可能会被多个线程持有
    std::shared_mutex m_shared_mutex;

private:

    // void 

public:

    Session(const std::string &id, size_t expired_time = 0);

    ~Session();
    
    std::string getId(){return m_id;}

    void setId(const std::string &id){m_id = id;}

    void setCookieAttribute(const std::string &key, const std::string &value);

    std::string getCookieAttribute(const std::string &key);

    void* setData(const std::string &key, int len);

    void* getData(const std::string &key);

    void removeData(const std::string &key);

    bool isExipred();

    void refresh();

    std::string returnCookieHeadValue();

};



} // namespace ZekHttpServer
