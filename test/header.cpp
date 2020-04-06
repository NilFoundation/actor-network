//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE basp.header

#include <nil/actor/network/basp/header.hpp>

#include <nil/actor/test/dsl.hpp>

#include <nil/actor/binary_deserializer.hpp>
#include <nil/actor/binary_serializer.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<template<typename...> class P, typename... T>
            struct print_log_value<P<T...>> {
                void operator()(std::ostream &, P<T...> const &) {
                }
            };

            template<template<typename, std::size_t> class P, typename T, std::size_t S>
            struct print_log_value<P<T, S>> {
                void operator()(std::ostream &, P<T, S> const &) {
                }
            };
            template<>
            struct print_log_value<none_t> {
                void operator()(std::ostream &, none_t const &) {
                }
            };
            template<>
            struct print_log_value<basp::header> {
                void operator()(std::ostream &, basp::header const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

BOOST_AUTO_TEST_CASE(serialization_test) {
    basp::header x {basp::message_type::handshake, 42, 4};
    std::vector<byte> buf;
    {
        binary_serializer sink {nullptr, buf};
        BOOST_CHECK_EQUAL(sink(x), none);
    }
    BOOST_CHECK_EQUAL(buf.size(), basp::header_size);
    auto buf2 = to_bytes(x);
    BOOST_REQUIRE_EQUAL(buf.size(), buf2.size());
    BOOST_CHECK(std::equal(buf.begin(), buf.end(), buf2.begin()));
    basp::header y;
    {
        binary_deserializer source {nullptr, buf};
        BOOST_CHECK_EQUAL(source(y), none);
    }
    BOOST_CHECK_EQUAL(x, y);
    auto z = basp::header::from_bytes(buf);
    BOOST_CHECK_EQUAL(x, z);
    BOOST_CHECK_EQUAL(y, z);
}

BOOST_AUTO_TEST_CASE(to_string_test) {
    basp::header x {basp::message_type::handshake, 42, 4};
    BOOST_CHECK_EQUAL(to_string(x), "basp::header(handshake, 42, 4)");
}
