//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE stream_socket

#include <nil/actor/network/stream_socket.hpp>

#include <nil/actor/test/dsl.hpp>

#include <nil/actor/byte.hpp>
#include <nil/actor/span.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<>
            struct print_log_value<error> {
                void operator()(std::ostream &, error const &) {
                }
            };
            template<>
            struct print_log_value<sec> {
                void operator()(std::ostream &, sec const &) {
                }
            };
            template<>
            struct print_log_value<none_t> {
                void operator()(std::ostream &, none_t const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

namespace {

    byte operator"" _b(unsigned long long x) {
        return static_cast<byte>(x);
    }

}    // namespace

BOOST_FIXTURE_TEST_SUITE(network_socket_simple_tests, host_fixture)

BOOST_AUTO_TEST_CASE(invalid_socket) {
    stream_socket x;
    BOOST_CHECK_EQUAL(keepalive(x, true), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(nodelay(x, true), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(allow_sigpipe(x, true), sec::network_syscall_failed);
}

BOOST_AUTO_TEST_SUITE_END()

namespace {

    struct fixture : host_fixture {
        fixture() : rd_buf(124) {
            std::tie(first, second) = unbox(make_stream_socket_pair());
            BOOST_REQUIRE_EQUAL(nonblocking(first, true), nil::actor::none);
            BOOST_REQUIRE_EQUAL(nonblocking(second, true), nil::actor::none);
            BOOST_REQUIRE_NE(unbox(send_buffer_size(first)), 0u);
            BOOST_REQUIRE_NE(unbox(send_buffer_size(second)), 0u);
        }

        ~fixture() {
            close(first);
            close(second);
        }

        stream_socket first;
        stream_socket second;
        std::vector<byte> rd_buf;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(network_socket_tests, fixture)

BOOST_AUTO_TEST_CASE(read_on_empty_sockets) {
    BOOST_CHECK_EQUAL(get<sec>(read(first, rd_buf)), sec::unavailable_or_would_block);
    BOOST_CHECK_EQUAL(get<sec>(read(second, rd_buf)), sec::unavailable_or_would_block);
}

BOOST_AUTO_TEST_CASE(transfer_data_from_first_to_second_socket) {
    std::vector<byte> wr_buf {1_b, 2_b, 4_b, 8_b, 16_b, 32_b, 64_b};
    BOOST_TEST_MESSAGE("transfer data from first to second socket");
    BOOST_CHECK_EQUAL(get<std::size_t>(write(first, wr_buf)), wr_buf.size());
    BOOST_CHECK_EQUAL(get<std::size_t>(read(second, rd_buf)), wr_buf.size());
    BOOST_CHECK(std::equal(wr_buf.begin(), wr_buf.end(), rd_buf.begin()));
    rd_buf.assign(rd_buf.size(), byte(0));
}

BOOST_AUTO_TEST_CASE(transfer_data_from_second_to_first_socket) {
    std::vector<byte> wr_buf {1_b, 2_b, 4_b, 8_b, 16_b, 32_b, 64_b};
    BOOST_CHECK_EQUAL(get<std::size_t>(write(second, wr_buf)), wr_buf.size());
    BOOST_CHECK_EQUAL(get<std::size_t>(read(first, rd_buf)), wr_buf.size());
    BOOST_CHECK(std::equal(wr_buf.begin(), wr_buf.end(), rd_buf.begin()));
}

BOOST_AUTO_TEST_CASE(shut_down_first_socket_and_observe_shutdown_on_the_second_one) {
    close(first);
    BOOST_CHECK_EQUAL(get<sec>(read(second, rd_buf)), sec::socket_disconnected);
    first.id = invalid_socket_id;
}

BOOST_AUTO_TEST_CASE(transfer_data_using_multiple_buffers) {
    std::vector<byte> wr_buf_1 {1_b, 2_b, 4_b};
    std::vector<byte> wr_buf_2 {8_b, 16_b, 32_b, 64_b};
    std::vector<byte> full_buf;
    full_buf.insert(full_buf.end(), wr_buf_1.begin(), wr_buf_1.end());
    full_buf.insert(full_buf.end(), wr_buf_2.begin(), wr_buf_2.end());
    BOOST_CHECK_EQUAL(get<std::size_t>(write(second, {make_span(wr_buf_1), make_span(wr_buf_2)})), full_buf.size());
    BOOST_CHECK_EQUAL(get<std::size_t>(read(first, rd_buf)), full_buf.size());
    BOOST_CHECK(std::equal(full_buf.begin(), full_buf.end(), rd_buf.begin()));
}

BOOST_AUTO_TEST_SUITE_END()
