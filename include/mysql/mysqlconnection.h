#pragma once
#include <mysql/mysql.h>
// #include <include/utils/utility.h>
#include <vector>
#include <string>

namespace ZekHttpServer
{

class MysqlConnection{

private:

    MYSQL *m_mysql;

    std::string m_host;

    std::string m_user;

    std::string m_password;

    std::string m_database_name;

    int m_port;

public:

    MysqlConnection(
        const std::string &host,
        const std::string &user,
        const std::string &password,
        const std::string &database_name,
        int port = 3306
    );

    ~MysqlConnection();

    bool isConnected();

    bool reConnect();

    bool doSQL(const std::string &sql);

    std::vector<std::string> findData(const std::string &sql);

};



} // namespace ZekHttpServer