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

namespace example {
using namespace grpcw;

ExampleClient::ExampleClient(const std::string& host_address)
    : grpc_client_(std::make_unique<client::GrpcClient<Service>>()) {

    grpc_client_->change_server(host_address, [](const client::GrpcClientState& state) {
        std::cout << "Connection state: " << state << std::endl;
    });
}

std::string ExampleClient::get_server_time_now(const protocol::Format& format) {
    protocol::Time time;

    if (grpc_client_->use_stub([&time, &format](typename Service::Stub& stub) {
            // This lambda is only used if the client is connected
            grpc::ClientContext context;
            protocol::FormatRequest format_request;
            format_request.set_format(format);
            stub.GetServerTimeNow(&context, format_request, &time);
        })) {
        return time.display_time();
    }
    return "Not connected a server";
}

void ExampleClient::toggle_streaming() {
    if (stream_context_) {
        stream_context_->TryCancel();
    } else {
        // Connect to the stream that delivers time updates
        grpc_client_
            ->register_stream<protocol::Time>([this](typename Service::Stub& stub, grpc::ClientContext* context) {
                stream_context_ = context;
                google::protobuf::Empty empty;
                return stub.GetServerTimeUpdates(context, empty);
            })
            .on_update(
                [](const protocol::Time& time) { std::cout << "Server time: " << time.display_time() << std::endl; })
            .on_finish([this](const grpc::Status& status) {
                stream_context_ = nullptr;
                std::cout << "Stream finished: " << status.error_message() << std::endl;
            });
    }
}

} // namespace example

int main(int argc, const char* argv[]) {
    std::string host_address = "0.0.0.0:50055";

    if (argc > 1) {
        host_address = argv[1];
    }

    example::ExampleClient client(host_address);

    auto display_usage = [] {
        std::cout << "Type characters to display the server time in different formats:\n"
                  << "\t'f' - Display the full time\n"
                  << "\t'h' - Display hours only\n"
                  << "\t'm' - Display minutes only\n"
                  << "\t's' - Display seconds only\n"
                  << "\t't' - Toggle a stream of time updates\n"
                  << "\t'u' - Display usage message\n"
                  << "\t'x' - Exit the program\n"
                  << std::endl;
    };

    display_usage();

    std::string input;
    std::cin >> input;

    while (std::cin.good() && input != "x") {
        std::string time_string;

        if (input == "f") {
            time_string = client.get_server_time_now(example::protocol::Format::COMPOUND);

        } else if (input == "h") {
            time_string = client.get_server_time_now(example::protocol::Format::HOURS);

        } else if (input == "m") {
            time_string = client.get_server_time_now(example::protocol::Format::MINUTES);

        } else if (input == "s") {
            time_string = client.get_server_time_now(example::protocol::Format::SECONDS);

        } else if (input == "t") {
            client.toggle_streaming();

        } else if (input == "u") {
            display_usage();

        } else {
            std::cout << "Unrecognized input: '" << input << "'" << std::endl;
            display_usage();
        }

        if (!time_string.empty()) {
            std::cout << "Time now: " << time_string << std::endl;
        }

        std::cin >> input;
    }

    std::cout << "\nExiting" << std::endl;

    return 0;
}
