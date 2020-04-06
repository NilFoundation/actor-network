//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE net.basp.message_queue

#include <nil/actor/network/basp/message_queue.hpp>

#include <nil/actor/test/dsl.hpp>

#include <nil/actor/actor_cast.hpp>
#include <nil/actor/spawner.hpp>
#include <nil/actor/behavior.hpp>

using namespace nil::actor;

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
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

namespace {

    behavior testee_impl() {
        return {
            [](ok_atom, int) {
                // nop
            },
        };
    }

    struct fixture : test_coordinator_fixture<> {
        network::basp::message_queue queue;
        strong_actor_ptr testee;

        fixture() {
            auto hdl = sys.spawn<lazy_init>(testee_impl);
            testee = actor_cast<strong_actor_ptr>(hdl);
        }

        void acquire_ids(size_t num) {
            for (size_t i = 0; i < num; ++i)
                queue.new_id();
        }

        void push(int msg_id) {
            queue.push(nullptr, static_cast<uint64_t>(msg_id), testee,
                       make_mailbox_element(self->ctrl(), make_message_id(), {}, ok_atom_v, msg_id));
        }
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(message_queue_tests, fixture)

BOOST_AUTO_TEST_CASE(default_construction) {
    BOOST_CHECK_EQUAL(queue.next_id, 0u);
    BOOST_CHECK_EQUAL(queue.next_undelivered, 0u);
    BOOST_CHECK_EQUAL(queue.pending.size(), 0u);
}

BOOST_AUTO_TEST_CASE(ascending_ids) {
    BOOST_CHECK_EQUAL(queue.new_id(), 0u);
    BOOST_CHECK_EQUAL(queue.new_id(), 1u);
    BOOST_CHECK_EQUAL(queue.new_id(), 2u);
    BOOST_CHECK_EQUAL(queue.next_undelivered, 0u);
}

BOOST_AUTO_TEST_CASE(push_order_0_1_2) {
    acquire_ids(3);
    push(0);
    expect((ok_atom, int), from(self).to(testee).with(_, 0));
    push(1);
    expect((ok_atom, int), from(self).to(testee).with(_, 1));
    push(2);
    expect((ok_atom, int), from(self).to(testee).with(_, 2));
}

BOOST_AUTO_TEST_CASE(push_order_0_2_1) {
    acquire_ids(3);
    push(0);
    expect((ok_atom, int), from(self).to(testee).with(_, 0));
    push(2);
    disallow((ok_atom, int), from(self).to(testee));
    push(1);
    expect((ok_atom, int), from(self).to(testee).with(_, 1));
    expect((ok_atom, int), from(self).to(testee).with(_, 2));
}

BOOST_AUTO_TEST_CASE(push_order_1_0_2) {
    acquire_ids(3);
    push(1);
    disallow((ok_atom, int), from(self).to(testee));
    push(0);
    expect((ok_atom, int), from(self).to(testee).with(_, 0));
    expect((ok_atom, int), from(self).to(testee).with(_, 1));
    push(2);
    expect((ok_atom, int), from(self).to(testee).with(_, 2));
}

BOOST_AUTO_TEST_CASE(push_order_1_2_0) {
    acquire_ids(3);
    push(1);
    disallow((ok_atom, int), from(self).to(testee));
    push(2);
    disallow((ok_atom, int), from(self).to(testee));
    push(0);
    expect((ok_atom, int), from(self).to(testee).with(_, 0));
    expect((ok_atom, int), from(self).to(testee).with(_, 1));
    expect((ok_atom, int), from(self).to(testee).with(_, 2));
}

BOOST_AUTO_TEST_CASE(push_order_2_0_1) {
    acquire_ids(3);
    push(2);
    disallow((ok_atom, int), from(self).to(testee));
    push(0);
    expect((ok_atom, int), from(self).to(testee).with(_, 0));
    push(1);
    expect((ok_atom, int), from(self).to(testee).with(_, 1));
    expect((ok_atom, int), from(self).to(testee).with(_, 2));
}

BOOST_AUTO_TEST_CASE(push_order_2_1_0) {
    acquire_ids(3);
    push(2);
    disallow((ok_atom, int), from(self).to(testee));
    push(1);
    disallow((ok_atom, int), from(self).to(testee));
    push(0);
    expect((ok_atom, int), from(self).to(testee).with(_, 0));
    expect((ok_atom, int), from(self).to(testee).with(_, 1));
    expect((ok_atom, int), from(self).to(testee).with(_, 2));
}

BOOST_AUTO_TEST_CASE(dropping) {
    acquire_ids(3);
    push(2);
    disallow((ok_atom, int), from(self).to(testee));
    queue.drop(nullptr, 1);
    disallow((ok_atom, int), from(self).to(testee));
    push(0);
    expect((ok_atom, int), from(self).to(testee).with(_, 0));
    expect((ok_atom, int), from(self).to(testee).with(_, 2));
}

BOOST_AUTO_TEST_SUITE_END()
