#include <iostream>
#include <include/http/httpserver.h>
#include <fstream>
#include <string>
#include <thread>

const char* ZekWebServer::Logger::file = "./logs/log_1016.txt";

using std::cout;
using std::cin;
using std::endl;
using ZekHttpServer::HttpResponse;
using ZekHttpServer::HttpRequest;


std::string *DEMO;


void openDemo(){
    DEMO = new std::string();
    DEMO->reserve(4096);
    std::fstream fs("demo.html", std::ios::in);
    std::string line;
    while(getline(fs, line)){
        *DEMO += line;
    }
}


void routeFun(const HttpRequest &request, HttpResponse *response){
    response->setContent(DEMO->c_str(), DEMO->size());

    response->setVersion("HTTP/1.1");
    
    response->setStateCode(ZekHttpServer::HttpResponse::Code_200_Ok);
    
    response->setStateMessage("OK");

    response->addHeader("Data", "Tue, 30 Sep 2025 08:00:00");

    response->addHeader("Server", "Apache/2.4.41 (Ubuntu)");

    response->addHeader("Content-Type", "text/html; charset=UTF-8");

    response->addHeader("Content-Length", std::to_string(DEMO->size()));

    response->addHeader("Connection", "close");

}


ZekHttpServer::HttpServer *SERVER = nullptr;


void httpServerTest(){
    ZekHttpServer::HttpServer server(1, 6666);

    SERVER = &server;

    server.registerKeyCallBack(
        ZekHttpServer::HttpRequest::Method::Get,
        "/",
        routeFun
    );

    openDemo();

    server.start();

    SERVER = nullptr;

}


int main(){
    ZekAsyncLogger::AsyncLogger::InitializeAsyncLogger();

    std::thread thread(httpServerTest);

    std::string op;
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
