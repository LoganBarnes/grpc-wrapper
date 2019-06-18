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
#include "example_client.hpp"

// grpcw
#include "grpcw/client/grpc_client.hpp"
#include "grpcw/util/make_unique.hpp"

namespace example {
using namespace grpcw;

ExampleClient::ExampleClient(const std::string& host_address)
    : grpc_client_(util::make_unique<client::GrpcClient<protocol::Clock>>()) {

    grpc_client_->change_server(host_address,
                                [this](const client::GrpcClientState& state) { handle_state_change(state); });

    // Connect to the stream that delivers time updates
    grpc_client_->register_stream<protocol::Time>(
        [](const std::unique_ptr<protocol::Clock::Stub>& stub, grpc::ClientContext* context) {
            google::protobuf::Empty empty;
            return stub->GetServerTimeUpdates(context, empty);
        },
        [this](const protocol::Time& time) { handle_time_update(time); });
}

std::string ExampleClient::get_server_time_now(const protocol::Format& format) {
    protocol::Time time;

    if (grpc_client_->use_stub([&time, &format](const std::unique_ptr<protocol::Clock::Stub>& stub) {
            // This lambda is only used if the client is connected
            grpc::ClientContext context;
            protocol::FormatRequest format_request;
            format_request.set_format(format);
            stub->GetServerTimeNow(&context, format_request, &time);
        })) {
        return time.display_time();
    }
    return "Not connected a server";
}

void ExampleClient::handle_state_change(const client::GrpcClientState& state) {
    auto connected = (state == client::GrpcClientState::connected);

    if (connected != connected_) {
        if (connected) {
            std::cout << "Connection established" << std::endl;
        } else {
            std::cout << "Connection lost" << std::endl;
        }
    }
    connected_ = connected;
}

void ExampleClient::handle_time_update(const protocol::Time& time_update) {
    std::cout << "Server time: " << time_update.display_time() << std::endl;
}

} // namespace example

int main(int argc, const char* argv[]) {
    std::string host_address = "0.0.0.0:50055";

    if (argc > 1) {
        host_address = argv[1];
    }

    example::ExampleClient client(host_address);

    std::cout << "Type 't' to get time or 'exit' to exit" << std::endl;
    std::string input;
    do {
        std::cin >> input;

        if (input == "t") {
            std::cerr << "Time now: " << client.get_server_time_now(example::protocol::Format::COMPOUND);

        } else if (input == "h") {
            std::cerr << "Time now: " << client.get_server_time_now(example::protocol::Format::HOURS);

        } else if (input == "m") {
            std::cerr << "Time now: " << client.get_server_time_now(example::protocol::Format::MINUTES);

        } else if (input == "s") {
            std::cerr << "Time now: " << client.get_server_time_now(example::protocol::Format::SECONDS);

        } else {
            std::cerr << "Input: " << input << std::endl;
            std::cerr << "Type 't' to get time or 'exit' to exit" << std::endl;
        }

    } while (input != "exit");

    return 0;
}
