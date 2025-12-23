#include <include/middleware/authorizationmiddleware.h>
#include <assert.h>


namespace ZekHttpServer
{

AuthorizationMiddleWare::AuthorizationMiddleWare()
: m_caches(40, 4)
{
    m_mysql_pool = MysqlConnectionPool::GetMysqlConnectionPool();

    m_authorization_method = "Basic";

}

AuthorizationMiddleWare::~AuthorizationMiddleWare(){

}

void AuthorizationMiddleWare::create401HttpResponse(HttpResponse *res){

    res->reset();

    res->setContent("zek", 3);

    res->setVersion("HTTP/1.1");

    res->setStateCode(HttpResponse::Code_401_Unauthorized);

    res->setStateMessage("Unauthorized");

    res->addHeader("Content-Type", "text/plain; charset=UTF-8");

    res->addHeader("Www-Authenticate", "Basic realm=\"Restricted\"");
    
    res->addHeader("X-Content-Type-Options", "nosniff");

    res->addHeader("Data", "Tue, 17 Nov 2025 19:00:00");

    res->addHeader("Content-Length", "3");

    res->addHeader("Connection", "keep-alive");
    
}

void AuthorizationMiddleWare::create403HttpResponse(HttpResponse *res){
    res->reset();

    res->setVersion("HTTP/1.1");

    res->setStateCode(HttpResponse::Code_403_Forbidden);

    res->setStateMessage("Forbidden");

    res->addHeader("Date", "Fri, 31 Oct 2025 12:00:00 GMT");

    res->addHeader("Server", "Apache/2.4.41 (Ubuntu)");

    res->addHeader("Content-Type", "text/html; charset=UTF-8");

    res->addHeader("Content-Length", "172");

    res->addHeader("Connection", "close");

    res->setContent(
        ""
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>403 Forbidden</title>"
        "</head>"
        "<body>"
        "<h1>Forbidden</h1>"
        "<p>You don't have permission to access this resource on this server.</p>"
        "</body>"
        "</html>"
        "",
        172
    );
}

std::vector<std::string> AuthorizationMiddleWare::findInMysql(const std::string &name){
    std::string sql = ""
                      "SELECT name, password FROM user_name_password WHERE name = '" + name + "'";
                      "";  

    auto mysql = m_mysql_pool->getMysqlConnection();

    std::vector<std::string> result = mysql->findData(sql);

    return std::move(result);
}


std::vector<std::string> AuthorizationMiddleWare::getNameAndPassword(const std::string &name){

    std::string password = m_caches.get(name);

    std::vector<std::string> result(2, "");

    result[0] = name;

    if(password.size() == 0){
        std::vector<std::string> mysql_result = findInMysql(name);

        if(mysql_result.size() > 0){
            assert(mysql_result.size() == 2);
            assert(mysql_result[0] == result[0]);

            password = mysql_result[1];

            m_caches.put(name, password);
        }
    }

    result[1] = password;

    return std::move(result);
}



bool AuthorizationMiddleWare::checkIdAndPassword(const std::string &base64_value){
    const char *encode_data = base64_value.c_str();
    int len = base64_value.size();

    BIO *base64_bio_in = BIO_new(BIO_s_mem());

    BIO *base64_bio_out = BIO_new(BIO_f_base64());

    BIO_set_flags(base64_bio_out, BIO_FLAGS_BASE64_NO_NL);

    // 编码数据 -> bio_in(mem) -> bio_out(base64) -> 解码数据
    BIO_push(base64_bio_out, base64_bio_in);

    BIO_write(base64_bio_in, encode_data, len);

    char decode_data[64];

    size_t size = BIO_read(base64_bio_out, decode_data, 64);

    decode_data[size] = '\0';

    char *split = strchr(decode_data, ':');

    std::string id(decode_data, split - decode_data);

    std::string password(split + 1);

    BIO_free(base64_bio_in);

    BIO_free(base64_bio_out);

    std::vector<std::string> result = getNameAndPassword(id);

    if(result[1].size() == 0) return false;

    return id == result[0] && password == result[1];
}

bool AuthorizationMiddleWare::forware(HttpRequest *req, HttpResponse *res, Session *session){
    std::string value = req->getHeaderValue("Authorization");
    if(value.size() == 0){
        create401HttpResponse(res);
        return false;
    }

    // 由于前面返回的401报文里
    // Www-Authenticate 的内容为basic
    // 后续http请求里的Authorization头，内容为 "basic xxxxxx"
    // xxxx为经过base64编码后的id和password
    // 因此要跳过前面的"basic "
    value = value.substr(6);

    bool correct = checkIdAndPassword(value);

    if(!session){
        if(!correct){
            create403HttpResponse(res);
            return false;
        }
        else{
            return true;
        }
    }


    int *error_time_ptr =  nullptr;

    int error_time = 0;

    {
        std::shared_lock<std::shared_mutex> lock(session->m_shared_mutex);
        
        error_time_ptr = (int*)session->getData("Error_time");

        if(error_time_ptr){
            
            error_time = *error_time_ptr;

        }
        
    }

    if(!correct){

        std::lock_guard<std::shared_mutex> lock(session->m_shared_mutex);


        if(!error_time_ptr){
            error_time_ptr = (int*)session->setData("Error_time", sizeof(int));
        }

        (*error_time_ptr)++; 

        if((*error_time_ptr) >= 3){
            create403HttpResponse(res);
            // *error_time_ptr = 0;
        }
        else{
            create401HttpResponse(res);
        }
        return false;
    }


    if(error_time_ptr && error_time > 0){
        std::lock_guard<std::shared_mutex> lock(session->m_shared_mutex);

        *error_time_ptr = 0;

        error_time = 0;
    }
    
    return true;
}

bool AuthorizationMiddleWare::backware(HttpRequest *req, HttpResponse *res, Session *session){

    return true;
}



} // namespace ZekHttpServer