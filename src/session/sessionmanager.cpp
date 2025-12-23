#include <include/session/sessionmanager.h>
#include <filesystem>
#include <sstream>
#include <assert.h>

namespace ZekHttpServer
{

SessionManager::SessionManager(const std::string &file_name)
: m_gen_id_random(std::random_device{}())
, m_sessions_caches(2)
{
    MysqlConnectionPool::InitilizeMysqlConnectionPool(
        2,
        "localhost",
        "zek",
        "201310599",
        "http_server",
        3306
    );

    m_mysql_pool = MysqlConnectionPool::GetMysqlConnectionPool();
    
}   

SessionManager::~SessionManager(){
    std::vector<std::string> ids = m_sessions_caches.getAllKeys();

    for(auto &id : ids){
        removeSession(id);
    }
    
}

std::string SessionManager::generateID(){
    std::stringstream id;

    std::uniform_int_distribution<> dist(0,15);

    for(int i = 0; i < 32; i++){
        int t = m_gen_id_random();
        // std::hex 将输出格式设置为十六进制
        // dist()里的参数需要输入 std::mt19937类型的随机数
        id << std::hex << dist(m_gen_id_random);
    }

    return id.str();
}

bool SessionManager::findInMysql(const std::string &id){
    
    std::string sql = "SELECT cookie FROM cookie WHERE cookie = '" + id + "'";

    auto mysql = m_mysql_pool->getMysqlConnection();

    std::vector<std::string> result = mysql->findData(sql);

    if(result.size() > 0){

        assert(result.size() == 1);

        assert(result[0] == id);
        
        return true;
    }
    else{
        return false;
    }
    
}

Session* SessionManager::findSession(const std::string &id){
    Session *result = nullptr;

    result = m_sessions_caches.get(id);

    // 在cache里找
    if(result == nullptr){
        // 在mysql里找
        if(findInMysql(id)){
            result = reloadSession(id);
        }
        // 都没有，生成一个新的session
        else {
            result = generateSession();
        }
    }

    return result;
}


bool SessionManager::insertIntoMysql(const std::string &id){
    
    std::string sql = "INSERT INTO cookie (cookie) VALUES ('" + id + "')";

    auto mysql = m_mysql_pool->getMysqlConnection();

    return mysql->doSQL(sql);

}

void SessionManager::removeSession(const std::string &id){


    Session *session =  m_sessions_caches.get(id);

    if(session){
        m_sessions_caches.remove(id);
        delete session;
        ZekAsyncLogger::LOG_DEBUG(
            "delete session id :%s", id.c_str()
        );
    }

}

Session* SessionManager::reloadSession(const std::string &id){
    Session *session = new Session(id, 60 * 60 * 24);
    session->setCookieAttribute("name", "zek");
    session->setCookieAttribute("Path", "/");

    assert(m_sessions_caches.get(id) == nullptr);

    m_sessions_caches.put(id, session);

    return session;
}


Session* SessionManager::generateSession(){
    std::string id = generateID();
    Session *session = new Session(id, 60 * 60 * 24);
    session->setCookieAttribute("name", "zek");
    session->setCookieAttribute("Path", "/");

    if(m_sessions_caches.get(id)){
        id = generateID();
        session->setId(id);
    }

    m_sessions_caches.put(id, session);

    insertIntoMysql(id);

    return session;
}


} // namespace ZekHttpServer