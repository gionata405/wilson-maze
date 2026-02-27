cmake_minimum_required(VERSION 3.16)
project(WilsonMaze CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CLI version
add_executable(wilson-maze maze.cpp)
target_compile_options(wilson-maze PRIVATE -Wall -Wextra -O2)

# GUI version — SFML 3.0.2 via FetchContent
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(wilson-maze-gui main.cpp)
target_compile_options(wilson-maze-gui PRIVATE -Wall -Wextra -O2)
target_link_libraries(wilson-maze-gui SFML::Graphics SFML::Window SFML::System)
