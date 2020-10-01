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
#include "../shared/tagger.hpp"
#include "grpcw/util/blocking_queue.hpp"
#include "unary_rpc_handler.hpp"

// generated
#include <example.grpc.pb.h>

// external
#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

// standard
#include <thread>

namespace grpcw {
namespace client {

constexpr auto poison_pill = nullptr;

template <typename Service>
class Client {
public:
    explicit Client(const std::string& host_address) {
        // Start the main task thread
        task_thread_ = std::thread([this] {
            while (auto task = task_queue_.pop_front()) {
                task();
            }
        });

        // Initialize objects
        task_queue_.emplace_back([this, host_address] {
            completion_queue_ = std::make_unique<grpc::CompletionQueue>();
            channel_ = grpc::CreateChannel(host_address, grpc::InsecureChannelCredentials());
            stub_ = Service::NewStub(channel_);
        });
    }

    ~Client() {
        // Destroy objects
        task_queue_.emplace_back([this] {
            stub_ = nullptr;
            channel_ = nullptr;
            completion_queue_->Shutdown();

            process_completion_queue();
            assert(!completion_queue_valid_);
        });

        task_queue_.emplace_back(poison_pill);
        task_thread_.join();
    }

private:
    std::unique_ptr<grpc::CompletionQueue> completion_queue_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<typename Service::Stub> stub_;

    bool completion_queue_valid_ = false;

    std::unordered_map<void*, std::unique_ptr<Rpc>> rpcs_;
    Tagger tagger_;

    grpcw::util::BlockingQueue<std::function<void()>> task_queue_; ///< EVERYTHING in this class happens via this queue
    std::thread task_thread_; ///< EVERYTHING in this class happens on this thread

    auto process_completion_queue() -> void {
        if (!completion_queue_valid_) {
            return;
        }

        void* received_tag;
        bool completed_successfully;

        if (!completion_queue_->Next(&received_tag, &completed_successfully)) {
            completion_queue_valid_ = false;
            return;
        }

        Tag tag = tagger_.get_tag(received_tag);
        std::cout << "C " << (completed_successfully ? "Success: " : "Failure: ") << tag << std::endl;

        if (completed_successfully) {
            switch (tag.label) {

            case TagLabel::ClientConnectionChange:
                break;

            case TagLabel::ClientFinished: {
                auto rpc = static_cast<UnaryRpc*>(tag.data);
                rpc->invoke_user_callback();
                rpcs_.erase(tag.data);

            } break;

            case TagLabel::ServerNewRpc:
            case TagLabel::ServerWriting:
            case TagLabel::ServerDone:
                throw std::runtime_error("Don't use server tags in the client ya dummy");

            } // end switch
        } else {
            rpcs_.erase(tag.data);
        }
    }
};

} // namespace client
} // namespace grpcw
