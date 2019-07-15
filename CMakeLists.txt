cmake_minimum_required(VERSION 3.14)
project(nothing)
include_directories(src)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror -Wfatal-errors -pipe -lstdc++fs")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fpie -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb -fsanitize=address,undefined,leak")

set (src
	src/main.cpp
	src/core/database.cpp
	src/core/scanner.cpp
)

add_executable(nothing ${src})
target_link_libraries(nothing)
