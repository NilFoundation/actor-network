//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE network_backend_tcp

#include <nil/actor/network/backend/tcp.hpp>

#include <nil/actor/network/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

#include <string>
#include <thread>

#include <nil/actor/actor_system_config.hpp>
#include <nil/actor/ip_endpoint.hpp>
#include <nil/actor/network/middleman.hpp>
#include <nil/actor/network/socket_guard.hpp>
#include <nil/actor/network/tcp_accept_socket.hpp>
#include <nil/actor/uri.hpp>

using namespace nil::actor;
using namespace nil::actor::network;
using namespace std::literals::string_literals;

namespace {

    behavior dummy_actor(event_based_actor *) {
        return {
            // nop
        };
    }

    struct earth_node {
        uri operator()() {
            return unbox(make_uri("tcp://earth"));
        }
    };

    struct mars_node {
        uri operator()() {
            return unbox(make_uri("tcp://mars"));
        }
    };

    template<class Node>
    struct config : actor_system_config {
        config() {
            Node this_node;
            put(content, "middleman.this-node", this_node());
            load<middleman, backend::tcp>();
        }
    };

    class planet_driver {
    public:
        virtual ~planet_driver() = default;

        virtual bool handle_io_event() = 0;
    };

    template<class Node>
    class planet : public test_coordinator_fixture<config<Node>> {
    public:
        planet(planet_driver &driver) : mm(this->sys.network_manager()), mpx(mm.mpx()), driver_(driver) {
            mpx->set_thread_id();
        }

        node_id id() const {
            return this->sys.node();
        }

        bool handle_io_event() override {
            return driver_.handle_io_event();
        }

        net::middleman &mm;
        multiplexer_ptr mpx;

    private:
        planet_driver &driver_;
    };

    struct fixture : host_fixture, planet_driver {
        fixture() : earth(*this), mars(*this) {
            earth.run();
            mars.run();
            BOOST_CHECK_EQUAL(earth.mpx->num_socket_managers(), 2);
            BOOST_CHECK_EQUAL(mars.mpx->num_socket_managers(), 2);
        }

        bool handle_io_event() override {
            return earth.mpx->poll_once(false) || mars.mpx->poll_once(false);
        }

        void run() {
            earth.run();
        }

        planet<earth_node> earth;
        planet<mars_node> mars;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(tcp_backend_tests, fixture)

BOOST_AUTO_TEST_SUITE(doorman accept) {
    auto backend = earth.mm.backend("tcp");
    BOOST_CHECK(backend);
    uri::authority_type auth;
    auth.host = "localhost"s;
    auth.port = backend->port();
    BOOST_TEST_MESSAGE("trying to connect to earth at " << ACTOR_ARG(auth));
    auto sock = make_connected_tcp_stream_socket(auth);
    BOOST_CHECK(sock);
    auto guard = make_socket_guard(*sock);
    int runs = 0;
    while (earth.mpx->num_socket_managers() < 3) {
        if (++runs >= 5)
            BOOST_FAIL("doorman did not create endpoint_manager");
        run();
    }
    BOOST_CHECK_EQUAL(earth.mpx->num_socket_managers(), 3);
}

BOOST_AUTO_TEST_SUITE(connect) {
    uri::authority_type auth;
    auth.host = "0.0.0.0"s;
    auth.port = 0;
    auto acceptor = unbox(make_tcp_accept_socket(auth, false));
    auto acc_guard = make_socket_guard(acceptor);
    auto port = unbox(local_port(acc_guard.socket()));
    auto uri_str = std::string("tcp://localhost:") + std::to_string(port);
    BOOST_TEST_MESSAGE("connecting to " << ACTOR_ARG(uri_str));
    BOOST_CHECK(earth.mm.connect(*make_uri(uri_str)));
    auto sock = unbox(accept(acc_guard.socket()));
    auto sock_guard = make_socket_guard(sock);
    handle_io_event();
    BOOST_CHECK_EQUAL(earth.mpx->num_socket_managers(), 3);
}

BOOST_AUTO_TEST_SUITE(publish) {
    auto dummy = earth.sys.spawn(dummy_actor);
    auto path = "dummy"s;
    BOOST_TEST_MESSAGE("publishing actor " << ACTOR_ARG(path));
    earth.mm.publish(dummy, path);
    BOOST_TEST_MESSAGE("check registry for " << ACTOR_ARG(path));
    BOOST_CHECK_NOT_EQUAL(earth.sys.registry().get(path), nullptr);
}

BOOST_AUTO_TEST_SUITE(resolve) {
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    auto sockets = unbox(make_stream_socket_pair());
    auto earth_be = reinterpret_cast<net::backend::tcp *>(earth.mm.backend("tcp"));
    BOOST_CHECK(earth_be->emplace(mars.id(), sockets.first));
    auto mars_be = reinterpret_cast<net::backend::tcp *>(mars.mm.backend("tcp"));
    BOOST_CHECK(mars_be->emplace(earth.id(), sockets.second));
    handle_io_event();
    BOOST_CHECK_EQUAL(earth.mpx->num_socket_managers(), 3);
    BOOST_CHECK_EQUAL(mars.mpx->num_socket_managers(), 3);
    auto dummy = earth.sys.spawn(dummy_actor);
    earth.mm.publish(dummy, "dummy"s);
    auto locator = unbox(make_uri("tcp://earth/name/dummy"s));
    BOOST_TEST_MESSAGE("resolve " << ACTOR_ARG(locator));
    mars.mm.resolve(locator, mars.self);
    mars.run();
    earth.run();
    mars.run();
    mars.self->receive(
        [](strong_actor_ptr &ptr, const std::set<std::string> &) {
            BOOST_TEST_MESSAGE("resolved actor!");
            BOOST_CHECK_NOT_EQUAL(ptr, nullptr);
        },
        [](const error &err) { BOOST_FAIL("got error while resolving: " << ACTOR_ARG(err)); },
        after(std::chrono::seconds(0)) >> [] { BOOST_FAIL("manager did not respond with a proxy."); });
}

BOOST_AUTO_TEST_SUITE_END()