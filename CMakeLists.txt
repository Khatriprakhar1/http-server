cmake_minimum_required(VERSION 3.16)
project(HttpServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

include_directories(include)

add_executable(http_server
    src/TcpServer.cpp
    src/HttpParser.cpp
    src/ThreadPool.cpp
    src/Router.cpp
    main.cpp
)

target_compile_options(http_server PRIVATE -Wall -Wextra -g)
target_link_libraries(http_server PRIVATE pthread)
