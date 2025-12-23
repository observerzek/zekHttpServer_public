#include <include/http/httpcontent.h>
#include <assert.h>
#include <stdio.h>

namespace ZekHttpServer
{

HttpContent::HttpContent()
: m_parse_state(HttpRequestParseState::ParseRequestLine)
, m_request()
{
}

int findCRLF(char *data){
    char *r = strchr(data, '\r');
    if(r){
        if(*(r + 1) == '\n'){
            // 如果此时内容为 \r\n
            // 则表面这还是请求头的结束行
            // 返回-1
            if(r == data) return -1;
            return int(r - data);
        }
    }
    return 0;
}

bool HttpContent::parseRequest(Buffer *buffer, size_t receive_time){

    bool has_more = true;
    bool ok = true;
    while(has_more){
        // 解析请求行
        if(m_parse_state == ParseRequestLine){
            int len = findCRLF(buffer->getPtrRead());
            if(len > 0){
                m_request.setReceiveTime(receive_time);
                // 复制buffer的这段数据
                std::string data = buffer->copyBufferData(len);
                ok = parseRequestLine(data);
                if(ok){
                    // 取出buffer这段数据
                    // 还要取出\r\n这两个字符
                    buffer->getBufferData(len + 2);
                    m_parse_state = ParseHeader;
                }
                else{
                    has_more = false;
                }
            }
            else{
                has_more = false;
                ok = false;
            }
        }
        // 解析请求头
        else if(m_parse_state == ParseHeader){
            int len = findCRLF(buffer->getPtrRead());
            if(len > 0){
                std::string data = buffer->copyBufferData(len);
                ok = parseRequestHeaders(data);
                if(ok){
                    buffer->getBufferData(len + 2);
                }
                else{
                    has_more = false;
                }
            }
            else if(len == -1){
                std::string body_len = m_request.getHeaderValue("Content-Length");
                if(body_len.size() > 0){
                    m_parse_state = ParseBody;
                }
                else{
                    m_parse_state = Complete;
                }
                buffer->getBufferData(2);
            }
            else{
                has_more = false;
                ok = false;
            }
        }
        // 解析请求体
        else if(m_parse_state == ParseBody){
            std::string body_len = m_request.getHeaderValue("Content-Length");
            if(body_len.size() > 0){
                ok = parseRequestBody(buffer, stoi(body_len));
                if(ok){
                    m_parse_state = Complete;
                }
                else{
                    has_more = false;
                }
            }
            else{
                m_parse_state = Complete;
            }
        }
        else{
            assert(m_parse_state == Complete);
            ok = true;
            break;
        }
    }
    const char *info;
    if(ok){
        info = "成功解析Hppt请求报文";
    }
    else{
        info = "解析Hppt请求报文失败";
    }
    ZekAsyncLogger::LOG_INFO("%s", info);
    return ok;


}

bool HttpContent::parseRequestLine(const std::string &data){
    bool result = false;
    std::string::size_type start = 0;
    std::string::size_type space = data.find(' ', start);
    if(space != std::string::npos){
        // 解析请求方法
        m_request.setMethod(data.substr(start, space - start));
        start = space + 1;
        space = data.find(' ', start);

        if(space != std::string::npos){
            // 解析URL
            m_request.setPath(data.substr(start, space - start).c_str());
            start = space + 1;

            // 解析版本
            m_request.setVersion(data.substr(start).c_str());
            result = true;
        }
    }
    return result;


}

bool HttpContent::parseRequestHeaders(const std::string &data){
    bool result = false;
    std::string::size_type space = data.find(' ');
    if(space != std::string::npos){
        m_request.addHeader(
            data.substr(0, space - 1),
            data.substr(space + 1)
        );
        result = true;
    }
    return result;
}

bool HttpContent::parseRequestBody(Buffer *buffer, int body_len){
    bool result = false;

    std::cout   << "bufer len : " << buffer->getReadableSize() << ". "
                << "body_len : " << body_len << std::endl << std::endl;

    if(buffer->getReadableSize() >= body_len){
        m_request.setContent(
            buffer->getBufferData(body_len).c_str(),
            body_len
        );
        result = true;
    }
    return result;
}

void HttpContent::reset(){
    m_parse_state = ParseRequestLine;
    m_request.reset();
}

} // namespace ZekHttpServer
