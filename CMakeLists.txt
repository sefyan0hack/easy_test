cmake_minimum_required(VERSION 3.30)

project(testing)

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

target_compile_features(testing PUBLIC cxx_std_26)
target_compile_options(testing PUBLIC
    $<$<CXX_COMPILER_ID:GNU>:-freflection>
    $<$<CXX_COMPILER_ID:Clang>:-freflection-latest>
)

add_subdirectory(example)