cmake_minimum_required(VERSION 2.8)
project(ftp_tcp_serv)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread")

set(SOURCE_FILES main.c)
add_executable(ftp_tcp_serv ${SOURCE_FILES})