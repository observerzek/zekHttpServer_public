#include <mysql/mysql.h>
#include <string>
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
#include <unistd.h>
#include <thread>

using std::cout;
using std::endl;

std::mt19937 gen_id_random;
std::random_device rd;


std::string HOST = "localhost";
std::string USER = "zek";
std::string PASSWORD = "201310599";
std::string DATABASE = "http_server";
size_t PORT = 3306;


std::string generateID(){
    std::stringstream id;

    std::uniform_int_distribution<> dist(0,15);

    for(int i = 0; i < 32; i++){
        int t = gen_id_random();
        // std::hex 将输出格式设置为十六进制
        // dist()里的参数需要输入 std::mt19937类型的随机数
        id << std::hex << dist(gen_id_random);
    }

    return id.str();
}

void printSelectResult(MYSQL *mysql){

    MYSQL_RES *result = mysql_store_result(mysql);

    int rows = mysql_num_rows(result);

    int columns = mysql_num_fields(result);

    MYSQL_FIELD *column_data = mysql_fetch_field(result);

    MYSQL_ROW row;

    cout << "-------------" << endl;

    for(int i = 0; i < rows; i++){
        row = mysql_fetch_row(result);
        for(int j = 0; j < columns; j++){
            cout << column_data[j].name << " : " << row[j] << endl;
        }
    }

    cout << "-------------" << endl << endl;

    mysql_free_result(result);

}



void mysqlTest(){
    MYSQL *mysql = mysql_init(nullptr);


    if(
        !mysql_real_connect(
            mysql,
            HOST.c_str(),
            USER.c_str(),
            PASSWORD.c_str(),
            DATABASE.c_str(),
            PORT,
            nullptr,
            0
    )
    ){
        cout << "fail to connect MySQL" << endl;
    }


    std::string select_sql = "SELECT * FROM cookie";

    mysql_query(mysql, select_sql.c_str());


    printSelectResult(mysql);

    std::string cookie_value = generateID();
    
    std::string insert_sql = "INSERT INTO cookie (cookie) VALUE('" + cookie_value + "')";

    mysql_query(mysql, insert_sql.c_str());

    mysql_query(mysql, select_sql.c_str());

    printSelectResult(mysql);

    mysql_query(mysql, "SELECT * FROM cookie WHERE cookie = 'c0a1986348462c02741b2fdfd8f022ac'");

    printSelectResult(mysql);

    mysql_close(mysql);

}


void mysqlThreadPoolTest(){
    MYSQL *mysql = mysql_init(nullptr);


    if(
        !mysql_real_connect(
            mysql,
            HOST.c_str(),
            USER.c_str(),
            PASSWORD.c_str(),
            DATABASE.c_str(),
            PORT,
            nullptr,
            0
    )
    ){
        cout << "fail to connect MySQL" << endl;
    }

    for(int i = 0; i < 5; i++){

        std::string cookie_value = generateID();
        
        std::string insert_sql = "INSERT INTO cookie (cookie) VALUE('" + cookie_value + "')";
    
        mysql_query(mysql, insert_sql.c_str());

        sleep(1);
    }


    mysql_close(mysql);

}


int main(){
    gen_id_random.seed(rd());

    // mysqlTest();

    std::vector<std::thread> threads;
    threads.reserve(4);

    for(int i = 0; i < 4;i++){
        threads.emplace_back(mysqlThreadPoolTest);
    }

    for(int i = 0; i < 4; i++){
        if(threads[i].joinable()){
            threads[i].join();
        }
    }

    return 0;
}