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

#define BOOST_TEST_MODULE pipe_socket

#include <nil/actor/network/pipe_socket.hpp>

#include <nil/actor/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

#include <vector>

#include <nil/actor/byte.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

BOOST_FIXTURE_TEST_SUITE(pipe_socket_tests, host_fixture)

BOOST_AUTO_TEST_CASE(send and receive) {
    std::vector<byte> send_buf {byte(1), byte(2), byte(3), byte(4), byte(5), byte(6), byte(7), byte(8)};
    std::vector<byte> receive_buf;
    receive_buf.resize(100);
    pipe_socket rd_sock;
    pipe_socket wr_sock;
    std::tie(rd_sock, wr_sock) = unbox(make_pipe());
    BOOST_CHECK_EQUAL(write(wr_sock, send_buf), send_buf.size());
    BOOST_CHECK_EQUAL(read(rd_sock, receive_buf), send_buf.size());
    BOOST_CHECK(std::equal(send_buf.begin(), send_buf.end(), receive_buf.begin()));
}

BOOST_AUTO_TEST_SUITE_END()
