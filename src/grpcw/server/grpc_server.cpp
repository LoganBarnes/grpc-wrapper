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
#include "grpcw/server/grpc_server.hpp"

// third-party
#include <grpc++/impl/codegen/service_type.h>
#include <grpc++/server_builder.h>

// standard
#include <utility>

namespace grpcw {
namespace server {

GrpcServer::GrpcServer(std::unique_ptr<grpc::Service> service, const std::string& server_address)
    : service_(std::move(service)) {

    grpc::ServerBuilder builder;
    builder.RegisterService(service_.get());
    builder.SetMaxMessageSize(std::numeric_limits<int>::max());

    if (not server_address.empty()) {
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    }
    server_ = builder.BuildAndStart();
}

GrpcServer::~GrpcServer() = default;

void GrpcServer::run() {
    server_->Wait();
}

void GrpcServer::shutdown() {
    server_->Shutdown();
}

auto GrpcServer::in_process_channel(const grpc_impl::ChannelArguments& channel_arguments)
    -> std::shared_ptr<grpc_impl::Channel> {
    return server_->InProcessChannel(channel_arguments);
}

} // namespace server
} // namespace grpcw
