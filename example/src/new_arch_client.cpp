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
#include "grpcw/util/blocking_queue.hpp"
#include "tag.hpp"

//#define NEW_WAY

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

ExampleClient::~ExampleClient() {
    std::lock_guard<std::mutex> lock(stream_mutex);
    if (stream_context_) {
        stream_context_->TryCancel();
        if (stream_thread_.joinable()) {
            stream_thread_.join();
        }
    }
}

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
    {
        std::lock_guard<std::mutex> lock(stream_mutex);
        if (stream_context_) {
            stream_context_->TryCancel();
            if (stream_thread_.joinable()) {
                stream_thread_.join();
            }
            return;
        }
    }
    // Connect to the stream that delivers time updates
    {
        stream_thread_ = std::thread([this] {
            {
                std::lock_guard<std::mutex> lock(stream_mutex);
                stream_context_ = std::make_unique<grpc::ClientContext>();
                google::protobuf::Empty empty;
                stream_ = stub_->GetServerTimeUpdates(stream_context_.get(), empty);
            }

            protocol::Time time;
            while (stream_->Read(&time)) {
                std::cout << "Server time: " << time.display_time() << std::endl;
            }
        });
    }
}

} // namespace example

#ifdef NEW_WAY

namespace {

template <typename Stub, typename Request, typename Response>
using AsyncUnaryFunc = std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> (Stub::*)(grpc::ClientContext*,
                                                                                            const Request&,
                                                                                            grpc::CompletionQueue*);

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

    UnaryRpcConnection<Request, Response> connection_;
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

} // namespace

#endif

int main(int argc, const char* argv[]) {
    std::string host_address = "0.0.0.0:50055";

    if (argc > 1) {
        host_address = argv[1];
    }

#ifndef NEW_WAY
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

    using InitFunc = std::function<void(grpc::Channel&,
                                        typename Service::Stub&,
                                        grpc::CompletionQueue&,
                                        Tagger&,
                                        std::unordered_map<void*, std::unique_ptr<Rpc>>&)>;

    grpcw::util::BlockingQueue<InitFunc> initialization_queue;

    std::mutex client_mutex;

    std::thread run_thread = std::thread([host_address, &initialization_queue, &client_mutex] {
        std::unique_ptr<grpc::CompletionQueue> queue;
        std::shared_ptr<grpc::Channel> channel;
        std::unique_ptr<typename Service::Stub> stub;
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            queue = std::make_unique<grpc::CompletionQueue>();

            channel = grpc::CreateChannel(host_address, grpc::InsecureChannelCredentials());
            stub = Service::NewStub(channel);
        }

        std::unordered_map<void*, std::unique_ptr<Rpc>> data;
        Tagger tagger;

        while (auto func = initialization_queue.pop_front()) {
            func(*channel, *stub, *queue, tagger, data);
        }

        void* received_tag;
        bool completed_successfully;

        // TODO maybe have alternate queue that calls Next once inside each loop

        while (queue->Next(&received_tag, &completed_successfully)) {
            std::lock_guard<std::mutex> lock(client_mutex);

            Tag tag = tagger.get_tag(received_tag);
            std::cout << "C " << (completed_successfully ? "Success: " : "Failure: ") << tag << std::endl;

            if (completed_successfully) {
                switch (tag.label) {

                case TagLabel::ClientConnectionChange:
                    break;

                case TagLabel::ClientFinished: {
                    auto rpc = static_cast<UnaryRpc*>(tag.data);
                    rpc->invoke_user_callback();
                    data.erase(tag.data);

                } break;

                case TagLabel::ServerNewRpc:
                case TagLabel::ServerWriting:
                case TagLabel::ServerDone:
                    throw std::runtime_error("Don't use server tags in the client ya dummy");

                } // end switch
            } else {
                data.erase(tag.data);
            }
        }
    });

    grpc::Channel* channel_ptr = nullptr;
    typename Service::Stub* stub_ptr = nullptr;
    grpc::CompletionQueue* queue_ptr = nullptr;
    Tagger* tagger_ptr = nullptr;
    std::unordered_map<void*, std::unique_ptr<Rpc>>* data_ptr = nullptr;

    initialization_queue.emplace_back([channel_ptr = &channel_ptr,
                                       stub_ptr = &stub_ptr,
                                       queue_ptr = &queue_ptr,
                                       tagger_ptr = &tagger_ptr,
                                       data_ptr = &data_ptr,
                                       &client_mutex](grpc::Channel& channel,
                                                      typename Service::Stub& stub,
                                                      grpc::CompletionQueue& queue,
                                                      Tagger& tagger,
                                                      std::unordered_map<void*, std::unique_ptr<Rpc>>& data) {
        std::lock_guard<std::mutex> lock(client_mutex);
        *channel_ptr = &channel;
        *stub_ptr = &stub;
        *queue_ptr = &queue;
        *tagger_ptr = &tagger;
        *data_ptr = &data;
    });

    /*
     * "Run"
     */
    initialization_queue.emplace_back(nullptr);

    UnaryWriter<example::protocol::FormatRequest>* writer = nullptr;

    std::cin.ignore();
    {
        std::lock_guard<std::mutex> lock(client_mutex);

        auto rpc = make_unique_unary_rpc(*channel_ptr,
                                         *stub_ptr,
                                         *queue_ptr,
                                         *tagger_ptr,
                                         client_mutex,
                                         &Service::Stub::AsyncGetServerTimeNow,
                                         [](const grpc::Status& status, const example::protocol::Time& time) {
                                             if (status.ok()) {
                                                 std::cout << time.display_time();
                                             } else {
                                                 std::cerr << status.error_message();
                                             }
                                         });

        auto* key = rpc.get();
        data_ptr->emplace(key, std::move(rpc));
        writer = key;
    }

    std::cin.ignore();
    example::protocol::FormatRequest request;
    request.set_format(example::protocol::Format::SECONDS);
    writer->send(request);

    std::cin.ignore();
    {
        std::lock_guard<std::mutex> lock(client_mutex);

        auto rpc = make_unique_unary_rpc(*channel_ptr,
                                         *stub_ptr,
                                         *queue_ptr,
                                         *tagger_ptr,
                                         client_mutex,
                                         &Service::Stub::AsyncGetServerTimeNow,
                                         [](const grpc::Status& status, const example::protocol::Time& time) {
                                             if (status.ok()) {
                                                 std::cout << time.display_time();
                                             } else {
                                                 std::cerr << status.error_message();
                                             }
                                         });

        auto* key = rpc.get();
        data_ptr->emplace(key, std::move(rpc));
        writer = key;
    }

    std::cin.ignore();
    request.set_format(example::protocol::Format::COMPOUND);
    writer->send(request);

    /*
     * Clean up after enter
     */
    std::cin.ignore();

    std::cin.ignore();
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        queue_ptr->Shutdown();
    }
#endif

    return 0;
}
