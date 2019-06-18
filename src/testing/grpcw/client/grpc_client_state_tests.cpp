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
#include "grpcw/client/grpc_client_state.hpp"

// third-party
#include <doctest/doctest.h>

// standard
#include <sstream>

using namespace grpcw;

TEST_CASE("[grpcw] testing the GrpcClientState string functions") {
    std::stringstream ss;

    SUBCASE("not_connected_string") {
        ss << client::GrpcClientState::not_connected;
        CHECK(ss.str() == "not_connected");
    }

    SUBCASE("attempting_to_connect_string") {
        ss << client::GrpcClientState::attempting_to_connect;
        CHECK(ss.str() == "attempting_to_connect");
    }

    SUBCASE("connected_string") {
        ss << client::GrpcClientState::connected;
        CHECK(ss.str() == "connected");
    }

    SUBCASE("invalid_GrpcClientState_string") {
        // Really have to do some shadily incorrect coding to cause this
        CHECK_THROWS_AS(ss << static_cast<client::GrpcClientState>(-1), std::invalid_argument);
    }
}
