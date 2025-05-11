cmake_minimum_required(VERSION 3.28)
project(example C)

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME}
        game.c
        main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE celeven)