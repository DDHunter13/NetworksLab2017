cmake_minimum_required(VERSION 2.8)
project(multi_server_linux)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread")

set(SOURCE_FILES main.c)
add_executable(multi_server_linux ${SOURCE_FILES})