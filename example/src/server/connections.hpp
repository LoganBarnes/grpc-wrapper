// ///////////////////////////////////////////////////////////////////////////////////////
// gRPC Wrapper
// Copyright (c) 2019 Logan Barnes - All Rights Reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ///////////////////////////////////////////////////////////////////////////////////////
#pragma once

// third-party
#include <grpc++/server.h>
#include <grpc++/server_context.h>

namespace grpcw {
namespace server {

/**
 * @brief The function signature for a service's unary calls
 */
template <typename Service, typename Request, typename Response>
using AsyncUnaryRpc = void (Service::*)(grpc::ServerContext*,
                                        Request*,
                                        grpc::ServerAsyncResponseWriter<Response>*,
                                        grpc::CompletionQueue*,
                                        grpc::ServerCompletionQueue*,
                                        void*);

/**
 * @brief The function signature for a service's server-side-streaming calls
 */
template <typename Service, typename Request, typename Response>
using AsyncServerStreamRpc = void (Service::*)(grpc::ServerContext* context,
                                               Request*,
                                               grpc::ServerAsyncWriter<Response>*,
                                               grpc::CompletionQueue*,
                                               grpc::ServerCompletionQueue*,
                                               void*);

template <typename Request, typename Response>
struct UnaryRpcConnection {
    grpc::ServerContext context;
    Request request;
    grpc::ServerAsyncResponseWriter<Response> writer;

    UnaryRpcConnection() : writer(&context) {}
};

template <typename Request, typename Response>
struct ServerStreamRpcConnection {
    grpc::ServerContext context;
    Request request;
    grpc::ServerAsyncWriter<Response> writer;

    ServerStreamRpcConnection() : writer(&context) {}
};

} // namespace server
} // namespace grpcw