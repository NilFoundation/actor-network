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

#define BOOST_TEST_MODULE net.tcp_accept_socket

#include <nil/actor/network/tcp_accept_socket.hpp>

#include <nil/actor/serialization/binary_serializer.hpp>
#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/ip.hpp>
#include <nil/actor/network/make_endpoint_manager.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/network/socket_guard.hpp>
#include <nil/actor/network/tcp_stream_socket.hpp>
#include <nil/actor/uri.hpp>

#include <nil/actor/test/dsl.hpp>
#include <nil/actor/test/host_fixture.hpp>

using namespace nil::actor;
using namespace nil::actor::network;
using namespace std::literals::string_literals;

namespace {

    struct fixture : test_coordinator_fixture<>, host_fixture {
        fixture() {
            auth.port = 0;
            auth.host = "0.0.0.0"s;
        }

        uri::authority_type auth;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(doorman_tests, fixture)

BOOST_AUTO_TEST_CASE(tcp_connect) {
    auto acceptor = unbox(make_tcp_accept_socket(auth, false));
    auto port = unbox(local_port(socket_cast<network_socket>(acceptor)));
    auto acceptor_guard = make_socket_guard(acceptor);
    BOOST_TEST_MESSAGE("opened acceptor on port " << port);
    uri::authority_type dst;
    dst.port = port;
    dst.host = "localhost"s;
    auto conn = make_socket_guard(unbox(make_connected_tcp_stream_socket(dst)));
    auto accepted = make_socket_guard(unbox(accept(acceptor)));
    BOOST_TEST_MESSAGE("accepted connection");
}

BOOST_AUTO_TEST_SUITE_END()
