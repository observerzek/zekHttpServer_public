#include <iostream>
#include <include/http/httpserver.h>
#include <fstream>
#include <string>
#include <thread>

std::string ZekWebServer::Logger::file = "./logs/log_1115.txt";

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


std::string *DEMO;


void openDemo(){
    DEMO = new std::string();
    DEMO->reserve(4096);
    std::fstream fs("netdisk.html", std::ios::in);
    std::string line;
    while(getline(fs, line)){
        *DEMO += line;
    }
}


void routeFun(HttpRequest *request, HttpResponse *response){
    response->setContent(DEMO->c_str(), DEMO->size());

    response->setVersion("HTTP/1.1");
    
    response->setStateCode(ZekHttpServer::HttpResponse::Code_200_Ok);
    
    response->setStateMessage("OK");

    response->addHeader("Data", "Tue, 17 Nov 2025 19:00:00");

    response->addHeader("Server", "Apache/2.4.41 (Ubuntu)");

    response->addHeader("Content-Type", "text/html; charset=UTF-8");

    response->addHeader("Content-Length", std::to_string(DEMO->size()));

    response->addHeader("Connection", "close");

}

void routeFun_2(HttpRequest *request, HttpResponse *response){


    auto it = request->getHeaderValue("Authorization");

    int post = it.find(" ");

    
    if(it.size() > 0){
        std::string reuslt = base64Decode(it.substr(post + 1));
        routeFun(request, response);
        return;
    }


    response->setContent("zek", 3);

    response->setVersion("HTTP/1.1");
    
    response->setStateCode(ZekHttpServer::HttpResponse::Code_401_Unauthorized);
    
    response->setStateMessage("Unauthorized");

    response->addHeader("Content-Type", "text/plain; charset=UTF-8");

    response->addHeader("Www-Authenticate", "Basic realm=\"Restricted\"");
    
    response->addHeader("X-Content-Type-Options", "nosniff");

    response->addHeader("Data", "Tue, 17 Nov 2025 19:00:00");

    response->addHeader("Content-Length", "3");

    response->addHeader("Connection", "keep-alive");
}


ZekHttpServer::HttpServer *SERVER = nullptr;

std::string KEY_PATH = "/home/zek/sslFile/observer-zek.asia.key";

std::string CERTIFICATION_PATH = "/home/zek/sslFile/observer-zek.asia.pem";

bool USE_SSL = false;

void httpServerTest(){
    ZekHttpServer::HttpServer server(
        USE_SSL,
        KEY_PATH,
        CERTIFICATION_PATH,
        4, 
        6666
    );

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


    std::string op;

    cout << "use ssl : ";

    cin >> op;
    if(op == "y" || op == "yes"){
        USE_SSL = true;
    }
    
    std::thread thread(httpServerTest);

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
