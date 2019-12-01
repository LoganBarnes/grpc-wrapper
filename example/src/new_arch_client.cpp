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
#include "new_arch_client.hpp"

// grpcw
#include "grpcw/client/grpc_client.hpp"
#include "tag.hpp"

inline std::string to_string(const grpc_connectivity_state& state) {
    switch (state) {
    case GRPC_CHANNEL_SHUTDOWN:
        return "GRPC_CHANNEL_SHUTDOWN";
    case GRPC_CHANNEL_IDLE:
        return "GRPC_CHANNEL_IDLE";
    case GRPC_CHANNEL_CONNECTING:
        return "GRPC_CHANNEL_CONNECTING";
    case GRPC_CHANNEL_TRANSIENT_FAILURE:
        return "GRPC_CHANNEL_TRANSIENT_FAILURE";
    case GRPC_CHANNEL_READY:
        return "GRPC_CHANNEL_READY";
    }
    throw std::invalid_argument("Invalid GrpcClientState");
}

inline ::std::ostream& operator<<(::std::ostream& os, const grpc_connectivity_state& state) {
    return os << to_string(state);
}

namespace example {
using namespace grpcw;

ExampleClient::ExampleClient(const std::string& host_address)
    : channel_(grpc::CreateChannel(host_address, grpc::InsecureChannelCredentials())),
      stub_(Service::NewStub(channel_)) {}

std::string ExampleClient::get_server_time_now(const protocol::Format& format) {
    protocol::Time time;
    grpc::Status status;

    grpc::ClientContext context;
    protocol::FormatRequest format_request;
    format_request.set_format(format);
    status = stub_->GetServerTimeNow(&context, format_request, &time);

    if (status.ok()) {
        return time.display_time();
    }

    return status.error_message();
}

void ExampleClient::toggle_streaming() {
    if (stream_context_) {
        std::lock_guard<std::mutex> lock(stream_mutex);
        stream_context_->TryCancel();
        if (stream_thread_.joinable()) {
            stream_thread_.join();
        }
    } else {
        // Connect to the stream that delivers time updates
        {
            std::lock_guard<std::mutex> lock(stream_mutex);
            stream_context_ = std::make_unique<grpc::ClientContext>();
            google::protobuf::Empty empty;
            stream_ = stub_->GetServerTimeUpdates(stream_context_.get(), empty);

            stream_thread_ = std::thread([this] {
                protocol::Time time;
                while (stream_->Read(&time)) {
                    std::cout << "Server time: " << time.display_time() << std::endl;
                }
            });
        }
        //        grpc_client_
        //            ->register_stream<protocol::Time>([this](typename Service::Stub& stub, grpc::ClientContext* context) {
        //                stream_context_ = context;
        //                google::protobuf::Empty empty;
        //                return stub.GetServerTimeUpdates(context, empty);
        //            })
        //            .on_update(
        //                [](const protocol::Time& time) { std::cout << "Server time: " << time.display_time() << std::endl; })
        //            .on_finish([this](const grpc::Status& status) {
        //                stream_context_ = nullptr;
        //                std::cout << "Stream finished: " << status.error_message() << std::endl;
        //            });
    }
}
auto ExampleClient::use_stub(const ExampleClient::UsageFunc& usage_func) -> bool {
    //    auto channel_state = channel_->GetState(true);
    //    if (channel_state == GRPC_CHANNEL_READY) {
    usage_func(*stub_);
    return true;
    //    }
    //    return false;
}

} // namespace example

namespace {

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

} // namespace

int main(int argc, const char* argv[]) {
    std::string host_address = "0.0.0.0:50055";

    if (argc > 1) {
        host_address = argv[1];
    }

#if 1
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
            std::cout << "Server time now: " << time_string << std::endl;
        }

        std::cin >> input;
    }

    std::cout << "\nExiting" << std::endl;
#else

    using Service = example::protocol::Clock;

    std::mutex queue_mutex;

    std::thread run_thread = std::thread([host_address, &queue_mutex] {
        std::unique_ptr<grpc::CompletionQueue> queue;
        std::shared_ptr<grpc::Channel> channel;
        std::unique_ptr<typename Service::Stub> stub;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            queue = std::make_unique<grpc::CompletionQueue>();

            channel = grpc::CreateChannel(host_address, grpc::InsecureChannelCredentials());
            stub = Service::NewStub(channel);
        }

        Tagger tagger;

        void* received_tag;
        bool completed_successfully;

        while (queue->Next(&received_tag, &completed_successfully)) {
            Tag tag = tagger.get_tag(received_tag);

            std::cout << "C " << (completed_successfully ? "Success: " : "Failure: ") << tag << std::endl;

            if (completed_successfully) {
                switch (tag.label) {

                case TagLabel::ClientConnectionChange:
                    break;

                case TagLabel::ServerNewRpc:
                case TagLabel::ServerWriting:
                case TagLabel::ServerDone:
                    throw std::runtime_error("Don't use server tags in the client ya dummy");

                } // end switch
            } else {
                // Remove data
            }
        }
    });

    grpc::CompletionQueue* queue_ptr = nullptr;

    /*
     * Clean up after enter
     */
    std::cin.ignore();

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        queue_ptr->Shutdown();
    }
#endif

    return 0;
}
