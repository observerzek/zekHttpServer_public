#pragma once
#include <include/router/routercallback.h>
#include <lib/zekCache/include/cache.hpp>
#include <include/http/httpcontent.h>

namespace NetDiskRouter
{

class DownloadRouter : public ZekHttpServer::AbstractRouterCallBack{

private:

    std::string m_files_path;


private:

    static void SendFile(
        ZekWebServer::TcpConnection *tcp_connection,
        ZekHttpServer::SSLConnection *ssl_connection,
        const std::string &file_path,
        int begin,
        int end
    );

public:

    DownloadRouter(const std::string &files_path);

    ~DownloadRouter();

    void forward(ZekHttpServer::HttpRequest *req, ZekHttpServer::HttpResponse *resp);


};




} // namespace NetDiskRouter