cmake_minimum_required(VERSION 3.31)
project(CSOPESY_OS)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CSOPESY_OS src/main.cpp
        src/shell/shell.cpp
        src/shell/shell.h
        src/process/process.cpp
        src/process/process.h
        src/session/session.cpp
        src/session/session.h)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(CSOPESY_OS PRIVATE -std=c++23)
    target_link_libraries(CSOPESY_OS PRIVATE "-lstdc++exp")
endif ()