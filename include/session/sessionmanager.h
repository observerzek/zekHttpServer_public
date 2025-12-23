#pragma once
#include <include/utils/utility.h>
#include <include/mysql/mysqlpool.h>
#include "./session.h"
#include <fstream>
#include <random>
#include <shared_mutex>
#include <lib/zekCache/include/cache.hpp>

namespace ZekHttpServer
{

class SessionManager{
    
private:

    ZekCache::HashCache<std::string, Session *> m_sessions_caches;

    // 生成随机数
    std::mt19937 m_gen_id_random;

    // 存储sessions
    MysqlConnectionPool *m_mysql_pool;


private:

    std::string generateID();

    bool findInMysql(const std::string &id);

    bool insertIntoMysql(const std::string &id);

    Session* reloadSession(const std::string &id);

    Session* generateSession();

public:

    SessionManager(const std::string &file_name = "");

    ~SessionManager();

    // 同一个Session可能被多个线程同时拥有
    // 可能会有线程安全问题
    Session* findSession(const std::string &id);

    void removeSession(const std::string &id);


};


} // namespace ZekHttpServer