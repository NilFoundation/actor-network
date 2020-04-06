//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE convert_ip_endpoint

#include <nil/actor/detail/convert_ip_endpoint.hpp>
#include <nil/actor/detail/socket_sys_includes.hpp>

#include <nil/actor/test/dsl.hpp>

#include <cstring>

#include <nil/actor/ipv4_endpoint.hpp>
#include <nil/actor/ipv6_endpoint.hpp>

using namespace nil::actor;
using namespace nil::actor::detail;

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<>
            struct print_log_value<error> {
                void operator()(std::ostream &, error const &) {
                }
            };
            template<>
            struct print_log_value<none_t> {
                void operator()(std::ostream &, none_t const &) {
                }
            };
            template<>
            struct print_log_value<ipv4_endpoint> {
                void operator()(std::ostream &, ipv4_endpoint const &) {
                }
            };
            template<>
            struct print_log_value<ipv6_endpoint> {
                void operator()(std::ostream &, ipv6_endpoint const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

namespace {

    struct fixture : host_fixture {
        fixture() : host_fixture() {
            memset(&sockaddr4_src, 0, sizeof(sockaddr_storage));
            memset(&sockaddr6_src, 0, sizeof(sockaddr_storage));
            auto sockaddr6_ptr = reinterpret_cast<sockaddr_in6 *>(&sockaddr6_src);
            sockaddr6_ptr->sin6_family = AF_INET6;
            sockaddr6_ptr->sin6_port = htons(23);
            sockaddr6_ptr->sin6_addr = in6addr_loopback;
            auto sockaddr4_ptr = reinterpret_cast<sockaddr_in *>(&sockaddr4_src);
            sockaddr4_ptr->sin_family = AF_INET;
            sockaddr4_ptr->sin_port = htons(23);
            sockaddr4_ptr->sin_addr.s_addr = INADDR_LOOPBACK;
        }

        sockaddr_storage sockaddr6_src;
        sockaddr_storage sockaddr4_src;
        sockaddr_storage dst;
        ip_endpoint ep_src;
        ip_endpoint ep_dst;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(convert_ip_endpoint_tests, fixture)

BOOST_AUTO_TEST_CASE(sockaddr_in6_roundtrip) {
    ip_endpoint tmp;
    BOOST_TEST_MESSAGE("converting sockaddr_in6 to ip_endpoint");
    BOOST_CHECK_EQUAL(convert(sockaddr6_src, tmp), none);
    BOOST_TEST_MESSAGE("converting ip_endpoint to sockaddr_in6");
    convert(tmp, dst);
    BOOST_CHECK_EQUAL(memcmp(&sockaddr6_src, &dst, sizeof(sockaddr_storage)), 0);
}

BOOST_AUTO_TEST_CASE(ipv6_endpoint_roundtrip) {
    sockaddr_storage tmp = {};
    if (auto err = detail::parse("[::1]:55555", ep_src))
        BOOST_FAIL("unable to parse input");
    BOOST_TEST_MESSAGE("converting ip_endpoint to sockaddr_in6");
    convert(ep_src, tmp);
    BOOST_TEST_MESSAGE("converting sockaddr_in6 to ip_endpoint");
    BOOST_CHECK_EQUAL(convert(tmp, ep_dst), none);
    BOOST_CHECK_EQUAL(ep_src, ep_dst);
}

BOOST_AUTO_TEST_CASE(sockaddr_in4_roundtrip) {
    ip_endpoint tmp;
    BOOST_TEST_MESSAGE("converting sockaddr_in to ip_endpoint");
    BOOST_CHECK_EQUAL(convert(sockaddr4_src, tmp), none);
    BOOST_TEST_MESSAGE("converting ip_endpoint to sockaddr_in");
    convert(tmp, dst);
    BOOST_CHECK_EQUAL(memcmp(&sockaddr4_src, &dst, sizeof(sockaddr_storage)), 0);
}

BOOST_AUTO_TEST_CASE(ipv4_endpoint_roundtrip) {
    sockaddr_storage tmp = {};
    if (auto err = detail::parse("127.0.0.1:55555", ep_src))
        BOOST_FAIL("unable to parse input");
    BOOST_TEST_MESSAGE("converting ip_endpoint to sockaddr_in");
    convert(ep_src, tmp);
    BOOST_TEST_MESSAGE("converting sockaddr_in to ip_endpoint");
    BOOST_CHECK_EQUAL(convert(tmp, ep_dst), none);
    BOOST_CHECK_EQUAL(ep_src, ep_dst);
}

BOOST_AUTO_TEST_SUITE_END()
