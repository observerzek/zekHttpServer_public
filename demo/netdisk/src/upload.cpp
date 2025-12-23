#include "./upload.h"
#include <fstream>


namespace NetDiskRouter
{


UploadRouter::UploadRouter(const std::string &files_path)
: m_files_path(files_path)
{

}


UploadRouter::~UploadRouter(){

}

void UploadRouter::forward(ZekHttpServer::HttpRequest *req, ZekHttpServer::HttpResponse *resp){

    std::string file_name = req->getHeaderValue("FileName");

    char *data = req->getContent();

    size_t len = req->getContentLen();

    std::string file_path = m_files_path + "/" + file_name;

    std::fstream file(file_path, std::ios::out | std::ios::binary);

    file.write(data, len);

    file.close();

    resp->setVersion("HTTP/1.1");
    
    resp->setStateCode(ZekHttpServer::HttpResponse::Code_200_Ok);
    
    resp->setStateMessage("OK");

    resp->addHeader("Data", "Tue, 17 Nov 2025 19:00:00");

    resp->addHeader("Server", "Apache/2.4.41 (Ubuntu)");

    resp->addHeader("Content-Type", "text/html; charset=UTF-8");

    resp->addHeader("Connection", "close");



}



} // namespace NetDiskRouter