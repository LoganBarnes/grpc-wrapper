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

// standard
#include <mutex>

namespace grpcw {
namespace detail {

/**
 * @brief Owns complex data that can be accessed in a thread safe way.
 *
 * Example:
 *
 *     struct MyComplexData {
 *         int thing1;
 *         bool thing2;
 *         std::vector<double> more_things;
 *         OtherStruct complex_thing;
 *     };
 *
 *     AtomicData<MyComplexData> shared_data;
 *
 *     ... Later, in different threads
 *
 *     shared_data.use_safely([] (MyComplexData& data) {
 *         // Do things with 'data' here:
 *         ...
 *     });
 *
 *     ...
 */
template <typename T>
class AtomicData {
public:
    explicit AtomicData(T data = {});

    /**
     * @brief Use the data in a thread safe manner.
     */
    template <typename Func>
    void use_safely(const Func& func);

    template <typename Func>
    void use_safely(const Func& func) const;

private:
    mutable std::mutex lock_; // mutable so it can be used with const functions
    T data_;
};

template <typename T>
AtomicData<T>::AtomicData(T data) : data_(std::move(data)) {}

template <typename T>
template <typename Func>
void AtomicData<T>::use_safely(const Func& func) {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    func(data_);
}

template <typename T>
template <typename Func>
void AtomicData<T>::use_safely(const Func& func) const {
    std::lock_guard<std::mutex> scoped_lock(lock_);
    func(data_);
}

} // namespace detail
} // namespace grpcw
