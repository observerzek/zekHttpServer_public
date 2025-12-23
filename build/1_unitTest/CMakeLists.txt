cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME httpcontent_test)

set(PROJECT_DIR 1_hpptcontentTest)

set(MAIN_NAME httpcontentTest)

project(${PROJECT_NAME})

set(USER_HOME $ENV{HOME})

set(ROOT ${USER_HOME}/cppProject/zekHttpServer)

set(SOURCE 
    ${ROOT}/lib/zekWebServer/src/buffer.cpp
    ${ROOT}/src/http/httprequest.cpp
    ${ROOT}/src/http/httpcontent.cpp
)

file(GLOB LOGGER ${ROOT}/lib/zekAsyncLogger/src/*.cpp)

set(MAIN ${ROOT}/unitTest/${PROJECT_DIR}/${MAIN_NAME}.cpp)

include_directories(${ROOT})

include_directories(${USER_HOME})


link_directories(${USER_HOME}/ZekLib/memoryPool/lib)

add_executable(${PROJECT_NAME} ${MAIN} ${SOURCE} ${LOGGER})

target_link_libraries(
    ${PROJECT_NAME} zekMemoryPool fmt
)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "-g")

set(EXECUTABLE_OUTPUT_PATH ${ROOT}/unitTest/${PROJECT_DIR})




