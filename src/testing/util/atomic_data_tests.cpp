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

TEST_CASE("[grpcw-util] atomic_data_notify_all") {

    struct SharedData {
        bool write_away = false;
        int num_threads = 0;
    };
    util::AtomicData<SharedData> shared_data;

    std::array<std::thread, 500> threads;

    // Wait for 'notify_all' from the main thread then safely increment 'num_threads' once per thread
    for (auto& thread : threads) {
        thread = std::thread([&] {
            shared_data.wait_to_use_safely([](const SharedData& data) { return data.write_away; },
                                           [](SharedData& data) { ++data.num_threads; });
        });
    }

    // notify all the threads at once
    shared_data.use_safely([](SharedData& data) { data.write_away = true; });
    shared_data.notify_all();

    for (auto& thread : threads) {
        thread.join();
    }

    // 'num_threads' should exactly equal the amount of threads we created
    shared_data.use_safely([&](const SharedData& data) { CHECK(data.num_threads == threads.size()); });
}

TEST_CASE_TEMPLATE("[grpcw-util] interleaved_atomic_data", T, short, int, unsigned, float, double) {
    struct SharedData {
        T current_number = 0;
        std::vector<T> all_data = {};
        std::vector<T> odds = {};
        std::vector<T> even = {};
        bool writing_odds = false;
    };

    constexpr short max_number = 9;

    // Use AtomicData structure to update data from two threads
    util::AtomicData<SharedData> shared_data;

    // Write odd numbers
    std::thread thread([&] {
        bool stop_loop;
        do {
            shared_data.wait_to_use_safely([](const SharedData& data) { return data.writing_odds; },
                                           [&](SharedData& data) {
                                               data.all_data.emplace_back(data.current_number++);
                                               data.odds.emplace_back(data.all_data.back());
                                               stop_loop = data.current_number >= max_number;
                                               data.writing_odds = false;
                                           });
            shared_data.notify_one();
        } while (not stop_loop);
    });

    // Write even numbers
    bool stop_loop;
    do {
        shared_data.wait_to_use_safely([](const SharedData& data) { return not data.writing_odds; },
                                       [&](SharedData& data) {
                                           data.all_data.emplace_back(data.current_number++);
                                           data.even.emplace_back(data.all_data.back());
                                           stop_loop = data.current_number >= max_number;
                                           data.writing_odds = true;
                                       });
        shared_data.notify_one();
    } while (not stop_loop);

    thread.join();

    // shared_data should be correctly ordered despite being modified by separate threads
    shared_data.use_safely([&](const SharedData& data) {
        CHECK(data.current_number == max_number + 1);
        CHECK(data.even == std::vector<T>{0, 2, 4, 6, 8});
        CHECK(data.odds == std::vector<T>{1, 3, 5, 7, 9});
        CHECK(data.all_data == std::vector<T>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
        CHECK_FALSE(data.writing_odds);
    });
}

} // namespace
