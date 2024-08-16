cmake_minimum_required(VERSION 3.28)
project(dinput8)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(dinput8 SHARED main.cpp)

if (MSVC)
    add_custom_target(exports_def DEPENDS ${CMAKE_BINARY_DIR}/exports.def)

    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/exports.def
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/exports.def ${CMAKE_CURRENT_BINARY_DIR}/exports.def
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/exports.def)

    target_link_options(dinput8 PRIVATE /DEF:${CMAKE_BINARY_DIR}/exports.def)

    add_dependencies(dinput8 exports_def)
endif()

target_link_libraries(dinput8 shlwapi psapi)