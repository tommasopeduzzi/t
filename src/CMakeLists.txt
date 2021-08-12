cmake_minimum_required(VERSION 3.0)
project(t)

set(CMAKE_CXX_STANDARD 20)

add_executable(t main.cpp lexer.cpp nodes.h parser.cpp parser.h error.cpp error.h)