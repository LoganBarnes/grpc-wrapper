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
#include "grpcw/util/atomic_data.hpp"

// third-party
#include <doctest/doctest.h>

// standard
#include <condition_variable>
#include <numeric>
#include <thread>
#include <vector>

namespace {
using namespace grpcw;

TEST_CASE_TEMPLATE("[grpcw-util] interleaved_atomic_data", T, short, int, unsigned, float, double) {
    struct SharedData {
        T current_number = 0;
        std::vector<T> all_numbers = {};
        std::vector<T> odds = {};
        std::vector<T> evens = {};
        bool is_odd = false;
    };

    constexpr auto num_threads = 15u;
    constexpr auto num_loops_per_thread = 100u;
    constexpr auto max_number = num_threads * num_loops_per_thread;

    REQUIRE(max_number < std::numeric_limits<T>::max());
    REQUIRE(max_number % 2 == 0); // this assumption is used to check the answers

    // Use AtomicData structure to update data from two threads
    util::AtomicData<SharedData> shared_data;
    std::mutex lock;
    std::condition_variable blocker;

    auto parallel_compute_function = [&shared_data, &lock, &blocker] {
        {
            // wait for all threads to be initialized before starting
            std::unique_lock<std::mutex> tmp_lock(lock);
            blocker.wait(tmp_lock);
        }
        // The lock has been destroyed here so all thread safety is handled by AtomicData

        for (auto i = 0u; i < num_loops_per_thread; ++i) {

            // manipulate the shared data
            shared_data.use_safely([](SharedData& data) {
                data.all_numbers.emplace_back(data.current_number);
                if (data.is_odd) {
                    data.odds.emplace_back(data.current_number);
                } else {
                    data.evens.emplace_back(data.current_number);
                }
                ++data.current_number;
                data.is_odd = !data.is_odd;
            });
        }
    };

    {
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        for (auto i = 0u; i < num_threads; ++i) {
            threads.emplace_back(parallel_compute_function);
        }

        // let all the threads spin up then start them all.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        blocker.notify_all();

        // wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();
        }
    }

    std::vector<T> all_numbers_expected(max_number);
    std::vector<T> odds_expected(max_number / 2);
    std::vector<T> evens_expected(max_number / 2);

    std::iota(all_numbers_expected.begin(), all_numbers_expected.end(), T(0));
    for (auto i = 0u; i < max_number / 2u; ++i) {
        evens_expected[i] = all_numbers_expected[i * 2 + 0];
        odds_expected[i] = all_numbers_expected[i * 2 + 1];
    }

    // test the const functionality too.
    const auto& const_data = shared_data;

    // shared_data should be correctly ordered despite being modified by separate threads
    const_data.use_safely([&](const SharedData& data) {
        CHECK(data.current_number == max_number);
        CHECK(data.all_numbers == all_numbers_expected);
        CHECK(data.odds == odds_expected);
        CHECK(data.evens == evens_expected);
    });
}

} // namespace
