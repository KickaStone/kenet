cmake_minimum_required(VERSION 3.10)

project(netcore)

# 添加echo_server可执行文件
add_executable(echo_server 
    example/echo_server.cpp 
    net/Logger.cpp 
    net/InetAddress.cpp
    net/EventLoop.cpp
    net/Poller.cpp
    net/Channel.cpp
    net/Acceptor.cpp
    net/TcpConnection.cpp
    net/TcpServer.cpp
)

add_executable(echo_client 
    example/echo_client.cpp 
)

# 为echo_server设置编译选项
target_compile_options(echo_server PRIVATE -Wall -Wextra -Werror -O2 -std=c++17 -pthread)

# 为echo_client设置编译选项
target_compile_options(echo_client PRIVATE -Wall -Wextra -Werror -O2 -std=c++17 -pthread)

# 为echo_server设置包含目录
target_include_directories(echo_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 链接pthread
target_link_libraries(echo_server PRIVATE -pthread)

target_link_libraries(echo_client PRIVATE -pthread)


