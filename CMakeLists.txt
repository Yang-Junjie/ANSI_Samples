cmake_minimum_required(VERSION 3.16)

project(ANSI_Samples LANGUAGES CXX)

add_library(ansi INTERFACE)
target_include_directories(ansi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ansi INTERFACE cxx_std_17)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/externals)

add_subdirectory(externals/stb)
add_subdirectory(samples)
