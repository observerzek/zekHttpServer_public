#pragma once
#include <include/utils/utility.h>
#include <vector>
#include "./middleware.h"

namespace ZekHttpServer
{

class MiddleWareChain{

private:

    int m_layer;
    
    std::vector<std::unique_ptr<BaseMiddleWare>> m_middle_wares;


public:
    MiddleWareChain();

    ~MiddleWareChain(){}


    void addMiddleWare(std::unique_ptr<BaseMiddleWare> middle_ware);

    int getLayerSize(){return m_layer;}

    int forward(HttpRequest *req, HttpResponse *res, Session *session = nullptr);

    bool backward(int layer, HttpRequest *req, HttpResponse *res, Session *session = nullptr);


};


} // namespace ZekHttpServer
