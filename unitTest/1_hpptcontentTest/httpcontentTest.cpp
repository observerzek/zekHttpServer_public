#include <include/http/httpcontent.h>
#include <iostream>


using std::cout;
using std::endl;


const char *REQUEST_1 = ""
"POST /root HTTP/1.1\r\n"
"HOST: www.XXX.com\r\n"
"User-Agent: Mozilla/5.0(Windows NT 6.1;rv:15.0) Firefox/15.0\r\n"
"\r\n"
"";

const char *REQUEST_2 = ""
"POST /root HTTP/1.1\r\n"
"HOST: www.XXX.com\r\n"
"Content-Lenght: 2"
"User-Agent: Mozilla/5.0(Windows NT 6.1;rv:15.0) Firefox/15.0\r\n"
"\r\n"
"12"
"";

const char *REQUEST_3 = ""
"POST /root HTTP/1.1\r\n"
"HOST: www.XXX.com\r\n"
"\r"
"Content-Lenght: 2"
"User-Agent: Mozilla/5.0(Windows NT 6.1;rv:15.0) Firefox/15.0\r\n"
"\r\n"
"12"
"";


int main(){
    ZekAsyncLogger::AsyncLogger::InitializeAsyncLogger("HttpContentTest");
    ZekAsyncLogger::LOG_INFO("-------------");
    ZekAsyncLogger::LOG_INFO("test start...");
    ZekHttpServer::HttpContent content;
    cout << REQUEST_1 << endl;
    
    ZekWebServer::Buffer buffer;
    buffer.append(REQUEST_1, strlen(REQUEST_1));

    content.parseRequest(&buffer);
    content.reset();

    buffer.append(REQUEST_2, strlen(REQUEST_2));
    buffer.append(REQUEST_3, strlen(REQUEST_3));

    content.parseRequest(&buffer);
    content.reset();
    content.parseRequest(&buffer);


    ZekAsyncLogger::LOG_INFO("test end...");
    ZekAsyncLogger::LOG_INFO("-------------");
    ZekAsyncLogger::AsyncLogger::Stop();
    return 0;
}