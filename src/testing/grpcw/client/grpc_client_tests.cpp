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

// grpcw
#include "grpcw/client/grpc_client.hpp"
#include "grpcw/server/scoped_grpc_server.hpp"
#include "grpcw/util/blocking_queue.hpp"
#include "testing/test_service.hpp"

// generated
#include <testing.grpc.pb.h>

// third-party
#include <doctest/doctest.h>
#include <grpc++/create_channel.h>

// standard
#include <sstream>

namespace {
using namespace grpcw;

TEST_CASE("[grpcw] testing the grpc_connectivity_state string functions") {
    std::stringstream ss;

    SUBCASE("GRPC_CHANNEL_CONNECTING_string") {
        CHECK(client::to_cnc_client_state(grpc_connectivity_state::GRPC_CHANNEL_CONNECTING)
              == client::GrpcClientState::attempting_to_connect);
    }

    SUBCASE("GRPC_CHANNEL_IDLE_string") {
        CHECK(client::to_cnc_client_state(grpc_connectivity_state::GRPC_CHANNEL_IDLE)
              == client::GrpcClientState::not_connected);
    }

    SUBCASE("GRPC_CHANNEL_READY_string") {
        CHECK(client::to_cnc_client_state(grpc_connectivity_state::GRPC_CHANNEL_READY)
              == client::GrpcClientState::connected);
    }

    SUBCASE("GRPC_CHANNEL_SHUTDOWN_string") {
        CHECK(client::to_cnc_client_state(grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN)
              == client::GrpcClientState::not_connected);
    }

    SUBCASE("GRPC_CHANNEL_TRANSIENT_FAILURE_string") {
        CHECK(client::to_cnc_client_state(grpc_connectivity_state::GRPC_CHANNEL_TRANSIENT_FAILURE)
              == client::GrpcClientState::attempting_to_connect);
    }

    SUBCASE("invalid_grpc_connectivity_state_string") {
        // Really have to do some shadily incorrect coding to cause this
        CHECK_THROWS_AS(client::to_cnc_client_state(static_cast<grpc_connectivity_state>(0xffff)),
                        std::invalid_argument);
    }
}

void check_connects(util::BlockingQueue<client::GrpcClientState>& queue) {
    client::GrpcClientState state = queue.pop_front();

    // The client may enter an 'attempting_to_connect' state before connecting but
    // many times it will connect immediately since the server is hosted locally.
    if (state == client::GrpcClientState::attempting_to_connect) {
        REQUIRE(queue.pop_front() == client::GrpcClientState::connected);

    } else {
        REQUIRE(state == client::GrpcClientState::connected);
    }
}

class StateUpdater {
public:
    void handle_state_change(client::GrpcClientState state) {
        INFO(state);
        state_queue.push_back(state);
    }

    util::BlockingQueue<client::GrpcClientState> state_queue;
};

TEST_CASE("[grpcw-client] no_server") {
    StateUpdater updater;
    std::string address = "0.0.0.0:50050";

    {
        client::GrpcClient<testing::protocol::Test> client;
        REQUIRE(client.get_state() == client::GrpcClientState::not_connected);

        client.change_server(address, std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::attempting_to_connect);
    }

    REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
    REQUIRE(updater.state_queue.empty());
}

TEST_CASE("[grpcw-client] no_server_stop_connection_attempts") {
    StateUpdater updater;
    std::string address = "0.0.0.0:50051";

    {
        client::GrpcClient<testing::protocol::Test> client;
        REQUIRE(client.get_state() == client::GrpcClientState::not_connected);

        client.change_server(address, std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::attempting_to_connect);

        client.kill_streams_and_channel();

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
    }
    // Client is deleted but the state was already not connected so nothing is sent on exit.
    REQUIRE(updater.state_queue.empty());
}

TEST_CASE("[grpcw-client] delayed_server") {
    StateUpdater updater;
    std::string server_address = "0.0.0.0:50052";

    {
        client::GrpcClient<testing::protocol::Test> client;
        REQUIRE(client.get_state() == client::GrpcClientState::not_connected);

        client.change_server(server_address,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::attempting_to_connect);

        {
            server::ScopedGrpcServer server(std::make_unique<testing::TestService>(), server_address);
            REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::connected);
        }
        // Server is deleted
        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::attempting_to_connect);
    }
    // Client is deleted
    REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
    REQUIRE(updater.state_queue.empty());
}

TEST_CASE("[grpcw-client] already_running_server") {
    StateUpdater updater;
    std::string server_address = "0.0.0.0:50053";
    server::ScopedGrpcServer server(std::make_unique<testing::TestService>(), server_address);

    {
        client::GrpcClient<testing::protocol::Test> client;
        REQUIRE(client.get_state() == client::GrpcClientState::not_connected);

        client.change_server(server_address,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        check_connects(updater.state_queue);
    }

    REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
    REQUIRE(updater.state_queue.empty());
}

// Switch between external servers
TEST_CASE("[grpcw-client] multiple_addresses") {
    StateUpdater updater;
    std::string server_address1 = "0.0.0.0:50054";
    std::string server_address2 = "0.0.0.0:50055";

    server::ScopedGrpcServer server1(std::make_unique<testing::TestService>(), server_address1);

    {
        client::GrpcClient<testing::protocol::Test> client;
        REQUIRE(client.get_state() == client::GrpcClientState::not_connected);

        client.change_server(server_address1,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        check_connects(updater.state_queue);

        server::ScopedGrpcServer server2(std::make_unique<testing::TestService>(), server_address2);

        // Connect to server 2
        client.change_server(server_address2,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
        check_connects(updater.state_queue);

        // Reconnect to server 1
        client.change_server(server_address1,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
        check_connects(updater.state_queue);
    }
    // Client is deleted.
    REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
    REQUIRE(updater.state_queue.empty());
}

// Switch between external servers
TEST_CASE("[grpcw-client] external_to_inprocess_and_back") {
    StateUpdater updater;
    std::string server_address1 = "0.0.0.0:50056";
    std::string server_address2 = "0.0.0.0:50057";

    server::ScopedGrpcServer server1(std::make_unique<testing::TestService>(), server_address1);

    {
        client::GrpcClient<testing::protocol::Test> client;
        REQUIRE(client.get_state() == client::GrpcClientState::not_connected);

        client.change_server(server_address1,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        check_connects(updater.state_queue);

        server::ScopedGrpcServer ip_server(std::make_unique<testing::TestService>());

        // Connect to server in-process server
        client.change_server(ip_server.in_process_channel(client::default_channel_arguments()));

        // One last state update callback before we switch to the in-process server
        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
        REQUIRE(updater.state_queue.empty());

        // The state should always be connected now
        REQUIRE(client.get_state() == client::GrpcClientState::connected);

        // Reconnect to server 1
        client.change_server(server_address1,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
        check_connects(updater.state_queue);
    }
    // Client is deleted.
    REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
    REQUIRE(updater.state_queue.empty());
}

// Switch between external servers
TEST_CASE("[grpcw-client] external_to_inprocess_and_back") {
    StateUpdater updater;
    std::string server_address1 = "0.0.0.0:50056";
    std::string server_address2 = "0.0.0.0:50057";

    server::ScopedGrpcServer server1(std::make_unique<testing::TestService>(), server_address1);

    {
        client::GrpcClient<testing::protocol::Test> client;
        REQUIRE(client.get_state() == client::GrpcClientState::not_connected);

        client.change_server(server_address1,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        check_connects(updater.state_queue);

        server::ScopedGrpcServer ip_server(std::make_unique<testing::TestService>());

        // Connect to server in-process server
        client.change_server(ip_server.in_process_channel());

        // One last state update callback before we switch to the in-process server
        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
        REQUIRE(updater.state_queue.empty());

        // The state should always be connected now
        REQUIRE(client.get_state() == client::GrpcClientState::connected);

        // Reconnect to server 1
        client.change_server(server_address1,
                             std::bind(&StateUpdater::handle_state_change, &updater, std::placeholders::_1));

        REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
        check_connects(updater.state_queue);
    }
    // Client is deleted.
    REQUIRE(updater.state_queue.pop_front() == client::GrpcClientState::not_connected);
    REQUIRE(updater.state_queue.empty());
}

} // namespace
