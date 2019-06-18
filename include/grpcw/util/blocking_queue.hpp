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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace grpcw {
namespace util {

///
/// @brief Thread-safe queue implementation.
///
template <typename T>
class BlockingQueue {
public:
    void push_back(T value);

    template <typename... Args>
    void emplace_back(Args&&... args);

    T pop_front();
    T pop_all_but_most_recent();

    bool empty();
    typename std::queue<T>::size_type size();

    void clear();

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<T> queue_;
};

template <typename T>
void BlockingQueue<T>::push_back(T value) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value)); // push_back
    }
    condition_.notify_one();
}

template <typename T>
template <typename... Args>
void BlockingQueue<T>::emplace_back(Args&&... args) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace(std::forward<Args>(args)...); // emplace_back
    }
    condition_.notify_one();
}

template <typename T>
T BlockingQueue<T>::pop_front() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [=] { return !queue_.empty(); });
    T rc(std::move(queue_.front()));
    queue_.pop(); // pop_front
    return rc;
}

template <typename T>
T BlockingQueue<T>::pop_all_but_most_recent() {
    std::lock_guard<std::mutex> scoped_lock(mutex_);
    while (queue_.size() > 1) {
        queue_.pop();
    }
    return queue_.front();
}

template <typename T>
bool BlockingQueue<T>::empty() {
    std::lock_guard<std::mutex> scoped_lock(mutex_);
    return queue_.empty();
}

template <typename T>
typename std::queue<T>::size_type BlockingQueue<T>::size() {
    std::lock_guard<std::mutex> scoped_lock(mutex_);
    return queue_.size();
}

template <typename T>
void BlockingQueue<T>::clear() {
    std::lock_guard<std::mutex> scoped_lock(mutex_);
    while (queue_.size() > 0) {
        queue_.pop();
    }
}

} // namespace util
} // namespace grpcw
