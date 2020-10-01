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
cmake_minimum_required(VERSION 3.18)
project(LtbGrpcw LANGUAGES CXX)

option(LTB_GRPCW_BUILD_EXAMPLE "Build an example server and client" OFF)
option(LTB_THREAD_SANITIZATION "Add thread sanitizer flags (only in debug mode)" OFF)

include(${CMAKE_CURRENT_LIST_DIR}/ltb-util/cmake/LtbConfig.cmake) # <-- Additional project options are in here.

############
### Util ###
############
add_subdirectory(ltb-util)

########################
### External Targets ###
########################
include(${CMAKE_CURRENT_LIST_DIR}/cmake/CreateProtoLibrary.cmake)

####################
### Source Files ###
####################
file(GLOB_RECURSE LTB_CORE_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/include/grpcw/*
        ${CMAKE_CURRENT_LIST_DIR}/src/grpcw/*
        )

file(GLOB_RECURSE LTB_TEST_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/src/testing/*
        )

#############
### GRPCW ###
#############
ltb_add_library(ltb_grpcw
        17
        ${LTB_CORE_SOURCE_FILES}
        $<$<BOOL:${LTB_BUILD_TESTS}>:${LTB_TEST_SOURCE_FILES}>
        )

ltb_link_libraries(ltb_grpcw
        PUBLIC
        gRPC::grpc++
        Ltb::Util
        )

ltb_include_directories(ltb_grpcw
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
        )

if (${LTB_THREAD_SANITIZATION})
    ltb_compile_options(ltb_grpcw
            PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:DEBUG>:-fsanitize=thread>>
            )
    ltb_link_options(ltb_grpcw
            PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:DEBUG>:-fsanitize=thread>>
            )
endif ()

add_library(Ltb::grpcw ALIAS ltb_grpcw)

###############
### Testing ###
###############
if (${LTB_BUILD_TESTS})
    create_proto_library(ltb_grpcw_testing_protos
            ${CMAKE_CURRENT_LIST_DIR}/protos/testing
            ${CMAKE_BINARY_DIR}/generated/protos
            )

    target_link_libraries(test_ltb_grpcw PRIVATE ltb_grpcw_testing_protos)
endif ()

###############
### Example ###
###############
if (${LTB_GRPCW_BUILD_EXAMPLE})
    add_subdirectory(example)
endif ()
