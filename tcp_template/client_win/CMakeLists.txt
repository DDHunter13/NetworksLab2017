cmake_minimum_required(VERSION 2.8)
project(client_win)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(SOURCE_FILES main.c)
add_executable(client_win ${SOURCE_FILES})
target_link_libraries(client_win wsock32)