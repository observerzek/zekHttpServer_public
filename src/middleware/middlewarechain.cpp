#include <include/middleware/middlewarechain.h>
#include <assert.h>

namespace ZekHttpServer
{

MiddleWareChain::MiddleWareChain(){
    m_layer = 0;
    m_middle_wares.reserve(4);
}


void MiddleWareChain::addMiddleWare(std::unique_ptr<BaseMiddleWare> middle_ware){
    m_layer++;
    m_middle_wares.push_back(std::move(middle_ware));
}

int MiddleWareChain::forward(HttpRequest *req, HttpResponse *res, Session *session){
    int next_layer = 0;
    for(int i = 0; i < m_layer; i++){
        bool continued = m_middle_wares[i]->forware(req, res, session);
        next_layer++;
        if(!continued){
            return next_layer;
        }
    }
    next_layer++;
    
    return next_layer;
}

bool MiddleWareChain::backward(int layer, HttpRequest *req, HttpResponse *res, Session *session){
    assert(layer <= m_layer);
    for(int i = layer - 1; i >= 0; i--){
        m_middle_wares[i]->backware(req, res, session);
    }
    return true;
}
    
    
} // namespace ZekHttpServer