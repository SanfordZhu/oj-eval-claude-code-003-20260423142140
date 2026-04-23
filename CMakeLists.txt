cmake_minimum_required(VERSION 3.16)
project(icpc_management_system)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(code src/main.cpp)

# Optimize for OJ
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(code PRIVATE -O2 -pipe -static -s)
endif()
