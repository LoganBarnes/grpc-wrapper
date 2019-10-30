##########################################################################################
# gRPC Wrapper
# Copyright (c) 2019 Logan Barnes - All Rights Reserved
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
##########################################################################################
cmake_minimum_required(VERSION 3.12)
project(gRPC-Wrapper LANGUAGES CXX)

option(GRPCW_BUILD_TESTS "Build unit tests" OFF)
option(GRPCW_BUILD_EXAMPLE "Build an example server and client" OFF)
option(GRPCW_USE_DEV_FLAGS "Compile with all the flags" OFF)
option(GRPCW_SKIP_PACKAGE_FINDING "Assume gRPC and Protobuf have already been found by CMake" OFF)

#############################
### Project Configuration ###
#############################
set(GRPCW_CXX_STANDARD 14)

if (MSVC)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS) # silly microsoft
    set(SYSTEM_FLAGS /experimental:external /external:anglebrackets /external:W0)
endif ()

if (GRPCW_USE_DEV_FLAGS)
    if (NOT MSVC)
        set(GRPCW_COMPILE_FLAGS
                -Wall -Wextra -Werror -Wpedantic -Wunused -pedantic-errors -Winit-self
                -Wold-style-cast -Woverloaded-virtual -Wsign-conversion -Wshadow
                -Wmissing-declarations -Wmissing-include-dirs -Wno-unknown-pragmas
                )
    else ()
        set(GRPCW_COMPILE_FLAGS /WX)
    endif ()
endif ()

include(cmake/ClangTidy.cmake)
include(cmake/ThirdParty.cmake)
include(cmake/CreateProtoLibrary.cmake)

# "Glob is terrible/root of all evil" yeah yeah. CONFIGURE_DEPENDS in cmake 3.12
# helps to fix that and it is super useful when refactoring
cmake_policy(SET CMP0009 NEW)

get_directory_property(GRPCW_HAS_PARENT PARENT_DIRECTORY)

####################
### gRPC Wrapper ###
####################
set(GRPCW_LIB_NAME grpc-wrapper)

file(GLOB_RECURSE SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/include/grpcw/*
        ${CMAKE_CURRENT_LIST_DIR}/src/grpcw/*
        )

add_library(${GRPCW_LIB_NAME} ${SOURCE_FILES})

if (GRPCW_HAS_PARENT)
    target_include_directories(${GRPCW_LIB_NAME} SYSTEM
            PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>"
            )
else ()
    target_include_directories(${GRPCW_LIB_NAME}
            PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>"
            )
endif ()

target_link_libraries(${GRPCW_LIB_NAME}
        PUBLIC protobuf::libprotobuf
        PUBLIC gRPC::grpc++
        PUBLIC ${CMAKE_DL_LIBS}
        PUBLIC Threads::Threads
        PRIVATE doctest
        )

target_compile_options(${GRPCW_LIB_NAME} PRIVATE ${GRPCW_COMPILE_FLAGS})

###############
### Testing ###
###############
if (GRPCW_BUILD_TESTS) # BUILDING WITH TESTS
    create_proto_library(
            testing-protos
            ${CMAKE_CURRENT_LIST_DIR}/protos/testing
            ${CMAKE_BINARY_DIR}/generated/protos
    )

    file(GLOB_RECURSE TEST_SOURCE_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_LIST_DIR}/src/testing/*
            )

    # Create an executable to run the tests
    set(GRPCW_TESTS_NAME ${GRPCW_LIB_NAME}-tests)
    add_executable(${GRPCW_TESTS_NAME} ${TEST_SOURCE_FILES})

    target_include_directories(${GRPCW_TESTS_NAME} PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>")
    target_link_libraries(${GRPCW_TESTS_NAME}
            PRIVATE
            doctest_with_main
            ${GRPCW_LIB_NAME}
            testing-protos
            )
    target_compile_options(${GRPCW_TESTS_NAME} PRIVATE ${GRPCW_COMPILE_FLAGS})

    add_test(NAME test-${GRPCW_LIB_NAME} COMMAND ${GRPCW_TESTS_NAME})

    if (COMMAND SETUP_TARGET_FOR_COVERAGE)
        # Create a build that generates coverage reports
        setup_target_for_coverage(${GRPCW_TESTS_NAME}-coverage ${GRPCW_TESTS_NAME} coverage)
        apply_coverage_dependencies(${GRPCW_LIB_NAME})
        apply_coverage_dependencies(${GRPCW_TESTS_NAME})
    endif ()

else () # NOT BUILDING WITH TESTS
    # The testing library is linked but we disable all the test macros
    target_compile_definitions(${GRPCW_LIB_NAME} PRIVATE -DDOCTEST_CONFIG_DISABLE)
endif ()

set_target_properties(
        ${GRPCW_LIB_NAME}
        ${GRPCW_TESTS_NAME}
        PROPERTIES
        # C++ flags
        CXX_STANDARD ${GRPCW_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        # Clang-Tidy
        CXX_CLANG_TIDY "${GRPCW_CLANG_TIDY}"
        # Binary locations
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

###############
### Example ###
###############
if (GRPCW_BUILD_EXAMPLE)
    add_subdirectory(example)
endif ()
