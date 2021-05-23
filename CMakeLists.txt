cmake_minimum_required(VERSION 3.16)

project(gameproject CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src/glad/include)

add_executable(engine
    ${PROJECT_SOURCE_DIR}/src/engine.hxx
    ${PROJECT_SOURCE_DIR}/src/engine.cxx
    ${PROJECT_SOURCE_DIR}/src/stb_image.h
    ${PROJECT_SOURCE_DIR}/src/glad/src/glad.c
    ${PROJECT_SOURCE_DIR}/src/glad/include/glad/glad.h
    ${PROJECT_SOURCE_DIR}/src/glad/include/KHR/khrplatform.h
    )

target_link_libraries(engine PRIVATE SDL2::SDL2 SDL2::SDL2main OpenGL::GL -ldl)
