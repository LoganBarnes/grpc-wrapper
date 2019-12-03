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

// project
#include "connections.hpp"
#include "rpc.hpp"
#include "tagger.hpp"

// third-party
#include <grpc++/support/status.h>

// standard
#include <memory>

namespace grpcw {
namespace server {

template <typename Service, typename Request, typename Response>
struct ServerStreamRpcHandler : NewRpc, ServerStreamWriter<Response> {
    using OnRequest = std::function<void(const Request&, ServerStreamWriter<Response>*)>;

    ServerStreamRpcHandler(Service& service,
                           grpc::ServerCompletionQueue& queue,
                           Tagger& tagger,
                           AsyncServerStreamRpc<Service, Request, Response> register_rpc,
                           OnRequest on_request);
    ~ServerStreamRpcHandler() override;

    // START NewRpc
    auto clone() -> std::unique_ptr<NewRpc> override;
    auto queue_handler() -> void override;
    auto invoke_user_callback() -> void override;
    // End NewRpc

    // START ServerStreamWriter
    auto write(const Response& response) -> void override;
    auto finish(const grpc::Status& status) -> void override;
    // END ServerStreamWriter

private:
    Service& service_;
    grpc::ServerCompletionQueue& queue_;
    Tagger& tagger_;

    std::mutex mutex_;

    ServerStreamRpcConnection<Request, Response> connection_;
    AsyncServerStreamRpc<Service, Request, Response> register_rpc_;
    OnRequest on_request_;
};

template <typename Service, typename Request, typename Response>
ServerStreamRpcHandler<Service, Request, Response>::ServerStreamRpcHandler(
    Service& service,
    grpc::ServerCompletionQueue& queue,
    Tagger& tagger,
    AsyncServerStreamRpc<Service, Request, Response> register_rpc,
    OnRequest on_request)
    : service_(service),
      queue_(queue),
      tagger_(tagger),
      register_rpc_(register_rpc),
      on_request_(std::move(on_request)) {}

template <typename Service, typename Request, typename Response>
ServerStreamRpcHandler<Service, Request, Response>::~ServerStreamRpcHandler() = default;

/*
 * START NewRpc
 */
template <typename Service, typename Request, typename Response>
auto ServerStreamRpcHandler<Service, Request, Response>::clone() -> std::unique_ptr<NewRpc> {
    return std::make_unique<ServerStreamRpcHandler<Service, Request, Response>>(this->service_,
                                                                                this->queue_,
                                                                                this->tagger_,
                                                                                this->register_rpc_,
                                                                                this->on_request_);
}

template <typename Service, typename Request, typename Response>
auto ServerStreamRpcHandler<Service, Request, Response>::queue_handler() -> void {
    auto tag = tagger_.make_tag(this, TagLabel::ServerNewRpc);
    (service_.*register_rpc_)(&connection_.context, &connection_.request, &connection_.writer, &queue_, &queue_, tag);
}

template <typename Service, typename Request, typename Response>
auto ServerStreamRpcHandler<Service, Request, Response>::invoke_user_callback() -> void {
    on_request_(connection_.request, this);
}
/*
 * End NewRpc
 */

/*
 * START ServerStreamWriter
 */
template <typename Service, typename Request, typename Response>
auto ServerStreamRpcHandler<Service, Request, Response>::write(const Response& response) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    auto tag = tagger_.make_tag(this, TagLabel::ServerWriting);
    connection_.writer.Write(response, tag);
}

template <typename Service, typename Request, typename Response>
auto ServerStreamRpcHandler<Service, Request, Response>::finish(const grpc::Status& status) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    auto tag = tagger_.make_tag(this, TagLabel::ServerDone);
    connection_.writer.Finish(status, tag);
}
/*
 * END ServerStreamWriter
 */

template <typename Service, typename Request, typename Response>
auto make_unique_server_stream_rpc(Service& service,
                                   grpc::ServerCompletionQueue& queue,
                                   Tagger& tagger,
                                   AsyncServerStreamRpc<Service, Request, Response> register_rpc,
                                   typename ServerStreamRpcHandler<Service, Request, Response>::OnRequest on_request)
    -> std::unique_ptr<ServerStreamRpcHandler<Service, Request, Response>> {

    return std::make_unique<ServerStreamRpcHandler<Service, Request, Response>>(service,
                                                                                queue,
                                                                                tagger,
                                                                                register_rpc,
                                                                                std::move(on_request));
}

} // namespace server
} // namespace grpcw