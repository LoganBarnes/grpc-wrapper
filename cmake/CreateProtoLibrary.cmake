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
function(create_proto_library target_name proto_dir cpp_output_dir)

    if (NOT DEFINED PROTOBUF_GENERATE_CPP_APPEND_PATH)
        set(PROTOBUF_GENERATE_CPP_APPEND_PATH TRUE)
    endif ()
    if (NOT DEFINED GRPC_GENERATE_CPP_APPEND_PATH)
        set(GRPC_GENERATE_CPP_APPEND_PATH TRUE)
    endif ()

    # make the output directory
    file(MAKE_DIRECTORY ${cpp_output_dir})

    # gather proto files
    file(GLOB_RECURSE PROTO_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${proto_dir}/*.proto
            )

    # generate service objects
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${cpp_output_dir} ${PROTO_FILES})
    grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${cpp_output_dir} ${PROTO_FILES})

    add_library(${target_name} ${PROTO_HDRS} ${PROTO_SRCS} ${GRPC_HDRS} ${GRPC_SRCS})
    target_link_libraries(${target_name} PUBLIC grpc_wrapper)
    target_include_directories(${target_name} SYSTEM PUBLIC ${cpp_output_dir})
endfunction()
