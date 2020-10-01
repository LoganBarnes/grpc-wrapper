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
#include "grpcw/util/blocking_queue.hpp"
#include "../shared/tag.hpp"

// generated
#include <example.grpc.pb.h>

// external
#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

// standard
#include <thread>

namespace grpcw {
namespace client {

template <typename Stub, typename Request, typename Response>
using AsyncUnaryFunc = std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> (Stub::*)(grpc::ClientContext*,
                                                                                            const Request&,
                                                                                            grpc::CompletionQueue*);

template <typename Response>
struct UnaryRpcConnection {
    grpc::ClientContext context;
    Response response;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> reader;
};

struct Rpc {
    virtual ~Rpc() = default;
};

struct UnaryRpc : virtual Rpc {
    ~UnaryRpc() override = default;
    virtual auto invoke_user_callback() -> void = 0;
};

template <typename Request>
struct UnaryWriter : virtual Rpc {
    ~UnaryWriter() override = default;
    virtual auto send(const Request& request) -> void = 0;
};

template <typename Stub, typename Request, typename Response>
struct UnaryRpcHandler : UnaryRpc, UnaryWriter<Request> {
    using OnFinish = std::function<void(const grpc::Status&, const Response&)>;

    UnaryRpcHandler(grpc::Channel& channel,
                    Stub& stub,
                    grpc::CompletionQueue& queue,
                    Tagger& tagger,
                    std::mutex& client_mutex,
                    AsyncUnaryFunc<Stub, Request, Response> rpc,
                    OnFinish on_finish)
        : channel_(channel),
          stub_(stub),
          queue_(queue),
          tagger_(tagger),
          client_mutex_(client_mutex),
          rpc_(rpc),
          on_finish_(std::move(on_finish)) {}

    ~UnaryRpcHandler() override = default;

    /*
     * START UnaryRpc
     */
    auto invoke_user_callback() -> void override { on_finish_(connection_.status, connection_.response); }
    /*
     * END UnaryRpc
     */

    /*
     * START UnaryWriter
     */
    auto send(const Request& request) -> void override {
        std::lock_guard<std::mutex> lock(client_mutex_);
        connection_.reader = (stub_.*rpc_)(&connection_.context, request, &queue_);
        auto tag = tagger_.make_tag(this, TagLabel::ClientFinished);
        connection_.reader->Finish(&connection_.response, &connection_.status, tag);
    }
    /*
     * END UnaryWriter
     */

private:
    grpc::Channel& channel_;
    Stub& stub_;
    grpc::CompletionQueue& queue_;
    Tagger& tagger_;

    std::mutex& client_mutex_;

    UnaryRpcConnection<Response> connection_;
    AsyncUnaryFunc<Stub, Request, Response> rpc_;
    OnFinish on_finish_;
};

template <typename Stub, typename Request, typename Response>
auto make_unique_unary_rpc(grpc::Channel& channel,
                           Stub& stub,
                           grpc::CompletionQueue& queue,
                           Tagger& tagger,
                           std::mutex& client_mutex,
                           AsyncUnaryFunc<Stub, Request, Response> register_rpc,
                           typename UnaryRpcHandler<Stub, Request, Response>::OnFinish on_finish)
    -> std::unique_ptr<UnaryRpcHandler<Stub, Request, Response>> {

    return std::make_unique<UnaryRpcHandler<Stub, Request, Response>>(channel,
                                                                      stub,
                                                                      queue,
                                                                      tagger,
                                                                      client_mutex,
                                                                      register_rpc,
                                                                      std::move(on_finish));
}

} // namespace client
} // namespace grpcw
