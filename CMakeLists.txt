cmake_minimum_required(VERSION 3.20)
project(yourlang CXX)
set(CMAKE_CXX_STANDARD 23)

add_library(reader src/reader.cpp src/node.cpp)
target_include_directories(reader PUBLIC src)

add_subdirectory(tests)
