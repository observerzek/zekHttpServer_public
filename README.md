# 使用方法

开启https服务，请参考`unitTest/3_sslTest/sslTest.cpp`

开启反向代理服务，请参考`unitTest/4_httpproxyTest/httpsproxyTest.cpp`

# 简单使用方法
```c++
#include "include/http/httpserver.h"

using ZekHttpServer::HttpResponse;
using ZekHttpServer::HttpRequest;

void routeFun(HttpRequest *request, HttpResponse *response);

int main(){
    // 开启异步日志
    ZekAsyncLogger::AsyncLogger::InitializeAsyncLogger();
    // 设置TLS私钥地址
    std::string KEY_PATH = "/home/zek/sslFile/observer-zek.asia.key";
    // 设置TLS证书地址
    std::string CERTIFICATION_PATH = "/home/zek/sslFile/observer-zek.asia.pem";
    // 设置线程数
    int thread_count = 4;
    // 设置监听端口
    int listen_port = 6666;
    // 设置是否开启https
    bool use_ssl = true;
    // 实例化服务器对象
    ZekHttpServer::HttpServer server {
        use_ssl,
        KEY_PATH,
        CERTIFICATION_PATH,
        thread_count, 
        listen_port
    };
    // 注册根路由Get请求的具体响应函数
    server.registerKeyCallBack(
        ZekHttpServer::HttpRequest::Method::Get,
        "/",
        routeFun
    );

    // 服务器开始监听
    server.start();

    /*
    执行其他逻辑
    */

    // 服务器停止
    server.stop();

    // 关闭异步日志
    ZekAsyncLogger::AsyncLogger::Stop();
    return 0;
}



```