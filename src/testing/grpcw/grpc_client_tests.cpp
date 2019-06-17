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
#include "grpcw/grpc_client.hpp"

// third-party
#include <doctest/doctest.h>

// standard
#include <sstream>

using namespace grpcw;

TEST_CASE("[grpcw] testing the grpc_connectivity_state string functions") {
    std::stringstream ss;

    SUBCASE("GRPC_CHANNEL_CONNECTING_string") {
        ss << grpc_connectivity_state::GRPC_CHANNEL_CONNECTING;
        CHECK(ss.str() == "GRPC_CHANNEL_CONNECTING");
    }

    SUBCASE("GRPC_CHANNEL_IDLE_string") {
        ss << grpc_connectivity_state::GRPC_CHANNEL_IDLE;
        CHECK(ss.str() == "GRPC_CHANNEL_IDLE");
    }

    SUBCASE("GRPC_CHANNEL_READY_string") {
        ss << grpc_connectivity_state::GRPC_CHANNEL_READY;
        CHECK(ss.str() == "GRPC_CHANNEL_READY");
    }

    SUBCASE("GRPC_CHANNEL_SHUTDOWN_string") {
        ss << grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN;
        CHECK(ss.str() == "GRPC_CHANNEL_SHUTDOWN");
    }

    SUBCASE("GRPC_CHANNEL_TRANSIENT_FAILURE_string") {
        ss << grpc_connectivity_state::GRPC_CHANNEL_TRANSIENT_FAILURE;
        CHECK(ss.str() == "GRPC_CHANNEL_TRANSIENT_FAILURE");
    }

    SUBCASE("invalid_grpc_connectivity_state_string") {
        // Really have to do some shadily incorrect coding to cause this
        ss << static_cast<grpc_connectivity_state>(-1);
        CHECK(ss.str() == "Invalid enum value");
    }
}

TEST_CASE("[grpcw] testing the grpc_connectivity_state string functions") {
    std::stringstream ss;

    SUBCASE("GRPC_CHANNEL_CONNECTING_to_typed_state") {
        CHECK(to_typed_state(grpc_connectivity_state::GRPC_CHANNEL_CONNECTING)
              == GrpcClientState::attempting_to_connect);
    }

    SUBCASE("GRPC_CHANNEL_IDLE_to_typed_state") {
        CHECK(to_typed_state(grpc_connectivity_state::GRPC_CHANNEL_IDLE) == GrpcClientState::not_connected);
    }

    SUBCASE("GRPC_CHANNEL_READY_to_typed_state") {
        CHECK(to_typed_state(grpc_connectivity_state::GRPC_CHANNEL_READY) == GrpcClientState::connected);
    }

    SUBCASE("GRPC_CHANNEL_SHUTDOWN_to_typed_state") {
        CHECK(to_typed_state(grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN) == GrpcClientState::not_connected);
    }

    SUBCASE("GRPC_CHANNEL_TRANSIENT_FAILURE_to_typed_state") {
        CHECK(to_typed_state(grpc_connectivity_state::GRPC_CHANNEL_TRANSIENT_FAILURE)
              == GrpcClientState::attempting_to_connect);
    }

    SUBCASE("invalid_grpc_connectivity_state_throws") {
        // Really have to do some shadily incorrect coding to cause this
        CHECK_THROWS_AS(to_typed_state(static_cast<grpc_connectivity_state>(-1)), std::invalid_argument);
    }
}
