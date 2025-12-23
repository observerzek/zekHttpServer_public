#pragma once
#include "./middleware.h"
#include <include/session/session.h>

namespace ZekHttpServer
{

class CookieMiddleWare : public BaseMiddleWare{

private:



public:

    ~CookieMiddleWare() override {};

    virtual bool forware(HttpRequest *req, HttpResponse *res, Session *session) override;

    virtual bool backware(HttpRequest *req, HttpResponse *res, Session *session) override;


};



} // namespace ZekHttpServer