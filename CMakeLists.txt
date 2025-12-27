cmake_minimum_required(VERSION 3.20)
project(flanki_shooter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
FetchContent_MakeAvailable(raylib)

add_executable(flanki_shooter
    src/main.cpp
)

target_link_libraries(flanki_shooter PRIVATE raylib)

if (MSVC)
    target_compile_definitions(flanki_shooter PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
