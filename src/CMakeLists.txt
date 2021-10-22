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

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cpp error.cpp lexer.cpp parser.cpp codegen.cpp ../include/KaleidoscopeJIT.h codegen.h)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(t ${SOURCE_FILES} ../include/KaleidoscopeJIT.h codegen.h)

llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(t ${llvm_libs})