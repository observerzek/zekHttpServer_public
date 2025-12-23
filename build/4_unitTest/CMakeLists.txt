cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME proxy_test)

set(PROJECT_DIR 4_httpproxyTest)

set(MAIN_NAME httpsproxyTest)

project(${PROJECT_NAME})

set(USER_HOME $ENV{HOME})

set(ROOT ${USER_HOME}/cppProject/zekHttpServer)

set(SOURCE 
    # ${ROOT}/lib/zekWebServer/src/buffer.cpp
    ${ROOT}/src/http/httprequest.cpp
    ${ROOT}/src/http/httpcontent.cpp
    ${ROOT}/src/http/httpresponse.cpp
    ${ROOT}/src/http/httpserver.cpp
    ${ROOT}/src/router/router.cpp
    ${ROOT}/src/tls/sslConnect.cpp
    ${ROOT}/src/tls/sslContext.cpp
    ${ROOT}/src/session/session.cpp
    ${ROOT}/src/session/sessionmanager.cpp
    ${ROOT}/src/http/httpsproxy.cpp
    ${ROOT}/src/middleware/middlewarechain.cpp
    ${ROOT}/src/middleware/cookiemiddleware.cpp
    ${ROOT}/src/middleware/authorizationmiddleware.cpp
    ${ROOT}/src/mysql/mysqlpool.cpp
    ${ROOT}/src/mysql/mysqlconnection.cpp

)

file(GLOB LOGGER ${ROOT}/lib/zekAsyncLogger/src/*.cpp)

file(GLOB WEB_SERVER ${ROOT}/lib/zekWebServer/src/*.cpp)


set(MAIN ${ROOT}/unitTest/${PROJECT_DIR}/${MAIN_NAME}.cpp)

include_directories(${ROOT})

include_directories(${USER_HOME}/ZekLib)


link_directories(${USER_HOME}/ZekLib/ZekLib/memoryPool/lib)

add_executable(${PROJECT_NAME} ${MAIN} ${SOURCE} ${LOGGER} ${WEB_SERVER})

find_package(OpenSSL REQUIRED)

target_link_libraries(
    ${PROJECT_NAME} zekMemoryPool fmt 
    mysqlclient
    OpenSSL::SSL
    OpenSSL::Crypto
)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "-g")

set(EXECUTABLE_OUTPUT_PATH ${ROOT}/unitTest/${PROJECT_DIR})




