cmake_minimum_required(VERSION 3.31)
project(ent)

set(CMAKE_CXX_STANDARD 23)

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
)
