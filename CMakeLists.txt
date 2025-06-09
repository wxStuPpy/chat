cmake_minimum_required(VERSION 3.10)
project(chat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++20")

aux_source_directory(. SRC)

add_executable(server ${SRC})