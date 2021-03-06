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
#include <grpc++/server.h>

// generated
#include <example.grpc.pb.h>

// standard
#include <thread>

namespace example {

class ExampleServer {
public:
    explicit ExampleServer(const std::string& server_address = "");
    ~ExampleServer();

    grpc::Server& server();

private:
    using Service = protocol::Clock::AsyncService;

    using TimePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
    const TimePoint server_start_time_;

    // send back updates to clients
    grpcw::server::StreamInterface<protocol::Time>* time_stream_;

    std::thread ticker_ = {};
    std::atomic_bool keep_ticking_;

    std::unique_ptr<grpcw::server::GrpcAsyncServer<Service>> server_;

    grpc::Status get_time(const protocol::FormatRequest& request, protocol::Time* time);
};

} // namespace example