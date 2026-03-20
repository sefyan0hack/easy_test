cmake_minimum_required(VERSION 3.30)

project(easy_test)

option(EASY_TEST_BUILD_EXAMPLES "Build Example Exe" OFF)

add_library(easy_test)

target_sources(easy_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/easy_test.cpp
    PUBLIC
    FILE_SET headers
      TYPE HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
      FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/easy_test/easy_test.hpp
)

target_compile_features(easy_test PUBLIC cxx_std_26)
target_compile_options(easy_test PUBLIC
    $<$<CXX_COMPILER_ID:GNU>:-freflection>
    $<$<CXX_COMPILER_ID:Clang>:-freflection-latest>
)

if(EASY_TEST_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()