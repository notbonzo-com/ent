cmake_minimum_required(VERSION 3.31)
project(ent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(ent source/main.cpp
        source/Preprocessor.hh
        source/Preprocessor.cc
        source/Error.hh
        source/Lexer.cc
        source/Lexer.hh
        source/Parser.cc
        source/Parser.hh
        source/AST.icc
        source/AST.cc
        source/Codegen.cc
        source/Codegen.hh
)

target_link_libraries(ent PRIVATE ${LLVM_LIBRARIES} LLVM)
