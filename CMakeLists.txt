cmake_minimum_required(VERSION 3.10)

project(netcore)

add_library(netcore)

target_compile_options(netcore PRIVATE -Wall -Wextra -Werror -O2 -std=c++17 -pthread)

target_include_directories(netcore PRIVATE include)

target_link_libraries(netcore PRIVATE -pthread)


