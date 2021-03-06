cmake_minimum_required(VERSION 3.13)  # CMake version check
project(t)               # Create project "t"
set(CMAKE_CXX_STANDARD 17)            # Enable c++17 standard
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add CoreFn library to project
add_subdirectory(corefn)
include_directories(./corefn)

set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(SOURCE_FILES main.cpp error.cpp lexer.cpp parser.cpp codegen.cpp passes.cpp type.cpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(t ${SOURCE_FILES})
llvm_map_components_to_libnames(llvm_libs support core irreader executionengine native codegen orcjit orcshared orctargetprocess)
target_link_libraries(t  ${llvm_libs} t_corefn)
