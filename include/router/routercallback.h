#pragma once
#include <include/utils/utility.h>
#include <include/http/httprequest.h>
#include <include/http/httpresponse.h>

namespace ZekHttpServer
{

class AbstractRouterCallBack{


public:
    virtual ~AbstractRouterCallBack(){}

    virtual void forward(HttpRequest *, HttpResponse *) = 0;


};



}