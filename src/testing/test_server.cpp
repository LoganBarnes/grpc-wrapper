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
#include "test_server.hpp"

// grpcw
#include "testing/test_service.hpp"

namespace grpcw {
namespace testing {

TestServer::TestServer(const std::string& server_address)
    : server_(std::make_unique<server::GrpcServer>(std::make_unique<TestService>(), server_address)),
      run_thread_([&] { server_->run(); }) {}

TestServer::~TestServer() {
    server_->shutdown();
    run_thread_.join();
}

std::shared_ptr<grpc::Channel> TestServer::inprocess_channel() {
    return server_->in_process_channel();
}

} // namespace testing
} // namespace grpcw

// grpcw
#include "testing/test_proto_util.hpp"

// third-party
#include <doctest/doctest.h>
#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

using namespace grpcw;

TEST_CASE("[grpcw-test-util] run_test_server_and_check_echo_rpc_call") {
    // Create a server and run it in a separate thread
    std::string server_address = "0.0.0.0:50050";
    testing::TestServer server(server_address);

    // Create a client to connect to the server
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub = testing::protocol::Test::NewStub(channel);

    // Send a message to the server
    std::string test_msg = "a1, 23kqv9 !F(VMas3982fj!#!#+(*@)(a assdaf;le 1342 asdw32nm";

    testing::protocol::TestMessage request = {};
    request.set_msg(test_msg);

    grpc::ClientContext context;
    testing::protocol::TestMessage response;

    grpc::Status status = stub->echo(&context, request, &response);

    // Check the server received the message and responded with the same message
    CHECK(status.ok());
    CHECK(response.msg() == test_msg);
}

TEST_CASE("[grpcw-test-util] run_inprocess_test_server_and_check_echo_rpc_call") {

    // Create a server and run it in a separate thread
    testing::TestServer server;

    // Create a client using the server's in-process channel
    auto stub = testing::protocol::Test::NewStub(server.inprocess_channel());

    // Send a message to the server
    std::string test_msg = "a1, 23kqv9 !F(VMas3982fj!#!#+(*@)(a assdaf;le 1342 asdw32nm";

    testing::protocol::TestMessage request = {};
    request.set_msg(test_msg);

    grpc::ClientContext context;
    testing::protocol::TestMessage response;

    grpc::Status status = stub->echo(&context, request, &response);

    // Check the server recieved the message and responded with the same message
    CHECK(status.ok());
    CHECK(response.msg() == test_msg);
}

TEST_CASE("[grpcw-test-util] run_test_server_and_check_echo_streaming_rpc_call") {
    // Create a server and run it in a separate thread
    std::string server_address = "0.0.0.0:50050";
    testing::TestServer server(server_address);

    // Create a client to connect to the server
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub = testing::protocol::Test::NewStub(channel);

    // Send a message to the server
    std::string test_msg = "a1, 23kqv9 !F(VMas3982fj!#!#+(*@)(a assdaf;le 1342 asdw32nm";

    testing::protocol::TestMessage request = {};
    request.set_msg(test_msg);

    grpc::ClientContext context;
    testing::protocol::TestMessage response;

    auto reader = stub->endless_echo_stream(&context, request);

    reader->Read(&response);

    for (int i = 0; i < 1000000; ++i) {
        // Check the server recieved the message and responded with the same message
        CHECK(request == response);
        CHECK(response.msg() == test_msg);
        response.clear_msg();
        REQUIRE(response.msg().empty());

        if (!reader->Read(&response)) {
            break;
        }
        if (i == 500) {
            context.TryCancel();
        }
    }

    grpc::Status status = reader->Finish();

    CHECK_FALSE(status.ok());
}
