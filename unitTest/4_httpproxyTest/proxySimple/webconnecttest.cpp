#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

using std::cout;
using std::endl;



void webConnectTest(){
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_add;
    server_add.sin_family = AF_INET;
    server_add.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_add.sin_port = htons(7860);

    int rt = connect(fd, (sockaddr *)&server_add, sizeof(server_add));

    if(rt == 0){
        cout << "successful connect..." << endl;
    }

    std::string send_data;

    send_data =  "GET / HTTP/1.1\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Accept-Language: zh-CN,zh;q=0.9\r\n"
    "Cache-Control: max-age=0\r\n"
    "Connection: keep-alive\r\n"
    "Host: 127.0.0.1:7860\r\n"
    "Sec-Fetch-Dest: document\r\n"
    "Sec-Fetch-Mode: navigate\r\n"
    "Sec-Fetch-Site: none\r\n"
    "Sec-Fetch-User: ?1\r\n"
    "Upgrade-Insecure-Requests: 1\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.5845.97 Safari/537.36 Core/1.116.569.400 QQBrowser/19.7.6765.400\r\n"
    "sec-ch-ua: \"Not)A;Brand\";v=\"24\", \"Chromium\";v=\"116\"\r\n"
    "sec-ch-ua-mobile: ?0\r\n"
    "sec-ch-ua-platform: \"Windows\"\r\n\r\n";

    int send_length = write(fd, send_data.c_str(), send_data.size());

    cout << "send length : " << send_length << endl;

    char recv_data[1024];

    int recv_length = read(fd, recv_data, 1024);

    cout << "recv length : " << recv_length << endl;

    cout << recv_data << endl;

}





int main(){
    webConnectTest();
    return 0;
}