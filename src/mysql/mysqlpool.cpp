#include <include/mysql/mysqlpool.h>
#include <assert.h>
#include <unistd.h>
#include <iostream>

namespace ZekHttpServer
{


MysqlConnectionPool::MysqlConnectionPool(
    int connection_count,
    const std::string &host,
    const std::string &user,
    const std::string &password,
    const std::string &database_name,
    int port
)
: m_mysql_connection_count(connection_count)
, m_host(host)
, m_user(user)
, m_password(password)
, m_database_name(database_name)
, m_port(port)
{
    m_is_end = false;

    m_mysql_connection_connected_count = 0; 

    m_check_connected_thread = std::thread(
        &MysqlConnectionPool::checkThreadMainFuntion, this
    );


    
}

MysqlConnectionPool::~MysqlConnectionPool(){

    m_is_end = true;

    while(!m_connections.empty()){
        m_connections.pop();
    }

}

void MysqlConnectionPool::InitilizeMysqlConnectionPool(
    int connection_count,
    const std::string &host,
    const std::string &user,
    const std::string &password,
    const std::string &database_name,
    int port
){
    assert(mysql_pool == nullptr);
    mysql_pool = new MysqlConnectionPool(
        connection_count,
        host,
        user,
        password,
        database_name,
        port
    );

    if(mysql_pool == nullptr){
        ZekAsyncLogger::LOG_ERROR(
            "fail to create MysqlConnectionPool"
        );
    }
    else{
        ZekAsyncLogger::LOG_INFO(
            "success to create MysqlConnectionPool"
        );
    }
}

MysqlConnectionPool* MysqlConnectionPool::GetMysqlConnectionPool(){
    return mysql_pool;
}


void MysqlConnectionDeleter::operator() (MysqlConnection *ptr){
    if(ptr){
        std::unique_lock<std::mutex> lock(m_connection_pool->m_mutex);

        if(!m_connection_pool->m_is_end){
            
            m_connection_pool->m_connections.push(
                MysqlUniquePtr(
                    ptr, MysqlConnectionDeleter(m_connection_pool)
                )
            );
            m_connection_pool->m_condition_variable.notify_one();

        }
        else{
            delete ptr;
        }

    }
}

bool MysqlConnectionPool::requireBuildMysqlConnection(){
    if(m_mysql_connection_connected_count == m_mysql_connection_count) return false;

    MysqlUniquePtr connection(
        new MysqlConnection(
            m_host,
            m_user,
            m_password,
            m_database_name,
            m_port
        ),
        MysqlConnectionDeleter(this)
    );

    m_connections.push(std::move(connection));

    m_mysql_connection_connected_count++;


    return true;
}

MysqlUniquePtr MysqlConnectionPool::getMysqlConnection(){

    MysqlUniquePtr connection(nullptr, MysqlConnectionDeleter(this));

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        requireBuildMysqlConnection();

        while(m_connections.empty()){
            m_condition_variable.wait(lock);
        }

        connection = std::move(m_connections.front());

        m_connections.pop();
    
    }

    return std::move(connection);
}


void MysqlConnectionPool::checkThreadMainFuntion(){

    char main_thread_name[16];

    pthread_getname_np (pthread_self(), main_thread_name, 16);

    std::string thread_name = main_thread_name;
    // thread_name += "_check_mysql";

    thread_name += "_mysql";

    pthread_setname_np(pthread_self(), thread_name.c_str());


    while(!m_is_end){
        
        int count = m_connections.size();

        std::queue<MysqlUniquePtr> connections;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            connections.swap(m_connections);
        }
        while(!connections.empty()){
            MysqlUniquePtr connection = std::move(connections.front());
            connections.pop();

            if(!connection->isConnected()){
                connection->reConnect();
            }

        }

        sleep(10);

    }
    
}

} // namespace ZekHttpServer