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
#include "stream_rpc_handler.hpp"

namespace grpcw {
namespace server {
namespace detail {

/// \brief Allows callbacks to be set for `GrpcClientStream`s
/// \tparam Result is the stream's result type
template <typename BaseService, typename Request, typename Response>
class StreamRpcHandlerCallbackSetter {

    using StreamConnectionCallback =
        typename detail::StreamRpcHandler<BaseService, Request, Response>::ConnectionCallback;
    using StreamDeletionCallback = typename detail::StreamRpcHandler<BaseService, Request, Response>::DeletionCallback;

public:
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    StreamRpcHandlerCallbackSetter(detail::StreamRpcHandler<BaseService, Request, Response>* stream);

    /// \brief Set the ConnectionCallback function for this stream
    StreamRpcHandlerCallbackSetter<BaseService, Request, Response>&
    on_connect(StreamConnectionCallback connection_callback);

    /// \brief Set the DeletionCallback function for this stream
    StreamRpcHandlerCallbackSetter<BaseService, Request, Response>& on_delete(StreamDeletionCallback deletion_callback);

    /// \brief Return the stream used to send updates to the clients
    StreamInterface<Response>* stream();

private:
    StreamRpcHandler<BaseService, Request, Response>* stream_;
};

template <typename BaseService, typename Request, typename Response>
StreamRpcHandlerCallbackSetter<BaseService, Request, Response>::StreamRpcHandlerCallbackSetter(
    detail::StreamRpcHandler<BaseService, Request, Response>* stream)
    : stream_(stream) {}

template <typename BaseService, typename Request, typename Response>
StreamRpcHandlerCallbackSetter<BaseService, Request, Response>&
StreamRpcHandlerCallbackSetter<BaseService, Request, Response>::on_connect(
    StreamConnectionCallback connection_callback) {
    stream_->on_connect(stream_, std::move(connection_callback));
    return *this;
}

template <typename BaseService, typename Request, typename Response>
StreamRpcHandlerCallbackSetter<BaseService, Request, Response>&
StreamRpcHandlerCallbackSetter<BaseService, Request, Response>::on_delete(StreamDeletionCallback deletion_callback) {
    stream_->on_delete(stream_, std::move(deletion_callback));
    return *this;
}

template <typename BaseService, typename Request, typename Response>
StreamInterface<Response>* StreamRpcHandlerCallbackSetter<BaseService, Request, Response>::stream() {
    return stream_;
}

} // namespace detail
} // namespace server
} // namespace grpcw
