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

// generated
#include <example.grpc.pb.h>

// external
#include <grpc++/channel.h>

// standard
#include <thread>

namespace example {

class ExampleClient {
public:
    explicit ExampleClient(const std::string& host_address);
    ~ExampleClient();

    std::string get_server_time_now(const protocol::Format& format);

    void toggle_streaming();

private:
    using Service = protocol::Clock;

    std::mutex stream_mutex;
    std::unique_ptr<grpc::ClientContext> stream_context_;
    std::unique_ptr<grpc::ClientReader<example::protocol::Time>> stream_;
    std::thread stream_thread_;

    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<typename Service::Stub> stub_;
};

} // namespace example
