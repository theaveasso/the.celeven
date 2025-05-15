cmake_minimum_required(VERSION 3.28)
project(the_celeven C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendor)
add_subdirectory(celeven)
add_subdirectory(example)

set(VULKAN_SDK $ENV{VULKAN_SDK})
set(GLSLC_EXECUTABLE ${VULKAN_SDK}/Bin/glslc.exe)
if (NOT EXISTS ${GLSLC_EXECUTABLE})
    message(FATAL_ERROR "glslc not found at ${GLSL_EXECUTABLE}")
endif ()

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/resources/shaders)
file(GLOB_RECURSE SHADER_SOURCES ${SHADER_DIR}/*.vert.glsl ${SHADER_DIR}/*.frag.glsl)
set(SPIRV_OUTPUTS "")
foreach (SHADER ${SHADER_SOURCES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SPIRV_OUTPUT ${SHADER_DIR}/${FILE_NAME}.spv)

    if (FILE_NAME MATCHES "\\.common\\.")
        continue()
    elseif (FILE_NAME MATCHES "\\.vert\\.")
        set(SHADER_STAGE "vert")
    elseif (FILE_NAME MATCHES "\\.frag\\.")
        set(SHADER_STAGE "frag")
    else ()
        message(FATAL_ERROR "Unknown shader stage for ${FILE_NAME}")
    endif ()

    message(STATUS "Compiling ${FILE_NAME} with stage ${SHADER_STAGE}")
    message(STATUS "Command: ${GLSLC_EXECUTABLE} -fshader-stage=${SHADER_STAGE} ${SHADER} -o ${SPIRV_OUTPUT}")

    add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=${SHADER_STAGE} ${SHADER} -o ${SPIRV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "celshader compiling: ${FILE_NAME}"
            VERBATIM
    )

    list(APPEND SPIRV_OUTPUTS ${SPIRV_OUTPUT})
endforeach ()
add_custom_target(celshader_compile DEPENDS ${SPIRV_OUTPUTS})