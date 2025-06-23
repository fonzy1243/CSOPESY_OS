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
        src/session/session.h
        src/scheduler/scheduler.cpp
        src/scheduler/scheduler.h
        src/process/instruction.cpp
        src/process/instruction.h
        src/ApheliOS.cpp
        src/aphelios.h)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(CSOPESY_OS PRIVATE -std=c++23)
    target_link_libraries(CSOPESY_OS PRIVATE "-lstdc++exp")
endif ()

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif ()

include(FetchContent)
FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

target_link_libraries(CSOPESY_OS PRIVATE
        # Chose a submodule
        ftxui::component
        ftxui::dom
        ftxui::screen
)