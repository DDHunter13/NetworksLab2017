cmake_minimum_required(VERSION 3.8)
project(ftp_udp_client)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c)
add_executable(ftp_udp_client ${SOURCE_FILES})