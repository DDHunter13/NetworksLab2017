cmake_minimum_required(VERSION 2.8)
project(udp_server)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

set(SOURCE_FILES main.c)
add_executable(udp_server ${SOURCE_FILES})