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

// grpcw
#include "grpcw/forward_declarations.hpp"

// third-party
#include <grpc++/channel.h>
#include <grpc++/server.h>

// standard
#include <memory>
#include <thread>

namespace grpcw {
namespace server {

class ScopedGrpcServer {
public:
    /**
     * @brief The server will be running when the constructor finishes
     */
    explicit ScopedGrpcServer(std::shared_ptr<grpc::Service> service, const std::string& server_address = "");
    ~ScopedGrpcServer();

    /**
     * @brief This can be used to create in-process clients
     */
    grpc::Server& server();

private:
    std::unique_ptr<GrpcServer> server_;
    std::thread run_thread_;
};

} // namespace server
} // namespace grpcw