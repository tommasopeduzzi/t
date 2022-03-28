cmake_minimum_required(VERSION 3.13)  # CMake version check
project(t_corefn)                     # Create project "t_corefn"
set(CMAKE_CXX_STANDARD 17)            # Enable c++17 standard

set(SOURCES corefn.cpp)
add_library(t_corefn SHARED ${SOURCES})