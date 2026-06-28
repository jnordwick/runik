cmake_minimum_required(VERSION 3.20)
project(runik CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -g -O0)

add_library(runik_lib
    src/reader.cpp
    src/vec.cpp
)
target_include_directories(runik_lib PUBLIC src)

enable_testing()
add_subdirectory(tests)
