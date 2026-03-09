cmake_minimum_required(VERSION 3.30)

project(testing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(testing)
target_sources(testing
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/testing.cpp
    PUBLIC
    FILE_SET headers
      TYPE HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/testing.hpp
)
target_compile_options(testing PUBLIC 
    $<$<CXX_COMPILER_ID:GNU,Clang>:-freflection>
    $<$<CXX_COMPILER_ID:Clang>:-fexpansion-statements>
)

add_subdirectory(example)