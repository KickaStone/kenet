cmake_minimum_required(VERSION 3.14)

project(netcore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 只在需要时构建测试
include(CTest)          # 定义 BUILD_TESTING 选项（默认 ON）
if (BUILD_TESTING)
    message(STATUS "Build with tests")
endif()

add_library(netcore
        src/Acceptor.cpp
        src/Channel.cpp
        src/InetAddress.cpp
        src/Poller.cpp
        src/EventLoop.cpp
        src/TcpConnection.cpp
        src/TcpServer.cpp
)

target_include_directories(netcore PUBLIC include)
target_link_libraries(netcore PUBLIC pthread)

add_executable(echo_server example/echo_server.cpp)
target_link_libraries(echo_server PRIVATE netcore)

add_executable(echo_client example/echo_client.cpp)
target_link_libraries(echo_client PRIVATE netcore)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

add_subdirectory(test)