#include <include/utils/utility.h>
#include <functional>
#include <unordered_map>
#include <include/http/httprequest.h>
#include <include/http/httpresponse.h>
#include "./routercallback.h"

namespace ZekHttpServer
{

class Router{
public:
    #ifdef OVERLOAD_NEW_AND_DELETE

    void* operator new(size_t bytes){
    return MemoryPool::allocate(bytes);
    }

    void operator delete(void *ptr){
    MemoryPool::deallocate(ptr, sizeof(Router));
    }
    
    #endif


    struct RouterKey{
        
        HttpRequest::Method method;
        
        std::string path;

        RouterKey(HttpRequest::Method _method, const std::string &_path);

        bool operator==(const RouterKey &other) const;
    };

    struct RouterKeyHash{
        size_t operator()(const RouterKey &key) const;
    };

    using RouterCallBack = std::function<void (HttpRequest *, HttpResponse *)>;
    using RouterKeyCB = std::unordered_map<RouterKey, RouterCallBack, RouterKeyHash>;

    using RouterKeyFunCB = std::unordered_map<RouterKey, AbstractRouterCallBack *, RouterKeyHash>;

private:
    RouterKeyCB m_key_CBs; 

    RouterKeyFunCB m_key_Fun_CBs;

    RouterKeyFunCB m_dynamic_key_Fun_CBs;

public:
    void registerKeyCallBack(HttpRequest::Method method, const std::string &path, RouterCallBack cb);

    void registerKeyFunCallBack(HttpRequest::Method method, const std::string &path, AbstractRouterCallBack *cb);

    void registerDynamicKeyFunCallBack(HttpRequest::Method method, const std::string &path, AbstractRouterCallBack *cb);

    // 如果形参的某个类
    // 用const去限制它
    // 那么在这个函数内
    // 只能调用类的那些用const修饰过的成员函数
    bool route(HttpRequest *req, HttpResponse *resp);
};



} // namespace ZekHttpServer