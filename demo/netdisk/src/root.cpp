#include "./root.h"
#include <fstream>
#include <filesystem>
#include <sstream>

namespace NetDiskRouter
{


RootRouter::RootRouter(const std::string &files_path)
: m_files_path(files_path)
{

    std::string html;

    html.reserve(4096);

    std::fstream html_file("./html/netdisk.html", std::ios::in);

    std::string line;

    if(html_file.is_open()){
        ZekAsyncLogger::LOG_INFO(
            "successful open html file"
        );

        while(getline(html_file, line)){
            html += line;
        }

    }
    else{
        ZekAsyncLogger::LOG_ERROR(
            "failed to open html file"
        );

    }

    size_t split = html.find("{{FILE_LIST}}");

    m_html_data_prefix = html.substr(0, split);

    m_html_data_suffix = html.substr(split + 13);

    scanDirectory();

    html_file.close();

}



RootRouter::~RootRouter(){

}


void RootRouter::scanDirectory(){

    std::vector<std::string> files;

    std::stringstream ss;

    if(std::filesystem::exists(m_files_path) && std::filesystem::is_directory(m_files_path)){
        for(auto &file : std::filesystem::directory_iterator(m_files_path)){

            if(std::filesystem::is_directory(file) == false){
                files.push_back(std::filesystem::path(file).filename().string());
            }

        }

        ss << "<div class='file-list'><h3>已上传文件</h3>";

        for(auto &file : files){
            ss << "<div class='file-item'>"
               << "<div class='file-info'>"
               << "<span>📄 " << file << " </span>"
               << "<span class='file-type'>普通存储</span>"
               << "</div>"
            //    << "<button onclick=\"window.location='download/"
            //    << file 
            //    << "'\">⬇️ 下载</button>"
               << "<a href = \"download/"
               << file << "\" class='download-btn' download=''>"
               << "下载</a>"
               << "</div>";
        }
        ss << "</div>";

        m_html_files_list = ss.str();

    }
    else{
        m_html_files_list = "No File";
    }
    

}

void RootRouter::forward(ZekHttpServer::HttpRequest *req, ZekHttpServer::HttpResponse *resp){

    scanDirectory();

    std::string content;

    content.reserve(m_html_data_prefix.size() + m_html_files_list.size() + m_html_data_suffix.size());

    content += m_html_data_prefix + m_html_files_list + m_html_data_suffix;

    resp->setContent(content.c_str(), content.size());

    resp->setVersion("HTTP/1.1");
    
    resp->setStateCode(ZekHttpServer::HttpResponse::Code_200_Ok);
    
    resp->setStateMessage("OK");

    resp->addHeader("Data", "Tue, 17 Nov 2025 19:00:00");

    resp->addHeader("Server", "Apache/2.4.41 (Ubuntu)");

    resp->addHeader("Content-Type", "text/html; charset=UTF-8");

    resp->addHeader("Content-Length", std::to_string(content.size()));

    resp->addHeader("Connection", "close");

}


} // namespace NetDiskRouter



