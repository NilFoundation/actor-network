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

#define BOOST_TEST_MODULE net.basp.ping_pong

#include <nil/actor/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

#include <nil/actor/network/backend/test.hpp>
#include <nil/actor/network/middleman.hpp>
#include <nil/actor/network/multiplexer.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace {

    struct earth_node {
        uri operator()() {
            return unbox(make_uri("test://earth"));
        }
    };

    struct mars_node {
        uri operator()() {
            return unbox(make_uri("test://mars"));
        }
    };

    template<class Node>
    struct config : spawner_config {
        config() {
            Node this_node;
            put(content, "middleman.this-node", this_node());
            load<middleman, backend::test>();
        }
    };

    class planet_driver {
    public:
        virtual ~planet_driver() {
            // nop
        }

        virtual bool consume_message() = 0;

        virtual bool handle_io_event() = 0;

        virtual bool trigger_timeout() = 0;
    };

    template<class Node>
    class planet : public test_coordinator_fixture<config<Node>> {
    public:
        planet(planet_driver &driver) : mpx(*this->sys.network_manager().mpx()), driver_(driver) {
            mpx.set_thread_id();
        }

        network::backend::test &backend() {
            auto &mm = this->sys.network_manager();
            return *dynamic_cast<network::backend::test *>(mm.backend("test"));
        }

        node_id id() const {
            return this->sys.node();
        }

        bool consume_message() override {
            return driver_.consume_message();
        }

        bool handle_io_event() override {
            return driver_.handle_io_event();
        }

        bool trigger_timeout() override {
            return driver_.trigger_timeout();
        }

        actor resolve(string_view locator) {
            auto hdl = actor_cast<actor>(this->self);
            this->sys.network_manager().resolve(unbox(make_uri(locator)), hdl);
            this->run();
            actor result;
            this->self->receive([&](strong_actor_ptr &ptr, const std::set<std::string> &) {
                BOOST_TEST_MESSAGE("resolved " << locator);
                result = actor_cast<actor>(std::move(ptr));
            });
            return result;
        }

        multiplexer &mpx;

    private:
        planet_driver &driver_;
    };

    behavior ping_actor(event_based_actor *self, actor pong, size_t num_pings, std::shared_ptr<size_t> count) {
        BOOST_TEST_MESSAGE("num_pings: " << num_pings);
        self->send(pong, ping_atom_v, 1);
        return {
            [=](pong_atom, int value) {
                BOOST_TEST_MESSAGE("received `pong_atom`");
                if (++*count >= num_pings) {
                    BOOST_TEST_MESSAGE("received " << num_pings << " pings, call self->quit");
                    self->quit();
                }
                return std::make_tuple(ping_atom_v, value + 1);
            },
        };
    }

    behavior pong_actor(event_based_actor *self) {
        BOOST_TEST_MESSAGE("pong actor started");
        self->set_down_handler([=](down_msg &dm) {
            BOOST_TEST_MESSAGE("received down_msg{" << to_string(dm.reason) << "}");
            self->quit(dm.reason);
        });
        return {
            [=](ping_atom, int value) {
                BOOST_TEST_MESSAGE("received `ping_atom` from " << self->current_sender());
                if (self->current_sender() == self->ctrl())
                    abort();
                self->monitor(self->current_sender());
                // set next behavior
                self->become([](ping_atom, int val) { return std::make_tuple(pong_atom_v, val); });
                // reply to 'ping'
                return std::make_tuple(pong_atom_v, value);
            },
        };
    }

    struct fixture : host_fixture, planet_driver {
        fixture() : earth(*this), mars(*this) {
            auto sockets = unbox(make_stream_socket_pair());
            earth.backend().emplace(mars.id(), sockets.first, sockets.second);
            mars.backend().emplace(earth.id(), sockets.second, sockets.first);
            run();
        }

        bool consume_message() override {
            return earth.sched.try_run_once() || mars.sched.try_run_once();
        }

        bool handle_io_event() override {
            return earth.mpx.poll_once(false) || mars.mpx.poll_once(false);
        }

        bool trigger_timeout() override {
            return earth.sched.trigger_timeout() || mars.sched.trigger_timeout();
        }

        void run() {
            earth.run();
        }

        planet<earth_node> earth;
        planet<mars_node> mars;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(ping_pong_tests, fixture)

BOOST_AUTO_TEST_CASE(full setup) {
    auto pong = earth.sys.spawn(pong_actor);
    run();
    earth.sys.registry().put("pong", pong);
    auto remote_pong = mars.resolve("test://earth/name/pong");
    auto count = std::make_shared<size_t>(0);
    auto ping = mars.sys.spawn(ping_actor, remote_pong, 10, count);
    run();
    anon_send_exit(pong, exit_reason::kill);
    anon_send_exit(ping, exit_reason::kill);
    BOOST_CHECK_EQUAL(*count, 10u);
    run();
}

BOOST_AUTO_TEST_SUITE_END()