cmake_minimum_required(VERSION 4.0.0)
project(EMUL-8 VERSION 1.0.0)

set(SOURCE_FILES
  src/main.cpp
  src/CHIP8.cpp
  src/glad.c)

find_package(OpenGL REQUIRED)

add_subdirectory(dependencies/glfw)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} glfw OpenGL::GL)
target_include_directories(${PROJECT_NAME} PRIVATE dependencies)