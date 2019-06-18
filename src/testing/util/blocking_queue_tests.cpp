// ///////////////////////////////////////////////////////////////////////////////////////
//                                                                           |________|
//  Copyright (c) 2018 CloudNC Ltd - All Rights Reserved                        |  |
//                                                                              |__|
//        ____                                                                .  ||
//       / __ \                                                               .`~||$$$$
//      | /  \ \         /$$$$$$  /$$                           /$$ /$$   /$$  /$$$$$$$
//      \ \ \ \ \       /$$__  $$| $$                          | $$| $$$ | $$ /$$__  $$
//    / / /  \ \ \     | $$  \__/| $$  /$$$$$$  /$$   /$$  /$$$$$$$| $$$$| $$| $$  \__/
//   / / /    \ \__    | $$      | $$ /$$__  $$| $$  | $$ /$$__  $$| $$ $$ $$| $$
//  / / /      \__ \   | $$      | $$| $$  \ $$| $$  | $$| $$  | $$| $$  $$$$| $$
// | | / ________ \ \  | $$    $$| $$| $$  | $$| $$  | $$| $$  | $$| $$\  $$$| $$    $$
//  \ \_/ ________/ /  |  $$$$$$/| $$|  $$$$$$/|  $$$$$$/|  $$$$$$$| $$ \  $$|  $$$$$$/
//   \___/ ________/    \______/ |__/ \______/  \______/  \_______/|__/  \__/ \______/
//
// ///////////////////////////////////////////////////////////////////////////////////////
#include "grpcw/util/blocking_queue.hpp"

// third-party
#include <doctest/doctest.h>

// standard
#include <thread>

// declare all functions in the template class to get proper coverage information
template class grpcw::util::BlockingQueue<int>;

namespace {
using namespace grpcw;

TEST_CASE("[grpcw-util] empty_is_empty") {
    util::BlockingQueue<char> bq;
    CHECK(bq.size() == 0);
    CHECK(bq.empty());

    bq.emplace_back('$');

    CHECK(bq.size() == 1);
    CHECK_FALSE(bq.empty());

    bq.emplace_back('M');
    bq.emplace_back('O');
    bq.emplace_back('N');
    bq.emplace_back('E');
    bq.emplace_back('Y');

    CHECK(bq.size() == 6);
    CHECK_FALSE(bq.empty());

    bq.clear();

    CHECK(bq.size() == 0);
    CHECK(bq.empty());
}

TEST_CASE("[grpcw-util] pop_all_but_most_recent") {
    util::BlockingQueue<std::string> strings;

    std::string alphabet = "abcdefghijklmnopqrstuvwxyz";

    for (char c : alphabet) {
        strings.emplace_back(1, c); // emplace_back for specific std::string constructor
    }

    CHECK(strings.pop_all_but_most_recent() == "z");
    CHECK(strings.size() == 1);
}

TEST_CASE_TEMPLATE("[grpcw-util] interleaved_data", T, short, int, float, double) {
    std::vector<T> shared_data;

    // Use two blocking queues as control structures between two threads
    util::BlockingQueue<T> even_bq;
    util::BlockingQueue<T> odds_bq;

    // Wait for the next even number from 'even_bq',
    // Write the number to 'shared_data',
    // Write the next odd number to 'odds_bq'
    std::thread thread([&] {
        for (short i = 1; i < 10; i += 2) {
            T to_add = even_bq.pop_front();
            shared_data.emplace_back(to_add);
            odds_bq.push_back(i);
        }
    });

    // Write the next even number to 'even_bq'
    // Wait for the next odd number from 'odds_bq',
    // Write the number to 'shared_data',
    for (short i = 0; i < 10; i += 2) {
        even_bq.push_back(i);
        T to_add = odds_bq.pop_front();
        shared_data.emplace_back(to_add);
    }

    thread.join();

    // All the data should be processed by now
    CHECK(even_bq.empty());
    CHECK(odds_bq.empty());

    // shared_data should be correctly ordered despite being modified by separate threads
    CHECK(shared_data == std::vector<T>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
}

} // namespace