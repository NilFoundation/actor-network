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

#define BOOST_TEST_MODULE network_socket

#include <nil/actor/network/network_socket.hpp>

#include <nil/actor/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

BOOST_FIXTURE_TEST_SUITE(network_socket_tests, host_fixture)

BOOST_AUTO_TEST_CASE(invalid socket) {
    network_socket x;
    BOOST_CHECK_EQUAL(allow_udp_connreset(x, true), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(send_buffer_size(x), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(local_port(x), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(local_addr(x), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(remote_port(x), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(remote_addr(x), sec::network_syscall_failed);
}

BOOST_AUTO_TEST_SUITE_END()
