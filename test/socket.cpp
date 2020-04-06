//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE socket

#include <nil/actor/network/socket.hpp>

#include <nil/actor/test/dsl.hpp>

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
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

BOOST_FIXTURE_TEST_SUITE(socket_tests, host_fixture)

BOOST_AUTO_TEST_CASE(invalid_socket_test) {
    auto x = invalid_socket;
    BOOST_CHECK_EQUAL(x.id, invalid_socket_id);
    BOOST_CHECK_EQUAL(child_process_inherit(x, true), sec::network_syscall_failed);
    BOOST_CHECK_EQUAL(nonblocking(x, true), sec::network_syscall_failed);
}

BOOST_AUTO_TEST_SUITE_END()
