//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE basp.stream_application

#include <nil/actor/network/basp/application.hpp>
#include <nil/actor/network/basp/connection_state.hpp>
#include <nil/actor/network/basp/constants.hpp>
#include <nil/actor/network/backend/test.hpp>
#include <nil/actor/network/make_endpoint_manager.hpp>
#include <nil/actor/network/middleman.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/network/stream_socket.hpp>
#include <nil/actor/network/stream_transport.hpp>

#include <nil/actor/test/dsl.hpp>

#include <vector>

#include <nil/actor/byte.hpp>
#include <nil/actor/uri.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

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

            template<>
            struct print_log_value<basp::connection_state> {
                void operator()(std::ostream &, basp::connection_state const &) {
                }
            };
            template<>
            struct print_log_value<basp::message_type> {
                void operator()(std::ostream &, basp::message_type const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

#define REQUIRE_OK(statement) \
    if (auto err = statement) \
        BOOST_FAIL(sys.render(err));

namespace {

    using buffer_type = std::vector<byte>;

    using transport_type = stream_transport<basp::application>;

    size_t fetch_size(variant<size_t, sec> x) {
        if (holds_alternative<sec>(x))
            BOOST_FAIL("read/write failed: " << to_string(get<sec>(x)));
        return get<size_t>(x);
    }

    struct config : spawner_config {
        config() {
            put(content, "middleman.this-node", unbox(make_uri("test:earth")));
            load<middleman, backend::test>();
        }
    };

    struct fixture : host_fixture, test_coordinator_fixture<config> {
        fixture() : mars(make_node_id(unbox(make_uri("test:mars")))) {
            auto &mm = sys.network_manager();
            mm.mpx()->set_thread_id();
            auto backend = dynamic_cast<backend::test *>(mm.backend("test"));
            auto mgr = backend->peer(mars);
            auto &dref = dynamic_cast<endpoint_manager_impl<transport_type> &>(*mgr);
            app = &dref.transport().application();
            sock = backend->socket(mars);
        }

        bool handle_io_event() override {
            auto mpx = sys.network_manager().mpx();
            return mpx->poll_once(false);
        }

        template<class... Ts>
        buffer_type to_buf(const Ts &... xs) {
            buffer_type buf;
            binary_serializer sink {system(), buf};
            REQUIRE_OK(sink(xs...));
            return buf;
        }

        template<class... Ts>
        void mock(const Ts &... xs) {
            auto buf = to_buf(xs...);
            if (fetch_size(write(sock, buf)) != buf.size())
                BOOST_FAIL("unable to write " << buf.size() << " bytes");
            run();
        }

        void handle_handshake() {
            BOOST_CHECK_EQUAL(app->state(), basp::connection_state::await_handshake_header);
            auto payload = to_buf(mars, defaults::middleman::app_identifiers);
            mock(basp::header {basp::message_type::handshake, static_cast<uint32_t>(payload.size()), basp::version});
            BOOST_CHECK_EQUAL(app->state(), basp::connection_state::await_handshake_payload);
            write(sock, payload);
            run();
        }

        void consume_handshake() {
            buffer_type buf(basp::header_size);
            if (fetch_size(read(sock, buf)) != basp::header_size)
                BOOST_FAIL("unable to read " << basp::header_size << " bytes");
            auto hdr = basp::header::from_bytes(buf);
            if (hdr.type != basp::message_type::handshake || hdr.payload_len == 0 ||
                hdr.operation_data != basp::version)
                BOOST_FAIL("invalid handshake header");
            buf.resize(hdr.payload_len);
            if (fetch_size(read(sock, buf)) != hdr.payload_len)
                BOOST_FAIL("unable to read " << hdr.payload_len << " bytes");
            node_id nid;
            std::vector<std::string> app_ids;
            binary_deserializer source {sys, buf};
            if (auto err = source(nid, app_ids))
                BOOST_FAIL("unable to deserialize payload: " << sys.render(err));
            if (source.remaining() > 0)
                BOOST_FAIL("trailing bytes after reading payload");
        }

        spawner &system() {
            return sys;
        }

        node_id mars;

        stream_socket sock;

        basp::application *app;

        unit_t no_payload;
    };

}    // namespace

#define MOCK(kind, op, ...)                                                                     \
    do {                                                                                        \
        BOOST_TEST_MESSAGE("mock");                                                             \
        if (!std::is_same<decltype(std::make_tuple(__VA_ARGS__)), std::tuple<unit_t>>::value) { \
            auto payload = to_buf(__VA_ARGS__);                                                 \
            mock(basp::header {kind, static_cast<uint32_t>(payload.size()), op});               \
            write(sock, payload);                                                               \
        } else {                                                                                \
            mock(basp::header {kind, 0, op});                                                   \
        }                                                                                       \
        run();                                                                                  \
    } while (false)

#define RECEIVE(msg_type, op_data, ...)                                                         \
    do {                                                                                        \
        BOOST_TEST_MESSAGE("receive");                                                          \
        buffer_type buf(basp::header_size);                                                     \
        if (fetch_size(read(sock, buf)) != basp::header_size)                                   \
            BOOST_FAIL("unable to read " << basp::header_size << " bytes");                     \
        auto hdr = basp::header::from_bytes(buf);                                               \
        BOOST_CHECK_EQUAL(hdr.type, msg_type);                                                  \
        BOOST_CHECK_EQUAL(hdr.operation_data, op_data);                                         \
        if (!std::is_same<decltype(std::make_tuple(__VA_ARGS__)), std::tuple<unit_t>>::value) { \
            buf.resize(hdr.payload_len);                                                        \
            if (fetch_size(read(sock, buf)) != size_t {hdr.payload_len})                        \
                BOOST_FAIL("unable to read " << hdr.payload_len << " bytes");                   \
            binary_deserializer source {sys, buf};                                              \
            if (auto err = source(__VA_ARGS__))                                                 \
                BOOST_FAIL("failed to receive data: " << sys.render(err));                      \
        } else {                                                                                \
            if (hdr.payload_len != 0)                                                           \
                BOOST_FAIL("unexpected payload");                                               \
        }                                                                                       \
    } while (false)

BOOST_FIXTURE_TEST_SUITE(application_tests, fixture)

BOOST_AUTO_TEST_CASE(actor_message_and_down_message) {
    handle_handshake();
    consume_handshake();
    sys.registry().put(self->id(), self);
    BOOST_REQUIRE_EQUAL(self->mailbox().size(), 0u);
    MOCK(basp::message_type::actor_message, make_message_id().integer_value(), mars, actor_id {42}, self->id(),
         std::vector<strong_actor_ptr> {}, make_message("hello world!"));
    MOCK(basp::message_type::monitor_message, 42u, no_payload);
    strong_actor_ptr proxy;
    self->receive([&](const std::string &str) {
        BOOST_CHECK_EQUAL(str, "hello world!");
        proxy = self->current_sender();
        BOOST_REQUIRE_NE(proxy, nullptr);
        self->monitor(proxy);
    });
    MOCK(basp::message_type::down_message, 42u, error {exit_reason::user_shutdown});
    expect((down_msg), from(_).to(self).with(down_msg {actor_cast<actor_addr>(proxy), exit_reason::user_shutdown}));
}

BOOST_AUTO_TEST_CASE(resolve_request_without_result) {
    handle_handshake();
    consume_handshake();
    BOOST_CHECK_EQUAL(app->state(), basp::connection_state::await_header);
    MOCK(basp::message_type::resolve_request, 42, std::string {"foo/bar"});
    BOOST_CHECK_EQUAL(app->state(), basp::connection_state::await_header);
    actor_id aid;
    std::set<std::string> ifs;
    RECEIVE(basp::message_type::resolve_response, 42u, aid, ifs);
    BOOST_CHECK_EQUAL(aid, 0u);
    BOOST_CHECK(ifs.empty());
}

BOOST_AUTO_TEST_CASE(resolve_request_on_id_with_result) {
    handle_handshake();
    consume_handshake();
    sys.registry().put(self->id(), self);
    auto path = "id/" + std::to_string(self->id());
    BOOST_CHECK_EQUAL(app->state(), basp::connection_state::await_header);
    MOCK(basp::message_type::resolve_request, 42, path);
    BOOST_CHECK_EQUAL(app->state(), basp::connection_state::await_header);
    actor_id aid;
    std::set<std::string> ifs;
    RECEIVE(basp::message_type::resolve_response, 42u, aid, ifs);
    BOOST_CHECK_EQUAL(aid, self->id());
    BOOST_CHECK(ifs.empty());
}

BOOST_AUTO_TEST_SUITE_END()
