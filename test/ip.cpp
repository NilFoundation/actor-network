//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Nil Foundation AG
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE ip

#include <nil/actor/network/ip.hpp>

#include <nil/actor/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

#include <nil/actor/ip_address.hpp>
#include <nil/actor/ipv4_address.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace {

    struct fixture : host_fixture {
        fixture() : v6_local {{0}, {0x1}} {
            v4_local = ip_address {make_ipv4_address(127, 0, 0, 1)};
            v4_any_addr = ip_address {make_ipv4_address(0, 0, 0, 0)};
        }

        bool contains(ip_address x) {
            return std::count(addrs.begin(), addrs.end(), x) > 0;
        }

        ip_address v4_any_addr;
        ip_address v6_any_addr;
        ip_address v4_local;
        ip_address v6_local;

        std::vector<ip_address> addrs;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(ip_tests, fixture)

BOOST_AUTO_TEST_CASE(resolve localhost) {
    addrs = ip::resolve("localhost");
    BOOST_CHECK(!addrs.empty());
    BOOST_CHECK(contains(v4_local) || contains(v6_local));
}

BOOST_AUTO_TEST_CASE(resolve any) {
    addrs = ip::resolve("");
    BOOST_CHECK(!addrs.empty());
    BOOST_CHECK(contains(v4_any_addr) || contains(v6_any_addr));
}

BOOST_AUTO_TEST_CASE(local addresses localhost) {
    addrs = ip::local_addresses("localhost");
    BOOST_CHECK(!addrs.empty());
    BOOST_CHECK(contains(v4_local) || contains(v6_local));
}

BOOST_AUTO_TEST_CASE(local addresses any) {
    addrs = ip::local_addresses("0.0.0.0");
    auto tmp = ip::local_addresses("::");
    addrs.insert(addrs.end(), tmp.begin(), tmp.end());
    BOOST_CHECK(!addrs.empty());
    BOOST_CHECK(contains(v4_any_addr) || contains(v6_any_addr));
}

BOOST_AUTO_TEST_SUITE_END()
