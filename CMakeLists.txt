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

#############################
### Project Configuration ###
#############################
set(GRPCW_CXX_STANDARD 11)

if (MSVC)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS) # silly microsoft
    set(SYSTEM_FLAGS /experimental:external /external:anglebrackets /external:W0)
endif ()

if (GRPCW_USE_DEV_FLAGS)
    if (NOT MSVC)
        set(GRPCW_COMPILE_FLAGS
                -Wall -Wextra -Werror -Wpedantic -Wunused -pedantic-errors
                -Winit-self -Wold-style-cast -Woverloaded-virtual -Wsign-conversion
                -Wmissing-declarations -Wmissing-include-dirs -Wshadow -Wno-unknown-pragmas
                )
    else ()
        set(GRPCW_COMPILE_FLAGS /WX)
    endif ()
endif ()

# "Glob is terrible/root of all evil" yeah yeah. CONFIGURE_DEPENDS in cmake 3.12
# helps to fix that and it is super useful when refactoring
cmake_policy(SET CMP0009 NEW)

##################
### Clang-Tidy ###
##################
find_program(CLANG_TIDY_EXE
        NAMES "clang-tidy"
        DOC "Path to clang-tidy executable"
        )
if (NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
else ()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-format-style=file" "-fix" "-fix-errors")
endif ()

###################
### Third Party ###
###################
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

add_subdirectory(third-party)

####################
### gRPC Wrapper ###
####################
file(GLOB_RECURSE SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/include/grpcw/*
        ${CMAKE_CURRENT_LIST_DIR}/src/grpcw/*
        )

add_library(grpc-wrapper ${SOURCE_FILES})

target_include_directories(grpc-wrapper
        PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
        PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src
        )

target_compile_options(grpc-wrapper PRIVATE ${GRPCW_COMPILE_FLAGS})

set_target_properties(
        grpc-wrapper
        PROPERTIES
        CXX_STANDARD ${GRPCW_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

###############
### Testing ###
###############
if (${GRPCW_BUILD_TESTS})
    ### Protobuf Generation ###
    add_subdirectory(protos)

    file(GLOB_RECURSE TEST_SOURCE_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_LIST_DIR}/src/testing/*
            )

    add_executable(grpc-wrapper-tests ${TEST_SOURCE_FILES} src/testing/main.cpp)

    target_link_libraries(grpc-wrapper-tests
            PRIVATE grpc-wrapper
            PRIVATE testing-protos
            PRIVATE doctest
            )
    target_include_directories(grpc-wrapper-tests
            PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src
            )

    ### add coverage capabilities ###
    find_program(
            LCOV_EXE
            NAMES "lcov"
            DOC "Path to lcov executable"
    )

    if (LCOV_EXE AND CMAKE_COMPILER_IS_GNUCXX AND CMAKE_BUILD_TYPE MATCHES "Debug")
        include(CodeCoverage)
        setup_target_for_coverage(grpc-wrapper-coverage grpc-wrapper-tests coverage)
        set(PROFILE_FLAGS --coverage -fprofile-arcs -ftest-coverage -fno-inline)

        target_compile_options(grpc-wrapper PRIVATE ${GRPCW_COMPILE_FLAGS} ${PROFILE_FLAGS})
        target_link_libraries(grpc-wrapper PRIVATE gcov)

        target_compile_options(grpc-wrapper-tests PRIVATE ${GRPCW_COMPILE_FLAGS} ${PROFILE_FLAGS})
        target_link_libraries(grpc-wrapper-tests PRIVATE gcov)
    endif ()

    set_target_properties(
            grpc-wrapper-tests
            PROPERTIES
            CXX_STANDARD ${GRPCW_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            POSITION_INDEPENDENT_CODE ON
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endif ()

###############
### Example ###
###############
if (${GRPCW_BUILD_EXAMPLE})
    add_subdirectory(example)
endif ()
