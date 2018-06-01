cmake_minimum_required(VERSION 3.11)

project(c++lintaur VERSION 0.0.0 LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM include dir is ${LLVM_INCLUDE_DIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CXXSOURCES name-checker.cc)
set(EXAMPLES example.cc)

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(${LLVM_BUILD_MAIN_SRC_DIR}/tools/clang/include)
include_directories(${LLVM_BUILD_BINARY_DIR}/tools/clang/include)

add_executable(example ${EXAMPLES})
add_executable(c++lintaur ${CXXSOURCES})
target_link_libraries(c++lintaur LLVM)
target_link_libraries(c++lintaur clang
                                 clangAST
                                 clangASTMatchers
                                 clangAnalysis
                                 clangBasic
                                 clangDriver
                                 clangEdit
                                 clangFormat
                                 clangFrontend
                                 clangFrontendTool
                                 clangLex
                                 clangParse
                                 clangRewrite
                                 clangRewriteFrontend
                                 clangSema
                                 clangSerialization
                                 clangStaticAnalyzerCheckers
                                 clangStaticAnalyzerCore
                                 clangStaticAnalyzerFrontend
                                 clangTooling
                                 clangToolingCore)
