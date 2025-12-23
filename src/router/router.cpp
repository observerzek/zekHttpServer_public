#include <include/router/router.h>


namespace ZekHttpServer
{

Router::RouterKey::RouterKey(HttpRequest::Method _method, const std::string &_path)
: method(_method)
, path(_path)
{
}

bool Router::RouterKey::operator==(const RouterKey &other) const{
    return method == other.method && path == other.path;
}

size_t Router::RouterKeyHash::operator()(const RouterKey &key) const{
    size_t hash_method = std::hash<int>{}(key.method);

    size_t hash_path = std::hash<std::string>{}(key.path);

    // return hash_method * 31 + hash_path;
    return (hash_path << 16) + hash_method;
}


void Router::registerKeyCallBack(HttpRequest::Method method, const std::string &path, RouterCallBack cb){
    RouterKey key(method, path);
    m_key_CBs[key] = cb;
}


void Router::registerKeyFunCallBack(HttpRequest::Method method, const std::string &path, AbstractRouterCallBack *cb){
    RouterKey key(method, path);

    m_key_Fun_CBs[key] = cb;
}

void Router::registerDynamicKeyFunCallBack(HttpRequest::Method method, const std::string &path, AbstractRouterCallBack *cb){
    RouterKey key(method, path);

    m_dynamic_key_Fun_CBs[key] = cb;
}



bool Router::route(HttpRequest *req, HttpResponse *resp){
    RouterKey key(req->getMethod(), req->getPath());

    auto it = m_key_CBs.find(key);

    if(it != m_key_CBs.end()){
        RouterCallBack cb = it->second;
        cb(req, resp);
        return true;
    }

    auto it_fun = m_key_Fun_CBs.find(key);

    if(it_fun != m_key_Fun_CBs.end()){
        AbstractRouterCallBack *fun_cb = it_fun->second;
        fun_cb->forward(req, resp);
        return true;
    }

    for(auto &it : m_dynamic_key_Fun_CBs){
        
        // 这种动态路由
        // 一般对应的path是 /download*
        // 因此只需要去*前的内容去匹配就好
        // 比如 /download/file.txt
        // 就能匹配到这个动态路由
        std::string path = it.first.path;
        std::string dynamic_key = path.substr(0, path.size() - 1);

        size_t result = key.path.find(dynamic_key);

        if(result != std::string::npos){
            AbstractRouterCallBack *fun_cb = it.second;
            fun_cb->forward(req, resp);
            return true;
        }

    }

    return false;
}


} // namespace ZekHttpServer