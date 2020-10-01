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
#include "grpcw/util/blocking_queue.hpp"
#include "server_stream_rpc_handler.hpp"
#include "unary_rpc_handler.hpp"

// third-party
#include <grpc++/server_builder.h>

namespace grpcw {
namespace server {

template <typename Service>
class AsyncServer {
public:
    explicit AsyncServer(const std::string& host_address);
    ~AsyncServer();

    template <typename BaseService, typename Request, typename Response>
    auto register_async(AsyncUnaryRpc<BaseService, Request, Response> register_rpc,
                        typename UnaryRpcHandler<BaseService, Request, Response>::OnRequest on_request) -> void;

    auto start() -> void;

private:
    std::unique_ptr<Service> service_;

    std::unique_ptr<grpc::ServerCompletionQueue> server_queue_;
    std::unique_ptr<grpc::Server> server_;

    std::unordered_map<void*, std::unique_ptr<Rpc>> data_;
    Tagger tagger_;

    bool running_ = false;
    util::BlockingQueue<std::function<void()>> initialization_queue_; ///< All initialization happens via this queue
    std::thread run_thread_; ///< Everything in this class happens on this thread except the server shutdown

    std::mutex shutdown_mutex_;
    grpc::ServerCompletionQueue* shutdown_server_queue_;
    grpc::Server* shutdown_server_;

    auto run() -> void;

    template <typename R, typename = std::enable_if_t<std::is_base_of<Rpc, R>::value>>
    auto queue_rpc(std::unique_ptr<R> rpc) -> R*;
};

template <typename Service>
AsyncServer<Service>::AsyncServer(const std::string& host_address) {

    run_thread_ = std::thread([this] {
        while (auto init_task = initialization_queue_.pop_front()) {
            std::lock_guard<std::mutex> lock(shutdown_mutex_);
            init_task();
        }

        run();
    });

    initialization_queue_.emplace_back([this, host_address] {
        service_ = std::make_unique<Service>();

        grpc::ServerBuilder builder;
        builder.RegisterService(service_.get());
        builder.AddListeningPort(host_address, grpc::InsecureServerCredentials());

        server_queue_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
    });

    initialization_queue_.emplace_back([this, host_address] {
        shutdown_server_queue_ = server_queue_.get();
        shutdown_server_ = server_.get();
    });
}

template <typename Service>
AsyncServer<Service>::~AsyncServer() {
    {
        std::lock_guard<std::mutex> lock(shutdown_mutex_);
        shutdown_server_->Shutdown();
        shutdown_server_queue_->Shutdown();
    }
    run_thread_.join();
}

template <typename Service>
template <typename BaseService, typename Request, typename Response>
auto AsyncServer<Service>::register_async(
    AsyncUnaryRpc<BaseService, Request, Response> rpc,
    typename UnaryRpcHandler<BaseService, Request, Response>::OnRequest on_request) -> void {

    if (running_) {
        throw std::runtime_error("Cannot register functions after starting the server");
    }

    initialization_queue_.emplace_back([this, rpc, on_request = std::move(on_request)] {
        queue_rpc(make_unique_unary_rpc(*service_,
                                        *server_queue_,
                                        tagger_,
                                        shutdown_mutex_,
                                        std::this_thread::get_id(),
                                        rpc,
                                        std::move(on_request)));
    });
}

template <typename Service>
auto AsyncServer<Service>::start() -> void {
    initialization_queue_.emplace_back(nullptr);
    running_ = true;
}

template <typename Service>
auto AsyncServer<Service>::run() -> void {
    void* received_tag;
    bool completed_successfully;

    while (server_queue_->Next(&received_tag, &completed_successfully)) {
        std::lock_guard<std::mutex> lock(shutdown_mutex_);

        Tag tag = tagger_.get_tag(received_tag);

        std::cout << "S " << (completed_successfully ? "Success: " : "Failure: ") << tag << std::endl;

        if (completed_successfully) {
            switch (tag.label) {

            case TagLabel::ServerNewRpc: {
                auto rpc = static_cast<NewRpc*>(tag.data);
                queue_rpc(rpc->clone());
                rpc->invoke_user_callback();

            } break;

            case TagLabel::ServerWriting:
                break;

            case TagLabel::ServerDone:
                data_.erase(tag.data);
                break;

            case TagLabel::ClientConnectionChange:
            case TagLabel::ClientFinished:
                throw std::runtime_error("Don't use client tags in the server ya dummy");

            } // end switch

        } else {
            data_.erase(tag.data);
        }
    }
}

template <typename Service>
template <typename R, typename>
auto AsyncServer<Service>::queue_rpc(std::unique_ptr<R> rpc) -> R* {
    auto* rpc_ptr = rpc.get();
    data_.emplace(rpc_ptr, std::move(rpc));
    rpc_ptr->queue_handler();
    return rpc_ptr;
}

} // namespace server
} // namespace grpcw