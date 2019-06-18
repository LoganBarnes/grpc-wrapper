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
#include "test_proto_util.hpp"

// generated
#include <testing.grpc.pb.h>

// third-party
#include <doctest/doctest.h>

// standard
#include <sstream>

using namespace grpcw;

TEST_CASE("[grpcw-test-util] operator<< prints string") {
    testing::protocol::TestMessage test_msg;
    test_msg.set_msg("Blah blah");

    std::stringstream ss;
    ss << test_msg;
    CHECK(ss.str() == "msg: \"Blah blah\"\n");
}

TEST_CASE("[grpcw-test-util] operator== compares messages") {
    testing::protocol::TestMessage test_msg1;
    test_msg1.set_msg("Blah blah");

    testing::protocol::TestMessage test_msg2;

    CHECK_FALSE(test_msg1 == test_msg2);

    test_msg2.set_msg("Blah blah");
    CHECK(test_msg1 == test_msg2);
}
