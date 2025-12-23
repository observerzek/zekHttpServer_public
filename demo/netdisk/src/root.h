#pragma once
#include <include/router/routercallback.h>


namespace NetDiskRouter
{

class RootRouter : public ZekHttpServer::AbstractRouterCallBack{

private:

    std::string m_files_path;

    std::string m_html_data_prefix;

    std::string m_html_files_list;

    std::string m_html_data_suffix;


private:

    void scanDirectory();


public:

    RootRouter(const std::string &files_path);

    ~RootRouter() override;

    void forward(ZekHttpServer::HttpRequest *req, ZekHttpServer::HttpResponse *resp) override;


};




} // namespace NetDiskRouter







