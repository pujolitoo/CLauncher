cmake_minimum_required(VERSION 3.15)
project(CLauncher)

set(OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/out)

add_subdirectory(./CLauncher)
add_subdirectory(./ui-imgui)

make_directory(${OUT_DIR})
file(COPY_FILE ${CMAKE_SOURCE_DIR}/third/cJSON/Debug/cjson.dll ${OUT_DIR}/Debug/cjson.dll)
file(COPY ${CMAKE_SOURCE_DIR}/third/ultralight/bin/ DESTINATION ${OUT_DIR}/Debug)
file(COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/CLauncher/Debug/CLauncher.exe ${OUT_DIR}/Debug/CLauncher.exe)