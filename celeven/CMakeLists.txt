cmake_minimum_required(VERSION 3.28)
project(celeven C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} SHARED
        src/vk_mem_alloc.cpp
        src/cel.c
        src/cel_core.c
        src/cel_log.c
        src/cel_memory.c
        src/cel_vulkan.c)

target_link_libraries(${PROJECT_NAME} PUBLIC volk::volk GPUOpen::VulkanMemoryAllocator glfw)

target_include_directories(${PROJECT_NAME} PUBLIC src)

target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_SHARED)
add_dependencies(${PROJECT_NAME} celshader_compile)
