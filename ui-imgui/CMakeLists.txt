cmake_minimum_required(VERSION 3.15)
project(ui-imgui)

file(GLOB_RECURSE SOURCES "*.cpp" "*.c" "*.h")

include_directories(${CMAKE_SOURCE_DIR}/CLauncher)
include_directories(${CMAKE_SOURCE_DIR}/third/glfw/include)
include_directories(./)

message(STATUS ${CMAKE_SOURCE_DIR}/third/glfw/lib-vc2022)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third/glfw/lib-vc2022/glfw3.lib)