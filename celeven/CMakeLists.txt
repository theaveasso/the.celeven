cmake_minimum_required(VERSION 3.28)
project(celeven C)

set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} SHARED
        src/cel_core.c
        src/cel_memory.c)
target_include_directories(${PROJECT_NAME} PUBLIC src)

target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_SHARED)
