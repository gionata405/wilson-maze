cmake_minimum_required(VERSION 3.10)
project(WilsonMaze CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(wilson-maze maze.cpp)

target_compile_options(wilson-maze PRIVATE -Wall -Wextra -O2)
