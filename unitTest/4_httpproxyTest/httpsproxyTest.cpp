#include <iostream>
#include <include/http/httpserver.h>
#include <include/http/httpsproxy.h>
#include <fstream>
#include <string>
#include <thread>
#include <signal.h>

std::string ZekWebServer::Logger::file = "./logs/log_1104.txt";

using std::cout;
using std::cin;
using std::endl;
using ZekHttpServer::HttpResponse;
using ZekHttpServer::HttpRequest;


std::string base64Decode(const std::string &data){

    BIO *b64 = BIO_new(BIO_f_base64());

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO *mem = BIO_new(BIO_s_mem());

    BIO_write(mem, data.c_str(), data.size());

    BIO_push(b64, mem);

    char result[1024];

    size_t size = 0;

    size = BIO_read(b64, result, 1024);
    
    result[size] = '\0';

    return std::string{result};

}




ZekHttpServer::HttpsProxy *PROXY = nullptr;

bool USE_SSL = false;


std::string KEY_PATH = "./sslFiles/observer-zek.asia.key";

std::string CERTIFICATION_PATH = "./sslFiles/observer-zek.asia.pem";

void httpsProxyTest(){

    ZekHttpServer::HttpsProxy proxy(
        1,
        7860, "127.0.0.1",
        6666, nullptr,
        USE_SSL,
        KEY_PATH,
        CERTIFICATION_PATH,
        "zzq",
        "123456"
    );

    PROXY = &proxy;

    proxy.start();

    PROXY = nullptr;

}


int main(){
    ZekAsyncLogger::AsyncLogger::InitializeAsyncLogger();

    signal(SIGPIPE, SIG_IGN);

    std::string op;

    cout << "activate SSL server : " << std::flush;

    cin >> op;

    if(op == "y" || op == "yes"){
        USE_SSL = true;
    } 

    std::thread thread(httpsProxyTest);

    cout << "is end : ";
    cin >> op;

    if(PROXY){
        PROXY->stop();
    }
    if(thread.joinable()){
        thread.join();
    }

    ZekAsyncLogger::AsyncLogger::Stop();
    return 0;
}
