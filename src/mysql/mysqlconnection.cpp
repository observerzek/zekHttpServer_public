#include <include/mysql/mysqlconnection.h>


namespace ZekHttpServer
{

MysqlConnection::MysqlConnection(
const std::string &host,
const std::string &user,
const std::string &password,
const std::string &database_name,
int port
)
: m_host(host)
, m_user(user)
, m_password(password)
, m_database_name(database_name)
, m_port(port)
{
    m_mysql = mysql_init(nullptr);

    int op = 1;

    // 设置成执行mysql_ping()函数时，若断开会自动重连
    // mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &op);

    mysql_real_connect(
        m_mysql,
        host.c_str(),
        user.c_str(),
        password.c_str(),
        database_name.c_str(),
        port,
        nullptr,
        0
    );

}

MysqlConnection::~MysqlConnection(){
    if(m_mysql){
        mysql_close(m_mysql);
    }
}

bool MysqlConnection::isConnected(){
    return mysql_ping(m_mysql) == 0 ? true : false;
}

bool MysqlConnection::reConnect(){

    if(m_mysql){
        mysql_close(m_mysql);
    }

    mysql_real_connect(
        m_mysql,
        m_host.c_str(),
        m_user.c_str(),
        m_password.c_str(),
        m_database_name.c_str(),
        m_port,
        nullptr,
        0
    );

    return isConnected();
}

bool MysqlConnection::doSQL(const std::string &sql){
    return mysql_query(m_mysql, sql.c_str()) == 0 ? true : false;
}

std::vector<std::string> MysqlConnection::findData(const std::string &sql){

    if(!doSQL(sql)) return std::vector<std::string>();

    MYSQL_RES *result = mysql_store_result(m_mysql);

    int rows = mysql_num_rows(result);

    int columns = mysql_num_fields(result);

    MYSQL_ROW row;

    std::vector<std::string> data(rows * columns, "");

    for(int i = 0; i < rows; i++){
        row = mysql_fetch_row(result);
        for(int j = 0; j < columns; j++){
            data[i * columns + j] = row[j];
        }
    }

    mysql_free_result(result);

    return std::move(data);
}



} // namespace ZekHttpServer