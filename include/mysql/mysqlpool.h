#pragma once
#include "./mysqlconnection.h"
#include <include/utils/utility.h>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace ZekHttpServer
{

class MysqlConnectionPool;

static MysqlConnectionPool *mysql_pool = nullptr;


class MysqlConnectionDeleter{

private:
    MysqlConnectionPool *m_connection_pool;

public:
    MysqlConnectionDeleter(MysqlConnectionPool *connection_pool)
    : m_connection_pool(connection_pool)
    {}

    void operator() (MysqlConnection *ptr);

};

using MysqlUniquePtr = std::unique_ptr<MysqlConnection, MysqlConnectionDeleter>;


class MysqlConnectionPool{

public:

    friend class MysqlConnectionDeleter;
    
private:
    int m_mysql_connection_count;

    int m_mysql_connection_connected_count;

    std::string m_host;

    std::string m_user;

    std::string m_password;

    std::string m_database_name;

    int m_port;

    bool m_is_end;

    std::queue<MysqlUniquePtr> m_connections;

    std::mutex m_mutex;

    std::condition_variable m_condition_variable;

    std::thread m_check_connected_thread;

private:

    MysqlConnectionPool(
        int connection_count,
        const std::string &host,
        const std::string &user,
        const std::string &password,
        const std::string &database_name,
        int port = 3306
    );

    ~MysqlConnectionPool();
    
    bool requireBuildMysqlConnection();

public:

    static void InitilizeMysqlConnectionPool(
        int connection_count,
        const std::string &host,
        const std::string &user,
        const std::string &password,
        const std::string &database_name,
        int port = 3306
    );

    static MysqlConnectionPool* GetMysqlConnectionPool();

    MysqlUniquePtr getMysqlConnection();

    void checkThreadMainFuntion();

};


    
} // namespace ZekHttpServer