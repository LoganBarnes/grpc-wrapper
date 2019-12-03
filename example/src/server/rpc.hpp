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
#include <grpc++/support/status.h>

// standard
#include <memory>

namespace grpcw {
namespace server {

struct Rpc {
    virtual ~Rpc() = default;
};

struct NewRpc : virtual Rpc {
    ~NewRpc() override = default;
    virtual auto clone() -> std::unique_ptr<NewRpc> = 0;
    virtual auto queue_handler() -> void = 0;
    virtual auto invoke_user_callback() -> void = 0;
};

template <typename Response>
struct UnaryWriter : virtual Rpc {
    ~UnaryWriter() override = default;
    virtual auto finish(const Response& response, const grpc::Status& status) -> void = 0;
    virtual auto finish_with_error(const grpc::Status& status) -> void = 0;
};

template <typename Response>
struct ServerStreamWriter : virtual Rpc {
    ~ServerStreamWriter() override = default;
    virtual auto write(const Response& response) -> void = 0;
    virtual auto finish(const grpc::Status& status) -> void = 0;
};

} // namespace server
} // namespace grpcw