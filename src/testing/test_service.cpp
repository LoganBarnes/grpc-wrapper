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
#include "test_service.hpp"

// grpcw
#include "testing/test_proto_util.hpp"

namespace grpcw {
namespace testing {

TestService::TestService() : protocol::Test::Service() {}
TestService::~TestService() = default;

grpc::Status TestService::echo(grpc::ServerContext* /*context*/,
                               const protocol::TestMessage* request,
                               protocol::TestMessage* response) {
    response->CopyFrom(*request);
    return grpc::Status::OK;
}

grpc::Status TestService::endless_echo_stream(grpc::ServerContext* context,
                                              const protocol::TestMessage* request,
                                              grpc::ServerWriter<protocol::TestMessage>* writer) {
    while (writer->Write(*request)) {
        if (context->IsCancelled()) {
            return grpc::Status::CANCELLED;
        }
    }
    return grpc::Status::OK;
}

} // namespace testing
} // namespace grpcw

// third-party
#include <doctest/doctest.h>

using namespace grpcw;

TEST_CASE("[grpcw-test-util] echo_service_echos") {
    const std::string message = "01234aBcDeFgHiJkLmNoPqRsTuVwXyZ56789";

    testing::TestService service;

    testing::protocol::TestMessage request, response;
    request.set_msg(message);

    grpc::ServerContext context;
    service.echo(&context, &request, &response);

    CHECK(request.msg() == message); // request is unchanged
    CHECK(response == request); // response is an exact copy of request
}
