project(vendor)

include(FetchContent)
include("../cmake/CommonMacros.txt")

set(GLFW_STANDALONE OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(VOLK_HEADERS_ONLY OFF CACHE BOOL "" FORCE)
set(VOLK_HEADERS_ONLY OFF CACHE BOOL "" FORCE)
set(VOLK_STATIC_DEFINES VOLK_STATIC_DEFINE CACHE STRING "" FORCE)
set(VOLK_PULL_IN_VULKAN ON CACHE BOOL "" FORCE)

set(VMA_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)

fetch_dep(glfw https://github.com/glfw/glfw.git 3.4)
fetch_dep(volk https://github.com/zeux/volk.git 1.4.304)
fetch_dep(vma https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git v3.2.1)