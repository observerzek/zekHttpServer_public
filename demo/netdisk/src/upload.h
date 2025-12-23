#pragma once
#include <include/router/routercallback.h>


namespace NetDiskRouter
{

class UploadRouter : public ZekHttpServer::AbstractRouterCallBack{

private:

    std::string m_files_path;


public:
    

    UploadRouter(const std::string &files_path);

    ~UploadRouter();

    void forward(ZekHttpServer::HttpRequest *req, ZekHttpServer::HttpResponse *resp);



};



} // namespace NetDiskRouter