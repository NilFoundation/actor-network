//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE tcp_sockets

#include <nil/actor/network/tcp_accept_socket.hpp>
#include <nil/actor/network/tcp_stream_socket.hpp>

#include <nil/actor/test/dsl.hpp>

#include <nil/actor/network/socket_guard.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<>
            struct print_log_value<tcp_stream_socket> {
                void operator()(std::ostream &, tcp_stream_socket const &) {
                }
            };
            template<>
            struct print_log_value<socket> {
                void operator()(std::ostream &, socket const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

namespace {

    // TODO: switch to std::operator""s when switching to C++14
    std::string operator"" _s(const char *str, size_t size) {
        return std::string(str, size);
    }

    struct fixture : host_fixture {
        fixture() {
            auth.port = 0;
            auth.host = "0.0.0.0"_s;
        }

        uri::authority_type auth;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(tcp_sockets_tests, fixture)

BOOST_AUTO_TEST_CASE(open_tcp_port) {
    auto acceptor = unbox(make_tcp_accept_socket(auth, false));
    auto port = unbox(local_port(acceptor));
    BOOST_CHECK_NE(port, 0);
    auto acceptor_guard = make_socket_guard(acceptor);
    BOOST_TEST_MESSAGE("opened acceptor on port " << port);
}

BOOST_AUTO_TEST_CASE(tcp_connect) {
    auto acceptor = unbox(make_tcp_accept_socket(auth, false));
    auto port = unbox(local_port(acceptor));
    BOOST_CHECK_NE(port, 0);
    auto acceptor_guard = make_socket_guard(acceptor);
    BOOST_TEST_MESSAGE("opened acceptor on port " << port);
    uri::authority_type dst;
    dst.port = port;
    dst.host = "localhost"_s;
    BOOST_TEST_MESSAGE("connecting to localhost");
    auto conn = unbox(make_connected_tcp_stream_socket(dst));
    auto conn_guard = make_socket_guard(conn);
    BOOST_CHECK_NE(conn, invalid_socket);
    auto accepted = unbox(accept(acceptor));
    auto accepted_guard = make_socket_guard(conn);
    BOOST_CHECK_NE(accepted, invalid_socket);
    BOOST_TEST_MESSAGE("connected");
}

BOOST_AUTO_TEST_SUITE_END()
