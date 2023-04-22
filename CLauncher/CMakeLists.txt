cmake_minimum_required(VERSION 3.15)
project(CLauncher)

file(GLOB_RECURSE SOURCES "*.c" "*.h")

include_directories(./)
include_directories(${CMAKE_SOURCE_DIR}/ui-imgui)

include_directories(${CMAKE_SOURCE_DIR}/third/cJSON/include)
link_directories(${CMAKE_SOURCE_DIR}/third/cJSON/Debug)

include_directories(${CMAKE_SOURCE_DIR}/third/curl/x64Debug/include)
link_directories(${CMAKE_SOURCE_DIR}/third/curl/x64Debug/lib)

include_directories(${CMAKE_SOURCE_DIR}/third/minhook/include)
link_directories(${CMAKE_SOURCE_DIR}/third/minhook/lib/Debug)

include_directories(${CMAKE_SOURCE_DIR}/third/ultralight/include)
link_directories(${CMAKE_SOURCE_DIR}/third/ultralight/lib)

include_directories(${CMAKE_SOURCE_DIR}/third/ziplib/lib)
link_directories(${CMAKE_SOURCE_DIR}/third/ziplib/x64/Debug)

link_directories(${CMAKE_SOURCE_DIR}/third/zlib/x64/Debug)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE libcurl_a_debug)
target_link_libraries(${PROJECT_NAME} PRIVATE libz-static.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE libzip-static.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE AppCore Ultralight UltralightCore WebCore)
target_link_libraries(${PROJECT_NAME} PRIVATE libMinHook.x64.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE cjson)
target_link_libraries(${PROJECT_NAME} PUBLIC ui-imgui)

target_sources(${PROJECT_NAME} PRIVATE Resource.rc)