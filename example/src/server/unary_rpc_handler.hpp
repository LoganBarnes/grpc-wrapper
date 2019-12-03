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
struct UnaryRpcHandler : NewRpc, UnaryWriter<Response> {
    using OnRequest = std::function<void(const Request&, UnaryWriter<Response>*)>;

    UnaryRpcHandler(Service& service,
                    grpc::ServerCompletionQueue& queue,
                    Tagger& tagger,
                    std::mutex& mutex,
                    std::thread::id parent_thread_id,
                    AsyncUnaryRpc<Service, Request, Response> register_rpc,
                    OnRequest on_request);
    ~UnaryRpcHandler() override;

    // START NewRpc
    auto clone() -> std::unique_ptr<NewRpc> override;
    auto queue_handler() -> void override;
    auto invoke_user_callback() -> void override;
    // End NewRpc

    // START UnaryWriter
    auto finish(const Response& response, const grpc::Status& status) -> void override;
    auto finish_with_error(const grpc::Status& status) -> void override;
    // END UnaryWriter

private:
    Service& service_;
    grpc::ServerCompletionQueue& queue_;
    Tagger& tagger_;

    std::mutex& mutex_;
    std::thread::id parent_thread_id_;

    UnaryRpcConnection<Request, Response> connection_;
    AsyncUnaryRpc<Service, Request, Response> register_rpc_;
    OnRequest on_request_;
};

template <typename Service, typename Request, typename Response>
UnaryRpcHandler<Service, Request, Response>::UnaryRpcHandler(Service& service,
                                                             grpc::ServerCompletionQueue& queue,
                                                             Tagger& tagger,
                                                             std::mutex& mutex,
                                                             std::thread::id parent_thread_id,
                                                             AsyncUnaryRpc<Service, Request, Response> register_rpc,
                                                             OnRequest on_request)
    : service_(service),
      queue_(queue),
      tagger_(tagger),
      mutex_(mutex),
      parent_thread_id_(parent_thread_id),
      register_rpc_(register_rpc),
      on_request_(std::move(on_request)) {}

template <typename Service, typename Request, typename Response>
UnaryRpcHandler<Service, Request, Response>::~UnaryRpcHandler() = default;

/*
 * START NewRpc
 */
template <typename Service, typename Request, typename Response>
auto UnaryRpcHandler<Service, Request, Response>::clone() -> std::unique_ptr<NewRpc> {
    return std::make_unique<UnaryRpcHandler<Service, Request, Response>>(this->service_,
                                                                         this->queue_,
                                                                         this->tagger_,
                                                                         this->mutex_,
                                                                         this->parent_thread_id_,
                                                                         this->register_rpc_,
                                                                         this->on_request_);
}

template <typename Service, typename Request, typename Response>
auto UnaryRpcHandler<Service, Request, Response>::queue_handler() -> void {
    auto tag = tagger_.make_tag(this, TagLabel::ServerNewRpc);
    (service_.*register_rpc_)(&connection_.context, &connection_.request, &connection_.writer, &queue_, &queue_, tag);
}

template <typename Service, typename Request, typename Response>
auto UnaryRpcHandler<Service, Request, Response>::invoke_user_callback() -> void {
    on_request_(connection_.request, this);
}
/*
 * End NewRpc
 */

/*
 * START UnaryWriter
 */
template <typename Service, typename Request, typename Response>
auto UnaryRpcHandler<Service, Request, Response>::finish(const Response& response, const grpc::Status& status) -> void {
    std::cout << "Parent: " << parent_thread_id_ << std::endl;
    std::cout << "Current: " << std::this_thread::get_id() << std::endl;
    std::cout << "Parent == Current: " << std::boolalpha << (parent_thread_id_ == std::this_thread::get_id())
              << std::endl;

    if (parent_thread_id_ != std::this_thread::get_id()) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto tag = tagger_.make_tag(this, TagLabel::ServerDone);
        connection_.writer.Finish(response, status, tag);
    } else {
        auto tag = tagger_.make_tag(this, TagLabel::ServerDone);
        connection_.writer.Finish(response, status, tag);
    }
}

template <typename Service, typename Request, typename Response>
auto UnaryRpcHandler<Service, Request, Response>::finish_with_error(const grpc::Status& status) -> void {
    if (parent_thread_id_ != std::this_thread::get_id()) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto tag = tagger_.make_tag(this, TagLabel::ServerDone);
        connection_.writer.FinishWithError(status, tag);
    } else {
        auto tag = tagger_.make_tag(this, TagLabel::ServerDone);
        connection_.writer.FinishWithError(status, tag);
    }
}
/*
 * END UnaryWriter
 */

template <typename Service, typename Request, typename Response>
auto make_unique_unary_rpc(Service& service,
                           grpc::ServerCompletionQueue& queue,
                           Tagger& tagger,
                           std::mutex& mutex,
                           std::thread::id parent_thread_id,
                           AsyncUnaryRpc<Service, Request, Response> register_rpc,
                           typename UnaryRpcHandler<Service, Request, Response>::OnRequest on_request)
    -> std::unique_ptr<UnaryRpcHandler<Service, Request, Response>> {

    return std::make_unique<UnaryRpcHandler<Service, Request, Response>>(service,
                                                                         queue,
                                                                         tagger,
                                                                         mutex,
                                                                         parent_thread_id,
                                                                         register_rpc,
                                                                         std::move(on_request));
}

} // namespace server
} // namespace grpcw