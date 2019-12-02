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
#include "new_arch_server.hpp"

// grpcw
#include "grpcw/server/grpc_async_server.hpp"
#include "tag.hpp"

// third-party
#include <grpc++/security/server_credentials.h>
#include <grpc++/server_builder.h>

// standard
#include <grpcw/util/blocking_queue.hpp>
#include <sstream>

namespace example {
namespace {
using namespace grpcw;

template <typename Duration>
void make_pretty_time_string(std::ostream& os, const Duration& duration) {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration - hours);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration - hours - minutes);

    os << hours.count() << "h  " << minutes.count() << "m  " << seconds.count() << "s";
}

} // namespace

ExampleServer::ExampleServer(const std::string& server_address)
    : server_start_time_(std::chrono::system_clock::now()),
      keep_ticking_(true),
      server_(std::make_unique<server::GrpcAsyncServer<Service>>(std::make_shared<protocol::Clock::AsyncService>(),
                                                                 server_address)) {

    /*
     * Streaming calls
     */
    time_stream_ = server_->register_async_stream(&Service::RequestGetServerTimeUpdates).stream();

    /*
     * Getters for current state
     */
//    server_->register_async(&Service::RequestGetServerTimeNow,
//                            [this](const protocol::FormatRequest& request, protocol::Time* time) {
//                                return this->get_time(request, time);
//                            });
//
//    keep_ticking_.store(true);
//
//    ticker_ = std::thread([this] {
//        protocol::FormatRequest request;
//        protocol::Time time;
//
//        while (keep_ticking_) {
//            std::this_thread::sleep_for(std::chrono::seconds(1));
//            get_time(request, &time);
//            time_stream_->write(time);
//        }
//    });
}

ExampleServer::~ExampleServer() {
    keep_ticking_.store(false);
    if (ticker_.joinable()) {
        ticker_.join();
    }
    server_->shutdown_and_wait();
}

grpc::Server& ExampleServer::server() {
    return server_->server();
}

namespace {

grpc::Status time_since(const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>& start_time,
                        const protocol::FormatRequest& request,
                        protocol::Time* time) {
    std::stringstream ss;

    auto time_since_server_start = std::chrono::system_clock::now() - start_time;

    switch (request.format()) {
    case protocol::COMPOUND:
        make_pretty_time_string(ss, time_since_server_start);
        break;
    case protocol::HOURS:
        ss << std::chrono::duration_cast<std::chrono::hours>(time_since_server_start).count() << "h";
        break;
    case protocol::MINUTES:
        ss << std::chrono::duration_cast<std::chrono::minutes>(time_since_server_start).count() << "m";
        break;
    case protocol::SECONDS:
        ss << std::chrono::duration_cast<std::chrono::seconds>(time_since_server_start).count() << "s";
        break;

        // We can safely ignore these?
    case protocol::Format_INT_MIN_SENTINEL_DO_NOT_USE_:
    case protocol::Format_INT_MAX_SENTINEL_DO_NOT_USE_:
        break;
    }

    time->set_display_time(ss.str());
    return grpc::Status::OK;
}

} // namespace
} // namespace example

template <typename Service, typename Request, typename Response>
using AsyncUnaryFunc = void (Service::*)(grpc::ServerContext*,
                                         Request*,
                                         grpc::ServerAsyncResponseWriter<Response>*,
                                         grpc::CompletionQueue*,
                                         grpc::ServerCompletionQueue*,
                                         void*);

/**
 * @brief The function signature for a service's server-side-streaming calls
 */
template <typename Service, typename Request, typename Response>
using AsyncServerStreamFunc = void (Service::*)(grpc::ServerContext* context,
                                                Request*,
                                                grpc::ServerAsyncWriter<Response>*,
                                                grpc::CompletionQueue*,
                                                grpc::ServerCompletionQueue*,
                                                void*);

struct Tagger {
    auto make_tag(void* data, TagLabel label) -> void* {
        std::lock_guard<std::mutex> lock(mutex_);
        return detail::make_tag(data, label, &tags_);
    }

    auto get_tag(void* key) -> Tag {
        std::lock_guard<std::mutex> lock(mutex_);
        return detail::get_tag(key, &tags_);
    }

public:
    std::mutex mutex_;
    std::unordered_map<void*, std::unique_ptr<Tag>> tags_;
};

template <typename Request, typename Response>
struct UnaryRpcConnection {
    grpc::ServerContext context;
    Request request;
    grpc::ServerAsyncResponseWriter<Response> responder;

    UnaryRpcConnection() : responder(&context) {}
};

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

template <typename Service, typename Request, typename Response>
struct UnaryRpcHandler : NewRpc, UnaryWriter<Response> {
    using OnRequest = std::function<void(const Request&, UnaryWriter<Response>*)>;

    UnaryRpcHandler(Service& service,
                    grpc::ServerCompletionQueue& queue,
                    Tagger& tagger,
                    AsyncUnaryFunc<Service, Request, Response> register_rpc,
                    OnRequest on_request)
        : service_(service),
          queue_(queue),
          tagger_(tagger),
          register_rpc_(register_rpc),
          on_request_(std::move(on_request)) {}

    ~UnaryRpcHandler() override = default;

    /*
     * START NewRpc
     */
    auto clone() -> std::unique_ptr<NewRpc> override {
        return std::make_unique<UnaryRpcHandler<Service, Request, Response>>(this->service_,
                                                                             this->queue_,
                                                                             this->tagger_,
                                                                             this->register_rpc_,
                                                                             this->on_request_);
    }

    auto queue_handler() -> void override {
        auto tag = tagger_.make_tag(this, TagLabel::ServerNewRpc);
        (service_
         .*register_rpc_)(&connection_.context, &connection_.request, &connection_.responder, &queue_, &queue_, tag);
    }

    auto invoke_user_callback() -> void override { on_request_(connection_.request, this); }
    /*
     * End NewRpc
     */

    /*
     * START UnaryWriter
     */
    auto finish(const Response& response, const grpc::Status& status) -> void override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto tag = tagger_.make_tag(this, TagLabel::ServerDone);
        connection_.responder.Finish(response, status, tag);
    }

    auto finish_with_error(const grpc::Status& status) -> void override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto tag = tagger_.make_tag(this, TagLabel::ServerDone);
        connection_.responder.FinishWithError(status, tag);
    }
    /*
     * END UnaryWriter
     */

private:
    Service& service_;
    grpc::ServerCompletionQueue& queue_;
    Tagger& tagger_;

    std::mutex mutex_;

    UnaryRpcConnection<Request, Response> connection_;
    AsyncUnaryFunc<Service, Request, Response> register_rpc_;
    OnRequest on_request_;
};

template <typename Service, typename Request, typename Response>
auto make_unique_unary_rpc(Service& service,
                           grpc::ServerCompletionQueue& queue,
                           Tagger& tagger,
                           AsyncUnaryFunc<Service, Request, Response> register_rpc,
                           typename UnaryRpcHandler<Service, Request, Response>::OnRequest on_request)
    -> std::unique_ptr<UnaryRpcHandler<Service, Request, Response>> {

    return std::make_unique<UnaryRpcHandler<Service, Request, Response>>(service,
                                                                         queue,
                                                                         tagger,
                                                                         register_rpc,
                                                                         std::move(on_request));
}

int main(int argc, const char* argv[]) {
    std::string host_address = "0.0.0.0:50055";
    if (argc > 1) {
        host_address = argv[1];
    }
    using Service = example::protocol::Clock::AsyncService;
    using InitFunc = std::function<void(Service&,
                                        grpc::Server&,
                                        grpc::ServerCompletionQueue&,
                                        Tagger&,
                                        std::unordered_map<void*, std::unique_ptr<Rpc>>&)>;

    grpcw::util::BlockingQueue<InitFunc> initialization_queue;

    auto queue_rpc = [](std::unique_ptr<NewRpc> rpc, std::unordered_map<void*, std::unique_ptr<Rpc>>& data) -> NewRpc* {
        auto* rpc_ptr = rpc.get();
        data.emplace(rpc_ptr, std::move(rpc));
        rpc_ptr->queue_handler();
        return rpc_ptr;
    };

    std::mutex server_mutex;

    std::thread run_thread = std::thread([host_address, &initialization_queue, queue_rpc, &server_mutex] {
        Service service;

        grpc::ServerBuilder builder;
        builder.RegisterService(&service);
        builder.AddListeningPort(host_address, grpc::InsecureServerCredentials());

        std::unique_ptr<grpc::ServerCompletionQueue> server_queue;
        std::unique_ptr<grpc::Server> server;

        {
            std::lock_guard<std::mutex> lock(server_mutex);
            server_queue = builder.AddCompletionQueue();
            server = builder.BuildAndStart();
        }

        std::unordered_map<void*, std::unique_ptr<Rpc>> data;
        Tagger tagger;

        while (auto func = initialization_queue.pop_front()) {
            func(service, *server, *server_queue, tagger, data);
        }

        void* received_tag;
        bool completed_successfully;

        while (server_queue->Next(&received_tag, &completed_successfully)) {
            Tag tag = tagger.get_tag(received_tag);

            std::cout << "S " << (completed_successfully ? "Success: " : "Failure: ") << tag << std::endl;

            if (completed_successfully) {
                switch (tag.label) {

                case TagLabel::ServerNewRpc: {
                    auto rpc = static_cast<NewRpc*>(tag.data);
                    queue_rpc(rpc->clone(), data);
                    rpc->invoke_user_callback();

                } break;

                case TagLabel::ServerWriting:
                    break;

                case TagLabel::ServerDone:
                    data.erase(tag.data);
                    break;

                case TagLabel::ClientConnectionChange:
                case TagLabel::ClientFinished:
                    throw std::runtime_error("Don't use client tags in the server ya dummy");

                } // end switch

            } else {
                data.erase(tag.data);
            }
        }
    });

    auto server_start_time = std::chrono::system_clock::now();

    grpc::Server* server_ptr = nullptr;
    grpc::ServerCompletionQueue* server_queue_ptr = nullptr;

    // TODO: Add conditional_variable to make sure the pointers get set before continuing

    initialization_queue.emplace_back([server_ptr = &server_ptr,
                                       server_queue_ptr = &server_queue_ptr,
                                       &server_mutex](Service& /*service*/,
                                                      grpc::Server& server,
                                                      grpc::ServerCompletionQueue& server_queue,
                                                      Tagger& /*tagger*/,
                                                      std::unordered_map<void*, std::unique_ptr<Rpc>>& /*data*/) {
        std::lock_guard<std::mutex> lock(server_mutex);
        *server_ptr = &server;
        *server_queue_ptr = &server_queue;
    });

    initialization_queue.emplace_back(
        [queue_rpc, server_start_time](Service& service,
                                       grpc::Server& /*server*/,
                                       grpc::ServerCompletionQueue& server_queue,
                                       Tagger& tagger,
                                       std::unordered_map<void*, std::unique_ptr<Rpc>>& data) {
            auto rpc = make_unique_unary_rpc(service,
                                             server_queue,
                                             tagger,
                                             &Service::RequestGetServerTimeNow,
                                             [server_start_time](const example::protocol::FormatRequest& request,
                                                                 UnaryWriter<example::protocol::Time>* writer) {
                                                 example::protocol::Time time;
                                                 auto status = example::time_since(server_start_time, request, &time);
                                                 writer->finish(time, status);
                                             });
            queue_rpc(std::move(rpc), data);
        });

    /*
     * "Run"
     */
    initialization_queue.emplace_back(nullptr);

    std::cout << "Server running..." << std::endl;

    /*
     * Clean up after enter
     */
    std::cin.ignore();

    {
        std::lock_guard<std::mutex> lock(server_mutex);
        server_ptr->Shutdown();
        server_queue_ptr->Shutdown();
    }
    run_thread.join();

    std::cout << "Server exiting." << std::endl;

    return 0;
}
