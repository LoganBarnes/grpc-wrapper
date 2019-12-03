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
cmake_minimum_required(VERSION 3.14)
project(gRPC-Wrapper LANGUAGES CXX)

option(GRPCW_BUILD_TESTS "Build unit tests" OFF)
option(GRPCW_BUILD_EXAMPLE "Build an example server and client" OFF)
option(GRPCW_USE_DEV_FLAGS "Compile with all the flags" OFF)
option(GRPCW_SKIP_PACKAGE_FINDING "Assume gRPC and Protobuf have already been found by CMake" OFF)
option(GRPCW_THREAD_SANITIZATION "Add thread sanitizer flags (only in debug mode)" OFF)

#############################
### Project Configuration ###
#############################
set(GRPCW_CXX_STANDARD 14)

# ccache
find_program(GRPCW_CCACHE_PROGRAM ccache)
if (GRPCW_CCACHE_PROGRAM)
    message("-- Found CCache: ${GRPCW_CCACHE_PROGRAM}")
else ()
    message("-- Not Found: CCache")
endif ()

if (MSVC)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS) # silly microsoft
    set(SYSTEM_FLAGS /experimental:external /external:anglebrackets /external:W0)
endif ()

if (GRPCW_USE_DEV_FLAGS)
    if (NOT MSVC)
        set(GRPCW_COMPILE_FLAGS
                $<$<COMPILE_LANGUAGE:CXX>:-Wall>
                $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
                $<$<COMPILE_LANGUAGE:CXX>:-Werror>
                $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
                $<$<COMPILE_LANGUAGE:CXX>:-Wunused>
                $<$<COMPILE_LANGUAGE:CXX>:-pedantic-errors>
                $<$<COMPILE_LANGUAGE:CXX>:-Winit-self>
                $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
                $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
                $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
                $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
                $<$<COMPILE_LANGUAGE:CXX>:-Wmissing-declarations>
                $<$<COMPILE_LANGUAGE:CXX>:-Wmissing-include-dirs>
                $<$<COMPILE_LANGUAGE:CXX>:-Wno-unknown-pragmas>
                )
    else ()
        set(GRPCW_COMPILE_FLAGS $<$<COMPILE_LANGUAGE:CXX>:/WX>)
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
set(GRPCW_LIBRARY grpc_wrapper)

file(GLOB_RECURSE GRPCW_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/include/grpcw/*
        ${CMAKE_CURRENT_LIST_DIR}/src/grpcw/*
        )

set(GRPCW_PUBLIC_LIB_DEPS
        protobuf::libprotobuf
        gRPC::grpc++
        ${CMAKE_DL_LIBS}
        Threads::Threads
        )

add_library(${GRPCW_LIBRARY} ${GRPCW_SOURCE_FILES})
target_link_libraries(${GRPCW_LIBRARY}
        PUBLIC ${GRPCW_PUBLIC_LIB_DEPS}
        PRIVATE doctest
        )
target_compile_options(${GRPCW_LIBRARY} PRIVATE ${GRPCW_COMPILE_FLAGS})
target_compile_definitions(${GRPCW_LIBRARY} PRIVATE -DDOCTEST_CONFIG_DISABLE)

if (GRPCW_HAS_PARENT)
    target_include_directories(${GRPCW_LIBRARY} SYSTEM
            PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>"
            )
else ()
    target_include_directories(${GRPCW_LIBRARY}
            PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>"
            )
endif ()

if (GRPCW_THREAD_SANITIZATION)
    target_compile_options(${GRPCW_LIBRARY} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:DEBUG>:-fsanitize=thread>>)
    target_link_options(${GRPCW_LIBRARY} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:DEBUG>:-fsanitize=thread>>)
endif ()

###############
### Testing ###
###############
if (GRPCW_BUILD_TESTS)
    set(GRPCW_TESTS test_${GRPCW_LIBRARY})

    create_proto_library(
            testing_protos
            ${CMAKE_CURRENT_LIST_DIR}/protos/testing
            ${CMAKE_BINARY_DIR}/generated/protos
    )

    file(GLOB_RECURSE GRPCW_TEST_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_LIST_DIR}/src/testing/*
            )

    # Create an executable to run the tests
    add_executable(${GRPCW_TESTS} ${GRPCW_SOURCE_FILES} ${GRPCW_TEST_FILES})
    target_link_libraries(${GRPCW_TESTS}
            PRIVATE doctest_with_main
            PRIVATE ${GRPCW_PUBLIC_LIB_DEPS}
            PRIVATE testing_protos
            )
    target_compile_options(${GRPCW_TESTS} PRIVATE ${GRPCW_COMPILE_FLAGS})

    if (GRPCW_THREAD_SANITIZATION)
        target_compile_options(${GRPCW_TESTS} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:DEBUG>:-fsanitize=thread>>)
        target_link_options(${GRPCW_TESTS} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:DEBUG>:-fsanitize=thread>>)
    endif ()

    add_test(NAME ${GRPCW_LIBRARY}_tests COMMAND ${GRPCW_TESTS})

    if (COMMAND SETUP_TARGET_FOR_COVERAGE)
        # Create a build that generates coverage reports
        setup_target_for_coverage(${GRPCW_TESTS}_coverage ${GRPCW_TESTS} ${GRPCW_TESTS}_coverage)
        apply_coverage_dependencies(${GRPCW_TESTS})
    endif ()
endif ()


set_target_properties(
        ${GRPCW_LIBRARY}
        ${GRPCW_TESTS}
        PROPERTIES
        # C++ flags
        CXX_STANDARD ${GRPCW_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        # CCache
        COMPILER_LAUNCHER "${GRPCW_CCACHE_PROGRAM}"
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
