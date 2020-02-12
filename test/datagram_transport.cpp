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

#define BOOST_TEST_MODULE datagram_transport

#include <nil/actor/network/datagram_transport.hpp>

#include <nil/actor/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

#include <nil/actor/serialization/binary_serializer.hpp>
#include <nil/actor/byte.hpp>
#include <nil/actor/detail/socket_sys_includes.hpp>
#include <nil/actor/make_actor.hpp>
#include <nil/actor/network/actor_proxy_impl.hpp>
#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/endpoint_manager_impl.hpp>
#include <nil/actor/network/ip.hpp>
#include <nil/actor/network/make_endpoint_manager.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/network/udp_datagram_socket.hpp>
#include <nil/actor/span.hpp>

using namespace nil::actor;
using namespace nil::actor::network;
using namespace nil::actor::network::ip;

namespace {

    constexpr string_view hello_manager = "hello manager!";

    class dummy_application_factory;

    struct fixture : test_coordinator_fixture<>, host_fixture {
        using buffer_type = std::vector<byte>;

        using buffer_ptr = std::shared_ptr<buffer_type>;

        fixture() : shared_buf(std::make_shared<buffer_type>(1024)) {
            mpx = std::make_shared<multiplexer>();
            if (auto err = mpx->init())
                BOOST_FAIL("mpx->init failed: " << sys.render(err));
            mpx->set_thread_id();
            BOOST_CHECK_EQUAL(mpx->num_socket_managers(), 1u);
            auto addresses = local_addresses("localhost");
            BOOST_CHECK(!addresses.empty());
            ep = ip_endpoint(*addresses.begin(), 0);
            auto send_pair = unbox(make_udp_datagram_socket(ep));
            send_socket = send_pair.first;
            auto receive_pair = unbox(make_udp_datagram_socket(ep));
            recv_socket = receive_pair.first;
            ep.port(htons(receive_pair.second));
            BOOST_TEST_MESSAGE("sending message to " << ACTOR_ARG(ep));
            if (auto err = nonblocking(recv_socket, true))
                BOOST_FAIL("nonblocking() returned an error: " << err);
        }

        ~fixture() {
            close(send_socket);
            close(recv_socket);
        }

        bool handle_io_event() override {
            return mpx->poll_once(false);
        }

        error read_from_socket(udp_datagram_socket sock, buffer_type &buf) {
            uint8_t receive_attempts = 0;
            variant<std::pair<size_t, ip_endpoint>, sec> read_ret;
            do {
                read_ret = read(sock, buf);
                if (auto read_res = get_if<std::pair<size_t, ip_endpoint>>(&read_ret)) {
                    buf.resize(read_res->first);
                } else if (get<sec>(read_ret) != sec::unavailable_or_would_block) {
                    return make_error(get<sec>(read_ret), "read failed");
                }
                if (++receive_attempts > 100)
                    return make_error(sec::runtime_error, "too many unavailable_or_would_blocks");
            } while (read_ret.index() != 0);
            return none;
        }

        multiplexer_ptr mpx;
        buffer_ptr shared_buf;
        ip_endpoint ep;
        udp_datagram_socket send_socket;
        udp_datagram_socket recv_socket;
    };

    class dummy_application {
        using buffer_type = std::vector<byte>;

        using buffer_ptr = std::shared_ptr<buffer_type>;

    public:
        explicit dummy_application(buffer_ptr rec_buf) :
            rec_buf_(std::move(rec_buf)) {
                // nop
            };

        template<class Parent>
        error init(Parent &) {
            return none;
        }

        template<class Transport>
        void write_message(Transport &transport, std::unique_ptr<endpoint_manager_queue::message> msg) {
            transport.write_packet(msg->payload);
        }

        template<class Parent>
        error handle_data(Parent &, span<const byte> data) {
            rec_buf_->clear();
            rec_buf_->insert(rec_buf_->begin(), data.begin(), data.end());
            return none;
        }

        template<class Parent>
        void resolve(Parent &parent, string_view path, const actor &listener) {
            actor_id aid = 42;
            auto uri = unbox(make_uri("test:/id/42"));
            auto nid = make_node_id(uri);
            actor_config cfg;
            endpoint_manager_ptr ptr {&parent.manager()};
            auto p = make_actor<actor_proxy_impl, strong_actor_ptr>(aid, nid, &parent.system(), cfg, std::move(ptr));
            anon_send(listener, resolve_atom_v, std::string {path.begin(), path.end()}, p);
        }

        template<class Parent>
        void new_proxy(Parent &, actor_id) {
            // nop
        }

        template<class Parent>
        void local_actor_down(Parent &, actor_id, error) {
            // nop
        }

        template<class Parent>
        void timeout(Parent &, const std::string &, uint64_t) {
            // nop
        }

        void handle_error(sec sec) {
            BOOST_FAIL("handle_error called: " << to_string(sec));
        }

        static expected<buffer_type> serialize(spawner &sys, const type_erased_tuple &x) {
            buffer_type result;
            binary_serializer sink {sys, result};
            if (auto err = message::save(sink, x))
                return err.value();
            return result;
        }

    private:
        buffer_ptr rec_buf_;
    };

    class dummy_application_factory {
        using buffer_ptr = std::shared_ptr<std::vector<byte>>;

    public:
        using application_type = dummy_application;

        explicit dummy_application_factory(buffer_ptr buf) : buf_(std::move(buf)) {
            // nop
        }

        dummy_application make() {
            return dummy_application {buf_};
        }

    private:
        buffer_ptr buf_;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(datagram_transport_tests, fixture)

BOOST_AUTO_TEST_CASE(receive) {
    using transport_type = datagram_transport<dummy_application_factory>;
    if (auto err = nonblocking(recv_socket, true))
        BOOST_FAIL("nonblocking() returned an error: " << sys.render(err));
    auto mgr = make_endpoint_manager(mpx, sys, transport_type {recv_socket, dummy_application_factory {shared_buf}});
    BOOST_CHECK_EQUAL(mgr->init(), none);
    auto mgr_impl = mgr.downcast<endpoint_manager_impl<transport_type>>();
    BOOST_CHECK(mgr_impl != nullptr);
    auto &transport = mgr_impl->transport();
    transport.configure_read(network::receive_policy::exactly(hello_manager.size()));
    BOOST_CHECK_EQUAL(mpx->num_socket_managers(), 2u);
    BOOST_CHECK_EQUAL(write(send_socket, as_bytes(make_span(hello_manager)), ep), hello_manager.size());
    BOOST_TEST_MESSAGE("wrote " << hello_manager.size() << " bytes.");
    run();
    BOOST_CHECK_EQUAL(string_view(reinterpret_cast<char *>(shared_buf->data()), shared_buf->size()), hello_manager);
}

BOOST_AUTO_TEST_CASE(resolve and proxy communication) {
    using transport_type = datagram_transport<dummy_application_factory>;
    buffer_type recv_buf(1024);
    auto uri = unbox(make_uri("test:/id/42"));
    auto mgr = make_endpoint_manager(mpx, sys, transport_type {send_socket, dummy_application_factory {shared_buf}});
    BOOST_CHECK_EQUAL(mgr->init(), none);
    auto mgr_impl = mgr.downcast<endpoint_manager_impl<transport_type>>();
    BOOST_CHECK(mgr_impl != nullptr);
    auto &transport = mgr_impl->transport();
    transport.add_new_worker(make_node_id(uri), ep);
    run();
    mgr->resolve(uri, self);
    run();
    self->receive(
        [&](resolve_atom, const std::string &, const strong_actor_ptr &p) {
            BOOST_TEST_MESSAGE("got a proxy, send a message to it");
            self->send(actor_cast<actor>(p), "hello proxy!");
        },
        after(std::chrono::seconds(0)) >> [&] { BOOST_FAIL("manager did not respond with a proxy."); });
    run();
    BOOST_CHECK_EQUAL(read_from_socket(recv_socket, recv_buf), none);
    BOOST_TEST_MESSAGE("receive buffer contains " << recv_buf.size() << " bytes");
    message msg;
    binary_deserializer source {sys, recv_buf};
    BOOST_CHECK_EQUAL(source(msg), none);
    if (msg.match_elements<std::string>())
        BOOST_CHECK_EQUAL(msg.get_as<std::string>(0), "hello proxy!");
    else
        ACTOR_ERROR("expected a string, got: " << to_string(msg));
}

BOOST_AUTO_TEST_SUITE_END()
