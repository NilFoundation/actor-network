//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE socket_guard

#include <nil/actor/network/socket_guard.hpp>
#include <nil/actor/network/socket_id.hpp>

#include <nil/actor/test/dsl.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace {

    constexpr socket_id dummy_id = 13;

    struct dummy_socket {
        dummy_socket(socket_id id, bool &closed) : id(id), closed(closed) {
            // nop
        }

        dummy_socket(const dummy_socket &) = default;

        dummy_socket &operator=(const dummy_socket &other) {
            id = other.id;
            closed = other.closed;
            return *this;
        }

        socket_id id;
        bool &closed;
    };

    void close(dummy_socket x) {
        x.id = invalid_socket_id;
        x.closed = true;
    }

    struct fixture {
        fixture() : closed {false}, sock {dummy_id, closed} {
            // nop
        }

        bool closed;
        dummy_socket sock;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(socket_guard_tests, fixture)

BOOST_AUTO_TEST_CASE(cleanup) {
    {
        auto guard = make_socket_guard(sock);
        BOOST_CHECK_EQUAL(guard.socket().id, dummy_id);
    }
    BOOST_CHECK(sock.closed);
}

BOOST_AUTO_TEST_CASE(reset) {
    {
        auto guard = make_socket_guard(sock);
        BOOST_CHECK_EQUAL(guard.socket().id, dummy_id);
        guard.release();
        BOOST_CHECK_EQUAL(guard.socket().id, invalid_socket_id);
        guard.reset(sock);
        BOOST_CHECK_EQUAL(guard.socket().id, dummy_id);
    }
    BOOST_CHECK_EQUAL(sock.closed, true);
}

BOOST_AUTO_TEST_CASE(release) {
    {
        auto guard = make_socket_guard(sock);
        BOOST_CHECK_EQUAL(guard.socket().id, dummy_id);
        guard.release();
        BOOST_CHECK_EQUAL(guard.socket().id, invalid_socket_id);
    }
    BOOST_CHECK_EQUAL(sock.closed, false);
}

BOOST_AUTO_TEST_SUITE_END()
