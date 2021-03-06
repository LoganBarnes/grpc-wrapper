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
create_proto_library(ltb_grpcw_example_protos
        ${CMAKE_CURRENT_LIST_DIR}/protos
        ${CMAKE_BINARY_DIR}/generated/protos
        )

ltb_add_executable(example_client
        17
        ${CMAKE_CURRENT_LIST_DIR}/src/example_client.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/example_client.hpp
        )
ltb_add_executable(example_server
        17
        ${CMAKE_CURRENT_LIST_DIR}/src/example_server.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/example_server.hpp
        )

target_link_libraries(example_client PUBLIC ltb_grpcw_example_protos)
target_link_libraries(example_server PUBLIC ltb_grpcw_example_protos)

########################
### New Architecture ###
########################

### Source Files ###
set(GRPCW_TMP_SERVER_MAIN ${CMAKE_CURRENT_LIST_DIR}/src/server/new_arch_server.cpp)
set(GRPCW_TMP_CLIENT_MAIN ${CMAKE_CURRENT_LIST_DIR}/src/client/new_arch_client.cpp)

file(GLOB_RECURSE GRPCW_TMP_SHARED_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/src/shared/*
        )

file(GLOB_RECURSE GRPCW_TMP_SERVER_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/src/server/*
        )
list(REMOVE_ITEM GRPCW_TMP_SERVER_SOURCE_FILES ${GRPCW_TMP_SERVER_MAIN})

file(GLOB_RECURSE GRPCW_TMP_CLIENT_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/src/client/*
        )
list(REMOVE_ITEM GRPCW_TMP_CLIENT_SOURCE_FILES ${GRPCW_TMP_CLIENT_MAIN})

### Executables ###
ltb_add_executable(new_arch_server
        17
        ${GRPCW_TMP_SERVER_MAIN}
        ${GRPCW_TMP_SERVER_SOURCE_FILES}
        ${GRPCW_TMP_SHARED_SOURCE_FILES}
        )
ltb_add_executable(new_arch_client
        17
        ${GRPCW_TMP_CLIENT_MAIN}
        ${GRPCW_TMP_CLIENT_SOURCE_FILES}
        ${GRPCW_TMP_SHARED_SOURCE_FILES}
        )

target_link_libraries(new_arch_server PUBLIC ltb_grpcw_example_protos)
target_link_libraries(new_arch_client PUBLIC ltb_grpcw_example_protos)
