cmake_minimum_required(VERSION 3.10)
project(WilsonMaze CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CLI version
add_executable(wilson-maze maze.cpp)
target_compile_options(wilson-maze PRIVATE -Wall -Wextra -O2)

# GUI version
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
add_executable(wilson-maze-gui main.cpp)
target_compile_options(wilson-maze-gui PRIVATE -Wall -Wextra -O2)
target_link_libraries(wilson-maze-gui sfml-graphics sfml-window sfml-system)
