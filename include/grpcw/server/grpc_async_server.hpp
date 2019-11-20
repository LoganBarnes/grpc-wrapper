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
#include "grpcw/server/detail/non_stream_rpc_handler.hpp"
#include "grpcw/server/detail/stream_rpc_handler_callback_setter.hpp"
#include "grpcw/util/atomic_data.hpp"

// third-party
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>

namespace grpcw {
namespace server {

template <typename Request>
using DefaultStreamCallback = void (*)(const Request&);

template <typename Service>
class GrpcAsyncServer {
public:
    explicit GrpcAsyncServer(std::shared_ptr<Service> service, const std::string& address);
    ~GrpcAsyncServer();

    template <typename BaseService, typename Request, typename Response, typename Callback>
    void register_async(AsyncNoStreamFunc<BaseService, Request, Response> no_stream_func, Callback&& callback);

    /**
     * @brief StreamInterface* should stop being used before GrpcAsyncServer is destroyed
     */
    template <typename BaseService, typename Request, typename Response>
    detail::StreamRpcHandlerCallbackSetter<BaseService, Request, Response>
    register_async_stream(AsyncServerStreamFunc<BaseService, Request, Response> stream_func);

    // This is also called in the destructor
    void shutdown_and_wait();

    template <typename Duration>
    void force_shutdown_in(Duration duration);

    grpc::Server& server();

private:
    std::shared_ptr<Service> service_;
    std::unique_ptr<grpc::ServerCompletionQueue> server_queue_;
    std::unique_ptr<grpc::Server> server_;

    using RpcMap = std::unordered_map<void*, std::unique_ptr<detail::AsyncRpcHandlerInterface>>;
    util::AtomicData<RpcMap> rpc_handlers_;

    std::thread run_thread_;
};

template <typename Service>
GrpcAsyncServer<Service>::GrpcAsyncServer(std::shared_ptr<Service> service, const std::string& address)
    : service_(std::move(service)) {

    grpc::ServerBuilder builder;
    builder.RegisterService(service_.get());
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.SetMaxMessageSize(std::numeric_limits<int>::max());
    server_queue_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();

    run_thread_ = std::thread([&] {
        void* tag;
        bool call_ok;

        while (server_queue_->Next(&tag, &call_ok)) {
            if (call_ok) {
                rpc_handlers_.use_safely([&](RpcMap& rpc_handlers) { rpc_handlers.at(tag)->activate_next(); });
            } else {
                rpc_handlers_.use_safely([&](RpcMap& rpc_handlers) { rpc_handlers.erase(tag); });
            }
        }
    });
}

template <typename Service>
GrpcAsyncServer<Service>::~GrpcAsyncServer() {
    shutdown_and_wait();
    server_queue_->Shutdown();

    run_thread_.join();
}

template <typename Service>
template <typename BaseService, typename Request, typename Response, typename Callback>
void GrpcAsyncServer<Service>::register_async(AsyncNoStreamFunc<BaseService, Request, Response> no_stream_func,
                                              Callback&& callback) {
    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");
    auto handler = std::make_unique<
        detail::NonStreamRpcHandler<BaseService, Request, Response, Callback>>(*service_,
                                                                               *server_queue_,
                                                                               no_stream_func,
                                                                               std::forward<Callback>(callback));

    void* tag = handler.get();
    rpc_handlers_.use_safely([&](RpcMap& rpc_handlers) { rpc_handlers.emplace(tag, std::move(handler)); });
}

template <typename Service>
template <typename BaseService, typename Request, typename Response>
auto GrpcAsyncServer<Service>::register_async_stream(AsyncServerStreamFunc<BaseService, Request, Response> stream_func)
    -> detail::StreamRpcHandlerCallbackSetter<BaseService, Request, Response> {

    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");
    auto handler = std::make_unique<detail::StreamRpcHandler<BaseService, Request, Response>>(*service_,
                                                                                              *server_queue_,
                                                                                              stream_func);
    auto* tag = handler.get();
    rpc_handlers_.use_safely([&](RpcMap& rpc_handlers) { rpc_handlers.emplace(tag, std::move(handler)); });
    return {tag};
}

template <typename Service>
void GrpcAsyncServer<Service>::shutdown_and_wait() {
    server_->Shutdown();
}

template <typename Service>
template <typename Duration>
void GrpcAsyncServer<Service>::force_shutdown_in(Duration duration) {
    server_->Shutdown(std::chrono::system_clock::now() + duration);
}

template <typename Service>
grpc::Server& GrpcAsyncServer<Service>::server() {
    return *server_;
}

} // namespace server
} // namespace grpcw
