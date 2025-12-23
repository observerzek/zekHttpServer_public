cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME mysqlpool_test)

set(PROJECT_DIR 5_mysqlTest)

set(MAIN_NAME mysqlpoolUnitTest)

project(${PROJECT_NAME})

set(USER_HOME $ENV{HOME})

set(ROOT ${USER_HOME}/cppProject/zekHttpServer)

set(SOURCE 
    # ${ROOT}/lib/zekWebServer/src/buffer.cpp
    ${ROOT}/src/mysql/mysqlpool.cpp
    ${ROOT}/src/mysql/mysqlconnection.cpp
)

# file(GLOB LOGGER ${ROOT}/lib/zekAsyncLogger/src/*.cpp)

# file(GLOB WEB_SERVER ${ROOT}/lib/zekWebServer/src/*.cpp)


set(MAIN ${ROOT}/unitTest/${PROJECT_DIR}/${MAIN_NAME}.cpp)

include_directories(${ROOT})

# include_directories(${USER_HOME})


# link_directories(${USER_HOME}/ZekLib/memoryPool/lib)

add_executable(${PROJECT_NAME} ${MAIN} ${SOURCE})

find_package(OpenSSL REQUIRED)

target_link_libraries(
    ${PROJECT_NAME}
    mysqlclient
    OpenSSL::SSL
    OpenSSL::Crypto
)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "-g")

set(EXECUTABLE_OUTPUT_PATH ${ROOT}/unitTest/${PROJECT_DIR})




