#pragma once
#include <include/utils/utility.h>
#include <include/http/httprequest.h>
#include <include/http/httpresponse.h>

namespace ZekHttpServer
{

class Session;


class BaseMiddleWare{

public:

    virtual ~BaseMiddleWare() = default;

    virtual bool forware(HttpRequest *req, HttpResponse *res, Session *session = nullptr) = 0;

    virtual bool backware(HttpRequest *req, HttpResponse *res, Session *session = nullptr) = 0;

};



} // namespace ZekHttpServer

