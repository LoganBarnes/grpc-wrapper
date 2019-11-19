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
include(FetchContent)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

### Threads ###
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

### Protobuf/gRPC ###
if (NOT ${GRPCW_SKIP_PACKAGE_FINDING})
    find_package(Protobuf REQUIRED)
    find_package(gRPC REQUIRED)
endif ()

### DocTest ###
FetchContent_Declare(
        doctest_dl
        GIT_REPOSITORY https://github.com/onqtam/doctest.git
        GIT_TAG 2.3.5
)

FetchContent_GetProperties(doctest_dl)
if (NOT doctest_dl_POPULATED)
    FetchContent_Populate(doctest_dl)

    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

    # compile with current project
    add_subdirectory(${doctest_dl_SOURCE_DIR} ${doctest_dl_BINARY_DIR} EXCLUDE_FROM_ALL)

    # add test coverage capabilities if available
    find_program(LCOV_EXE
            NAMES "lcov"
            DOC "Path to lcov executable"
            )

    if (LCOV_EXE AND CMAKE_COMPILER_IS_GNUCXX AND CMAKE_BUILD_TYPE MATCHES "Debug")
        include(cmake/CodeCoverage.cmake)
    endif ()

    if (CNC_BUILD_TESTS)
        include(CTest)
    endif ()
endif ()
