#include <include/mysql/mysqlpool.h>
#include <string>
#include <iostream>
#include <sstream>
#include <random>
#include <vector>
#include <unistd.h>
#include <thread>
#include <mutex>

using namespace ZekHttpServer;

using std::cout;
using std::endl;


std::mt19937 gen_id_random;
std::random_device rd;
std::mutex mutex;

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

void testMysqlConnectionPool(){

    for(int i = 0; i < 4; i++){

        sleep(1);

        auto pool = MysqlConnectionPool::GetMysqlConnectionPool();

        auto mysql = pool->getMysqlConnection();
        
        if(!mysql->isConnected()){
            std::lock_guard<std::mutex> lock(mutex);
            cout << "mysql fail to connect" << endl;
            continue;
        }

        std::string id = generateID();
        
        std::string sql = "INSERT INTO cookie (cookie) VALUES ('" + id + "')";
        
        bool result = mysql->doSQL(sql);

        {
            std::lock_guard<std::mutex> lock(mutex);

            if(result){
                cout << "insert cookie : " << id << endl;
            }
            else{
                cout << "fail to insert cookie" << endl;
            }
        }

        
    }
}


int main(){
    
    gen_id_random.seed(rd());

    MysqlConnectionPool::InitilizeMysqlConnectionPool(
        2,
        "localhost",
        "zek",
        "201310599",
        "http_server",
        3306
    );

    std::vector<std::thread> threads;

    threads.reserve(8);

    for(int i = 0; i < 4; i++){
        threads.emplace_back(
            std::thread(testMysqlConnectionPool)
        );
    }

    for(auto &t : threads){
        if(t.joinable()){
            t.join();
        }
    }

    auto mysql = MysqlConnectionPool::GetMysqlConnectionPool()->getMysqlConnection();

    std::vector<std::string> cookie_list;

    cookie_list = mysql->findData("SELECT id, cookie FROM cookie");

    for(int i = 0; i < cookie_list.size(); i += 2){
        cout << "id : "
             << cookie_list[i] 
             << ", cookie : "
             << cookie_list[i + 1]
             << endl;
    }

    return 0;
}


