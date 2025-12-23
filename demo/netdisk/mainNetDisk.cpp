#include <include/http/httpserver.h>
#include "./src/root.h"
#include "./src/download.h"
#include "./src/upload.h"
#include <thread>
#include <iostream>
#include <string>

std::string ZekWebServer::Logger::file = "./logs/log_1119.txt";

using std::cout;
using std::cin;
using std::endl;

ZekHttpServer::HttpServer *SERVER = nullptr;

std::string KEY_PATH = "/home/zek/sslFile/observer-zek.asia.key";

std::string CERTIFICATION_PATH = "/home/zek/sslFile/observer-zek.asia.pem";

bool USE_SSL = false;


void NetDisk(){

    ZekHttpServer::HttpServer server(
        USE_SSL,
        KEY_PATH,
        CERTIFICATION_PATH,
        4, 
        6666
    );

    NetDiskRouter::RootRouter root_router("./files");

    NetDiskRouter::DownloadRouter download_router("./files");

    NetDiskRouter::UploadRouter upload_router("./files");

    server.registerKeyFunCallBack(
        ZekHttpServer::HttpRequest::Method::Get,
        "/",
        &root_router
    );

    server.registerDynamicKeyFunCallBack(
        ZekHttpServer::HttpRequest::Method::Get,
        "/download*",
        &download_router   
    );

    server.registerDynamicKeyFunCallBack(
        ZekHttpServer::HttpRequest::Method::Post,
        "/upload*",
        &upload_router
    );

    SERVER = &server;

    server.start();

    SERVER = nullptr;

}




int main(){
    ZekAsyncLogger::AsyncLogger::InitializeAsyncLogger();

    std::string op;

    cout << "use ssl : ";

    cin >> op;
    if(op == "y" || op == "yes"){
        USE_SSL = true;
    }

    std::thread thread(NetDisk);

    cout << "is end : ";
    cin >> op;

    if(SERVER){
        SERVER->stop();
    }
    if(thread.joinable()){
        thread.join();
    }

    ZekAsyncLogger::AsyncLogger::Stop();

    return 0;
}



