cmake_minimum_required(VERSION 3.31)
project(CSOPESY_OS)

set(CMAKE_CXX_STANDARD 23)

add_executable(CSOPESY_OS src/main.cpp
        src/shell/shell.cpp
        src/shell/shell.h)
