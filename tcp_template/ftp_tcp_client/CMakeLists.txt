cmake_minimum_required(VERSION 2.8)
project(ftp_tcp_client)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

set(SOURCE_FILES main.c)
add_executable(ftp_tcp_client ${SOURCE_FILES})
target_link_libraries(ftp_tcp_client wsock32)