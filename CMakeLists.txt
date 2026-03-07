cmake_minimum_required(VERSION 3.30)

project(testing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(unit_tests example/unit_1.cpp example/unit_2.cpp example/unit_3.cpp)
target_compile_options(unit_tests PRIVATE 
    $<$<CXX_COMPILER_ID:GNU,Clang>:-freflection>
    $<$<CXX_COMPILER_ID:Clang>:-fexpansion-statements>
)