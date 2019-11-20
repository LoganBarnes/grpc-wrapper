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
#include "example_server.hpp"

// grpcw
#include "grpcw/server/grpc_async_server.hpp"

// third-party
#include <grpc++/security/server_credentials.h>
#include <grpc++/server_builder.h>

// standard
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
    server_->register_async(&Service::RequestGetServerTimeNow,
                            [this](const protocol::FormatRequest& request, protocol::Time* time) {
                                return this->get_time(request, time);
                            });

    keep_ticking_.store(true);

    ticker_ = std::thread([this] {
        protocol::FormatRequest request;
        protocol::Time time;

        while (keep_ticking_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            get_time(request, &time);
            time_stream_->write(time);
        }
    });
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

grpc::Status ExampleServer::get_time(const protocol::FormatRequest& request, protocol::Time* time) {
    std::stringstream ss;

    auto time_since_server_start = std::chrono::system_clock::now() - server_start_time_;

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

} // namespace example

int main(int argc, const char* argv[]) {
    std::string host_address = "0.0.0.0:50055";

    if (argc > 1) {
        host_address = argv[1];
    }

    example::ExampleServer server(host_address);

    std::cout << "Server running on '" << host_address << "'\n";
    std::cout << "Press enter to quit..." << std::flush;
    std::cin.ignore();
    std::cout << "Exiting." << std::endl;

    return 0;
}
