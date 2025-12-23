#include <lib/zekAsyncLogger/include/logger.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <vector>
#include <cstring>
#include <assert.h>

using std::cout;
using std::endl;

namespace Asy = ZekAsyncLogger;



std::string handleClientHttpRequest(std::string &data){

    std::string original_ip = "8.138.214.123:22222";

    size_t start = 0;

    size_t split_position;

    std::string result;

    result.reserve(data.size());
    
    split_position = data.find(original_ip);

    while(split_position != std::string::npos){
        result += data.substr(start, split_position - start);
        result += "127.0.0.1:7860";

        start = split_position + original_ip.size();

        split_position = data.find(original_ip, start);
    }

    if(start != data.size() - 1){
        result += data.substr(start);
    }

    // cout << result << endl;

    return result;
}

class HttpRequest{
public:
    enum State{
        ParseHead,
        ParseBody
    };


private:
    int m_fd;

    int m_write_fd;

    std::string m_data;
    
    int m_state = ParseHead;
    
    int m_content_length = 0;

    int m_head_length = 0;

    bool m_is_web_socket = false;

    bool m_is_authorized = false;

public:
    HttpRequest(int fd, int write_fd){
        m_fd = fd;
        m_write_fd = write_fd;
    }

    bool checkAuthorized(){
        if(m_is_authorized) return true;

        size_t position = m_data.find("Authorization: ");

        if(position != std::string::npos){
            m_is_authorized = true;
            return true;
        }
        return false;
    }

    bool checkWebSocket(){
        size_t position = m_data.find("Connection: Upgrade");

        if(position != std::string::npos){
            size_t web_soket = m_data.find("Upgrade: websocket");
            if(web_soket != std::string::npos){
                m_is_web_socket = true;
            }
        }
    }

    void findContentLength(){

        if(m_content_length != 0) return;

        if(m_state == ParseBody) return;

        size_t position = m_data.find("Content-Length");

        if(position == std::string::npos){
            position = m_data.find("content-length");
        }

        if(position!= std::string::npos){
            int start = m_data.find(": ", position);

            if(start != std::string::npos){
                int end = m_data.find("\r\n", start);

                if(end != std::string::npos){

                    std::string length = m_data.substr(start + 2, end);
                    
                    m_content_length = std::stoi(length);
                }
            }
        }
    }


    void parseData(){

        if(m_is_web_socket) return;

        if(m_state == ParseBody) return;
        
        findContentLength();

        size_t position = m_data.find("\r\n\r\n");

        checkWebSocket();

        checkAuthorized();

        if(position != std::string::npos){

            m_data = handleClientHttpRequest(m_data);

            position = m_data.find("\r\n\r\n");

            // m_state = ParseBody;

            m_head_length = position + 4;

            cout << m_data.substr(0, m_head_length + 1) << endl;
        }
    }


    void readFromBuffer(const char *data, int len){
        m_data.append(data, len);
    }

    int Write(){
        if(!m_is_authorized && m_head_length > 0){
            std::string http_response;

            http_response = ""
            "HTTP/1.1 401 Unauthorized\r\n"
            "Connection: keep-alive\r\n"
            "Content-Length: 3\r\n"
            "Data: Tue, 17 Nov 2025 19:00:00\r\n"
            "X-Content-Type-Options: nosniff\r\n"
            "Www-Authenticate: Basic realm=\"Restricted\"\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "\r\n"
            "zek"
            "";

            write(m_fd, http_response.c_str(), http_response.size());
            m_data.clear();
            return 0;
        }


        if(m_is_web_socket && m_head_length == 0 && m_content_length == 0){
            int rt = write(m_write_fd, m_data.c_str(), m_data.size());
            m_data.erase(0, rt);
            cout << "web socket send : " << rt << endl;
            return rt;
        }

        if(m_state == ParseHead && m_head_length > 0){
            int rt = write(m_write_fd, m_data.c_str(), m_head_length);

            if(rt > 0){
                m_head_length -= rt;
                m_data.erase(0, rt);
            }
            else if(rt == -1){
                return -1;
            }

            if(m_head_length == 0){
                m_state = ParseBody;
                // 
                if(rt == 0){
                    
                    return -2;
                }
            }

            return rt;
            
        }
        else if(m_state == ParseBody&& m_content_length >= 0){
            int rt = 0;

            if(m_content_length > m_data.size()){
                rt = write(m_write_fd, m_data.c_str(), m_data.size());
            }
            else{
                rt = write(m_write_fd, m_data.c_str(), m_content_length);
            }

            if(rt > 0){
                m_content_length -= rt;
                m_data.erase(0, rt);
            }
            else if(rt == -1){
                return -1;
            }
    
            if(m_content_length == 0){
                m_state = ParseHead;
                parseData();
                if(rt == 0){
                    return -2;
                }
            }
    
            return  rt;
        }

        return 0;
    
    }

};

struct Proxy{
  
    int client_fd;
    int server_fd;
    bool is_listen_fd = false;
    bool is_server = false;
    bool is_client = false;

    Proxy *other_proxy = nullptr;
    
    HttpRequest http_request;
    
    Proxy(int s, int c)
    : http_request(c, s)
    {
        server_fd = s;
        client_fd = c;
        
    }

    bool isIisten(){return is_listen_fd;}

};





void proxyTest(){
    int epoll_fd = epoll_create(1);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    // std::vector<Proxy> proxys;

    int on = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    
    sockaddr_in listen_add;
    listen_add.sin_family = AF_INET;
    listen_add.sin_addr.s_addr = INADDR_ANY;
    listen_add.sin_port = htons(11111);

    bind(listen_fd, (sockaddr*)&listen_add, sizeof(listen_add));

    listen(listen_fd, 10);

    sockaddr_in server_add;
    server_add.sin_family = AF_INET;
    server_add.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_add.sin_port = htons(7860);

    Proxy *listen_proxy = new Proxy(0, 0);

    listen_proxy->is_listen_fd = true;

    epoll_event listen_event;
    listen_event.events = EPOLLIN;
    listen_event.data.ptr = listen_proxy;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_event);

    while(1){
        epoll_event events[32];
        int rt = epoll_wait(epoll_fd, events, 32, 10 * 1000);

        for(int i = 0; i < rt; i++){
            epoll_event event = events[i];

            Proxy *active_proxy = (Proxy *)event.data.ptr;

            if(active_proxy->isIisten()){
                sockaddr_in client_add;
                socklen_t client_len = sizeof(client_add);
                int client_fd = accept(listen_fd, (sockaddr *)&client_add, &client_len);

                int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                int connect_result = connect(server_fd, (sockaddr *)&server_add, sizeof(server_add));

                if(connect_result == 0){
                    cout << "successful connenct proxy" << endl;
                }

                Proxy *server_proxy = new Proxy(server_fd, client_fd);
                server_proxy->is_server = true;

                Proxy *client_proxy = new Proxy(server_fd, client_fd);
                client_proxy->is_client = true;

                epoll_event server_event;
                server_event.events = EPOLLIN;
                server_event.data.ptr = server_proxy;

                epoll_event client_event;
                client_event.events = EPOLLIN;
                client_event.data.ptr = client_proxy;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_event);
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);

            }
            else{
                int client_fd = active_proxy->client_fd;

                int server_fd = active_proxy->server_fd;

                if(active_proxy->is_client){
                    char recv_data[64 * 1024];

                    int recv_len = read(client_fd, recv_data, 64 * 1024);

                    Asy::LOG_INFO("client fd : %d receive data, date length : %d, server fd : %d", client_fd, recv_len, server_fd);

                    if(recv_len == 0 || recv_len == -1){
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);
                        close(client_fd);
                        close(server_fd);
                    }
                    else{
                        // cout << recv_data << endl;
                        
                        active_proxy->http_request.readFromBuffer(recv_data, recv_len);

                        active_proxy->http_request.parseData();

                        int send_length = 0;

                        int rt = active_proxy->http_request.Write();

                        while(rt != 0){
                            if(rt == -1){
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);
                                close(client_fd);
                                close(server_fd);
                                break;
                            }
                            if(rt > 0){
                                send_length += rt;
                            }
                            rt = active_proxy->http_request.Write();
                        }

    
                        // std::string string_data = recv_data;
    
                        // std::string send_string_data = handleClientHttpRequest(string_data);
    
                        // char send_data[send_string_data.size()];
    
                        // std::strcpy(send_data, send_string_data.c_str());
    
    
                        // // cout << send_string_data << endl;
    
                        // while(send_length < send_string_data.size()){
                        //     int sended = write(server_fd, send_data + send_length, send_string_data.size() - send_length); 
    
                        //     if(sended > 0){
                        //         send_length += sended;
                        //     }
                        //     else if(sended == -1){
                        //         epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                        //         epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);
                        //         close(client_fd);
                        //         close(server_fd);
                        //         break;
                        //     }
    
                        // }
    
                        Asy::LOG_INFO("client fd : %d send data, date length : %d, server fd : %d", client_fd, send_length, server_fd);

                    }


                }
                else if(active_proxy->is_server){
                    char recv_data[64 * 1024];

                    int recv_len = read(server_fd, recv_data, 64 * 1024);

                    Asy::LOG_INFO("server fd : %d receive data, date length : %d, client fd : %d", server_fd, recv_len, client_fd);


                    if(recv_len == 0){
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);
                        close(client_fd);
                        close(server_fd);
                    }
                    else{
                        int send_length = 0;

                        while(send_length < recv_len){
                            int sended =  write(client_fd, recv_data + send_length, recv_len - send_length);
    
                            if(sended > 0){
                                send_length += sended;
                            }
                            else if(sended == -1){
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, server_fd, nullptr);
                                close(client_fd);
                                close(server_fd);
                                break;
                            }
                        }
    
                        Asy::LOG_INFO("server fd : %d send data, date length : %d, client fd : %d", server_fd, send_length, client_fd);
    
                    }
                }

            }

        }

    }

}



int main(){
    Asy::AsyncLogger::InitializeAsyncLogger();
    proxyTest();
    return 0;
}