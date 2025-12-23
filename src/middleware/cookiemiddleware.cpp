#include <include/middleware/cookiemiddleware.h>



namespace ZekHttpServer
{

bool CookieMiddleWare::forware(HttpRequest *req, HttpResponse *res, Session *session){

    return true;
}

bool CookieMiddleWare::backware(HttpRequest *req, HttpResponse *res, Session *session){
    
    if(session){
        
        std::string cookie_value = session->returnCookieHeadValue();
        
        res->addHeader("Set-Cookie", cookie_value);

    }
     
    return true;
}


} // namespace ZekHttpServer