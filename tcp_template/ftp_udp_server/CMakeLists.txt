cmake_minimum_required(VERSION 3.8)
project(ftp_udp_server)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c)
add_executable(ftp_udp_server ${SOURCE_FILES})
target_link_libraries(ftp_udp_server wsock32)