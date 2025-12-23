cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME httpserver_test)

set(PROJECT_DIR 2_httpserverTest)

set(MAIN_NAME httpserverTest)

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
)

file(GLOB LOGGER ${ROOT}/lib/zekAsyncLogger/src/*.cpp)

file(GLOB WEB_SERVER ${ROOT}/lib/zekWebServer/src/*.cpp)


set(MAIN ${ROOT}/unitTest/${PROJECT_DIR}/${MAIN_NAME}.cpp)

include_directories(${ROOT})

include_directories(${USER_HOME})


link_directories(${USER_HOME}/ZekLib/memoryPool/lib)

add_executable(${PROJECT_NAME} ${MAIN} ${SOURCE} ${LOGGER} ${WEB_SERVER})

target_link_libraries(
    ${PROJECT_NAME} zekMemoryPool fmt
)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "-g")

set(EXECUTABLE_OUTPUT_PATH ${ROOT}/unitTest/${PROJECT_DIR})




