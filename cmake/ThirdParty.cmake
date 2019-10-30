##########################################################################################
# Copyright (c) 2019 Logan Barnes - All Rights Reserved
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
        GIT_TAG 2.3.4
)

FetchContent_GetProperties(doctest_dl)
if (NOT doctest_dl_POPULATED)
    FetchContent_Populate(doctest_dl)

    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB ON CACHE BOOL "" FORCE)

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
        file(WRITE ${CMAKE_BINARY_DIR}/generated/null.cpp "")
        include(CTest)
    endif ()
endif ()
