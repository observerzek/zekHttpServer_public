#include "./download.h"
#include <functional>


namespace NetDiskRouter
{


DownloadRouter::DownloadRouter(const std::string &files_path)
: m_files_path(files_path)
{
    
}

DownloadRouter::~DownloadRouter(){

}



void DownloadRouter::forward(ZekHttpServer::HttpRequest *req, ZekHttpServer::HttpResponse *resp){
    std::string path = req->getPath();

    size_t split = path.find_last_of("/");

    if(split == std::string::npos){

        return;
    }


    std::string filename = path.substr(split + 1);

    std::string file_path = m_files_path + "/" + filename;

    std::fstream file(file_path, std::ios::in | std::ios::binary);

    size_t file_size = 0;

    if(file.is_open()){
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
        file.close();
    }
    
    std::string content;

    resp->setContent(content.c_str(), content.size());

    resp->addHeader("Accept-Ranges", "bytes");

    resp->setVersion("HTTP/1.1");
    
    resp->setStateCode(ZekHttpServer::HttpResponse::Code_200_Ok);
    
    resp->setStateMessage("OK");

    resp->addHeader("Data", "Tue, 17 Nov 2025 19:00:00");

    resp->addHeader("Server", "Apache/2.4.41 (Ubuntu)");

    resp->addHeader("Content-Dispisition", "attachment; filename=\"" + filename + "\"");

    resp->addHeader("Content-Type", "application/octet-stream");

    resp->addHeader("Content-Length", std::to_string(file_size));

    resp->addHeader("Connection", "keep-alive");

    ZekWebServer::TcpConnection *tcp_connection = req->getTcpConnection();

    ZekHttpServer::SSLConnection *ssl_connection = req->getSslConnection();


    tcp_connection->setWriteEventCB(
        std::bind(
            &DownloadRouter::SendFile,
            tcp_connection,
            ssl_connection,
            file_path,
            0,
            file_size
        )
    );

    tcp_connection->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);



}


void DownloadRouter::SendFile(
ZekWebServer::TcpConnection *tcp_connection,
ZekHttpServer::SSLConnection *ssl_connection,
const std::string &file_path,
int begin,
int end
)
{
    std::fstream file(file_path, std::ios::in | std::ios::binary);

    file.seekg(begin, std::ios::beg);

    std::string data;

    data.reserve(1024);

    if(end - begin > 1024){
        tcp_connection->setWriteEventCB(
            std::bind(
                &DownloadRouter::SendFile,
                tcp_connection,
                ssl_connection,
                file_path,
                begin + 1024,
                end
            )
        );
        tcp_connection->getChannel()->registerEvent(ZekWebServer::Channel::WRITE);

        file.read(&(data[0]), 1024);

   }
    else{

        tcp_connection->setWriteEventCB(nullptr);

        file.read(&(data[0]), end - begin);
    }

    ZekAsyncLogger::LOG_DEBUG(
        "fd : %d, connect id : %d, send %d ~ %d Bytes",
        tcp_connection->getFd(),
        tcp_connection->getConnectId(),
        begin,
        end - begin > 1024 ? begin + 1024 : end
    );

    if(ssl_connection){
        ssl_connection->encryptData(tcp_connection, data.c_str(), 1024);
    }
    else{
        tcp_connection->send(data.c_str(), 1024);
    }

    // sleep(0.5);
    // usleep(2 * 1000);

    file.close();
}



} // namespace NetDiskRouter