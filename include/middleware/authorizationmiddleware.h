#pragma once
#include "./middleware.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <include/session/session.h>
#include <lib/zekCache/include/lrucache.hpp>
#include <include/mysql/mysqlpool.h>

namespace ZekHttpServer
{


class AuthorizationMiddleWare : public BaseMiddleWare{

private:

    ZekCache::HashLruCache<std::string, std::string> m_caches;

    MysqlConnectionPool *m_mysql_pool;

    std::string m_authorization_method;

    // 当验证用户和密码错误时
    // 会记录当前会话
    std::unordered_map<std::string, int*> m_error_sessions;

    std::mutex m_mutex;

private:

    void create401HttpResponse(HttpResponse *res);

    void create403HttpResponse(HttpResponse *res);

    bool checkIdAndPassword(const std::string &base64_value);

    std::vector<std::string> getNameAndPassword(const std::string &name);

    std::vector<std::string> findInMysql(const std::string &name);

public:

    AuthorizationMiddleWare();

    ~AuthorizationMiddleWare() override;

    virtual bool forware(HttpRequest *req, HttpResponse *res, Session *session) override;

    virtual bool backware(HttpRequest *req, HttpResponse *res, Session *session) override;

};


} //namespace ZekHttpServer