//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE net.doorman

#include <nil/actor/network/doorman.hpp>
#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/ip.hpp>
#include <nil/actor/network/make_endpoint_manager.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/network/socket_guard.hpp>
#include <nil/actor/network/tcp_accept_socket.hpp>

#include <nil/actor/binary_serializer.hpp>
#include <nil/actor/uri.hpp>

#include <nil/actor/test/dsl.hpp>

using namespace nil::actor;
using namespace nil::actor::network;
using namespace std::literals::string_literals;

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<>
            struct print_log_value<error> {
                void operator()(std::ostream &, error const &) {
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

    struct fixture : test_coordinator_fixture<>, host_fixture {
        fixture() {
            mpx = std::make_shared<multiplexer>();
            if (auto err = mpx->init())
                BOOST_FAIL("mpx->init failed: " << sys.render(err));
            mpx->set_thread_id();
            BOOST_CHECK_EQUAL(mpx->num_socket_managers(), 1u);
            auth.port = 0;
            auth.host = "0.0.0.0"s;
        }

        bool handle_io_event() override {
            return mpx->poll_once(false);
        }

        multiplexer_ptr mpx;
        uri::authority_type auth;
    };

    class dummy_application {
    public:
        static expected<std::vector<byte>> serialize(spawner &sys, const message &x) {
            std::vector<byte> result;
            binary_serializer sink {sys, result};
            if (auto err = x.save(sink))
                return err.value();
            return result;
        }

        template<class Parent>
        error init(Parent &) {
            return none;
        }

        template<class Parent>
        void write_message(Parent &parent, std::unique_ptr<endpoint_manager_queue::message> msg) {
            parent.write_packet(msg->payload);
        }

        template<class Parent>
        error handle_data(Parent &, span<const byte>) {
            return none;
        }

        template<class Parent>
        void resolve(Parent &, string_view path, const actor &listener) {
            anon_send(listener, resolve_atom_v, "the resolved path is still " + std::string(path.begin(), path.end()));
        }

        template<class Parent>
        void timeout(Parent &, const std::string &, uint64_t) {
            // nop
        }

        template<class Parent>
        void new_proxy(Parent &, actor_id) {
            // nop
        }

        template<class Parent>
        void local_actor_down(Parent &, actor_id, error) {
            // nop
        }

        void handle_error(sec) {
            // nop
        }
    };

    class dummy_application_factory {
    public:
        using application_type = dummy_application;

        static expected<std::vector<byte>> serialize(spawner &sys, const message &x) {
            return dummy_application::serialize(sys, x);
        }

        template<class Parent>
        error init(Parent &) {
            return none;
        }

        application_type make() const {
            return dummy_application {};
        }
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(doorman_tests, fixture)

BOOST_AUTO_TEST_CASE(doorman_accept) {
    auto acceptor = unbox(make_tcp_accept_socket(auth, false));
    auto port = unbox(local_port(socket_cast<network_socket>(acceptor)));
    auto acceptor_guard = make_socket_guard(acceptor);
    BOOST_TEST_MESSAGE("opened acceptor on port " << port);
    auto mgr = make_endpoint_manager(
        mpx, sys, doorman<dummy_application_factory> {acceptor_guard.release(), dummy_application_factory {}});
    BOOST_CHECK_EQUAL(mgr->init(), none);
    auto before = mpx->num_socket_managers();
    BOOST_CHECK_EQUAL(before, 2u);
    uri::authority_type dst;
    dst.port = port;
    dst.host = "localhost"s;
    BOOST_TEST_MESSAGE("connecting to doorman");
    auto conn = make_socket_guard(unbox(make_connected_tcp_stream_socket(dst)));
    BOOST_TEST_MESSAGE("waiting for connection");
    while (mpx->num_socket_managers() != before + 1)
        run();
    BOOST_TEST_MESSAGE("connected");
}

BOOST_AUTO_TEST_SUITE_END()
