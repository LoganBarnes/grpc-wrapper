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
#include "tag.hpp"

// standard
#include <cassert>
#include <memory>

Tag::Tag(void* d, TagLabel l) : data(d), label(l) {}

::std::ostream& operator<<(::std::ostream& os, const Tag& tag) {
    os << '{' << tag.data << ", ";

    switch (tag.label) {

    case TagLabel::NewRpc:
        os << "NewRpc";
        break;

    case TagLabel::Done:
        os << "Done";
        break;

    case TagLabel::Writing:
        os << "Writing";
        break;
    }
    return os << '}';
}

namespace detail {

void* make_tag(void* data, TagLabel label, std::unordered_map<void*, std::unique_ptr<Tag>>* tags) {
    auto&& tag = std::make_unique<Tag>(data, label);
    void* result = tag.get();
    tags->emplace(result, std::forward<decltype(tag)>(tag));
    return result;
}

Tag get_tag(void* key, std::unordered_map<void*, std::unique_ptr<Tag>>* tags) {
    if (tags->find(key) == tags->end()) {
        throw std::runtime_error("provided tag does not exist in the map");
    }
    Tag tag_copy = *tags->at(key);
    tags->erase(key);
    return tag_copy;
}

} // namespace detail
