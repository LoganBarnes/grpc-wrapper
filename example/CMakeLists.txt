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
create_proto_library(
        example-protos
        ${CMAKE_CURRENT_LIST_DIR}/protos
        ${CMAKE_BINARY_DIR}/generated/protos
)

add_executable(example-client ${CMAKE_CURRENT_LIST_DIR}/src/example_client)
add_executable(example-server ${CMAKE_CURRENT_LIST_DIR}/src/example_server)

target_link_libraries(example-client PRIVATE grpc-wrapper example-protos)
target_link_libraries(example-server PRIVATE grpc-wrapper example-protos)

target_compile_options(example-client PRIVATE ${GRPCW_COMPILE_FLAGS})
target_compile_options(example-server PRIVATE ${GRPCW_COMPILE_FLAGS})

set_target_properties(
        example-client
        example-server
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