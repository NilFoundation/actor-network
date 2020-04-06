//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE basp.application

#include <nil/actor/network/basp/application.hpp>
#include <nil/actor/network/basp/connection_state.hpp>
#include <nil/actor/network/basp/constants.hpp>
#include <nil/actor/network/basp/ec.hpp>
#include <nil/actor/network/packet_writer.hpp>

#include <nil/actor/test/dsl.hpp>

#include <vector>

#include <nil/actor/byte.hpp>
#include <nil/actor/forwarding_actor_proxy.hpp>
#include <nil/actor/none.hpp>
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

            template<>
            struct print_log_value<basp::ec> {
                void operator()(std::ostream &, basp::ec const &) {
                }
            };

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

#define REQUIRE_OK(statement) \
    if (auto err = statement) \
        BOOST_FAIL("failed to serialize data: " << sys.render(err));

namespace {

    struct fixture : test_coordinator_fixture<>,
                     proxy_registry::backend,
                     basp::application::test_tag,
                     public packet_writer {
        using buffer_type = std::vector<byte>;

        fixture() : proxies(sys, *this), app(proxies) {
            REQUIRE_OK(app.init(*this));
            uri mars_uri;
            REQUIRE_OK(parse("tcp://mars", mars_uri));
            mars = make_node_id(mars_uri);
        }

        template<class... Ts>
        buffer_type to_buf(const Ts &... xs) {
            buffer_type buf;
            binary_serializer sink {system(), buf};
            REQUIRE_OK(sink(xs...));
            return buf;
        }

        template<class... Ts>
        void set_input(const Ts &... xs) {
            input = to_buf(xs...);
        }

        void handle_handshake() {
            BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_header);
            auto payload = to_buf(mars, defaults::middleman::app_identifiers);
            set_input(
                basp::header {basp::message_type::handshake, static_cast<uint32_t>(payload.size()), basp::version});
            REQUIRE_OK(app.handle_data(*this, input));
            BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_payload);
            REQUIRE_OK(app.handle_data(*this, payload));
        }

        void consume_handshake() {
            if (output.size() < basp::header_size)
                BOOST_FAIL("BASP application did not write a handshake header");
            auto hdr = basp::header::from_bytes(output);
            if (hdr.type != basp::message_type::handshake || hdr.payload_len == 0 ||
                hdr.operation_data != basp::version)
                BOOST_FAIL("invalid handshake header");
            node_id nid;
            std::vector<std::string> app_ids;
            binary_deserializer source {sys, output};
            source.skip(basp::header_size);
            if (auto err = source(nid, app_ids))
                BOOST_FAIL("unable to deserialize payload: " << sys.render(err));
            if (source.remaining() > 0)
                BOOST_FAIL("trailing bytes after reading payload");
            output.clear();
        }

        spawner &system() {
            return sys;
        }

        fixture &transport() {
            return *this;
        }

        endpoint_manager &manager() {
            BOOST_FAIL("unexpected function call");
        }

        buffer_type next_payload_buffer() override {
            return {};
        }

        buffer_type next_header_buffer() override {
            return {};
        }

        template<class... Ts>
        void configure_read(Ts...) {
            // nop
        }

        strong_actor_ptr make_proxy(node_id nid, actor_id aid) override {
            using impl_type = forwarding_actor_proxy;
            using hdl_type = strong_actor_ptr;
            actor_config cfg;
            return make_actor<impl_type, hdl_type>(aid, nid, &sys, cfg, self);
        }

        void set_last_hop(node_id *) override {
            // nop
        }

    protected:
        void write_impl(span<buffer_type *> buffers) override {
            for (auto buf : buffers)
                output.insert(output.end(), buf->begin(), buf->end());
        }

        buffer_type input;

        buffer_type output;

        node_id mars;

        proxy_registry proxies;

        basp::application app;
    };

}    // namespace

#define MOCK(kind, op, ...)                                                                      \
    do {                                                                                         \
        auto payload = to_buf(__VA_ARGS__);                                                      \
        set_input(basp::header {kind, static_cast<uint32_t>(payload.size()), op});               \
        if (auto err = app.handle_data(*this, input))                                            \
            BOOST_FAIL("application-under-test failed to process header: " << sys.render(err));  \
        if (auto err = app.handle_data(*this, payload))                                          \
            BOOST_FAIL("application-under-test failed to process payload: " << sys.render(err)); \
    } while (false)

#define RECEIVE(msg_type, op_data, ...)                                                                     \
    do {                                                                                                    \
        binary_deserializer source {sys, output};                                                           \
        basp::header hdr;                                                                                   \
        if (auto err = source(hdr, __VA_ARGS__))                                                            \
            BOOST_FAIL("failed to receive data: " << sys.render(err));                                      \
        if (source.remaining() != 0)                                                                        \
            BOOST_FAIL("unable to read entire message, " << source.remaining() << " bytes left in buffer"); \
        BOOST_CHECK_EQUAL(hdr.type, msg_type);                                                              \
        BOOST_CHECK_EQUAL(hdr.operation_data, op_data);                                                     \
        output.clear();                                                                                     \
    } while (false)

BOOST_FIXTURE_TEST_SUITE(application_tests, fixture)

BOOST_AUTO_TEST_CASE(missing_handshake) {
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_header);
    set_input(basp::header {basp::message_type::heartbeat, 0, 0});
    BOOST_CHECK_EQUAL(app.handle_data(*this, input), basp::ec::missing_handshake);
}

BOOST_AUTO_TEST_CASE(version_mismatch) {
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_header);
    set_input(basp::header {basp::message_type::handshake, 0, 0});
    BOOST_CHECK_EQUAL(app.handle_data(*this, input), basp::ec::version_mismatch);
}

BOOST_AUTO_TEST_CASE(missing_payload_in_handshake) {
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_header);
    set_input(basp::header {basp::message_type::handshake, 0, basp::version});
    BOOST_CHECK_EQUAL(app.handle_data(*this, input), basp::ec::missing_payload);
}

BOOST_AUTO_TEST_CASE(invalid_handshake) {
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_header);
    node_id no_nid;
    std::vector<std::string> no_ids;
    auto payload = to_buf(no_nid, no_ids);
    set_input(basp::header {basp::message_type::handshake, static_cast<uint32_t>(payload.size()), basp::version});
    REQUIRE_OK(app.handle_data(*this, input));
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_payload);
    BOOST_CHECK_EQUAL(app.handle_data(*this, payload), basp::ec::invalid_handshake);
}

BOOST_AUTO_TEST_CASE(app_identifier_mismatch) {
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_header);
    std::vector<std::string> wrong_ids {"YOLO!!!"};
    auto payload = to_buf(mars, wrong_ids);
    set_input(basp::header {basp::message_type::handshake, static_cast<uint32_t>(payload.size()), basp::version});
    REQUIRE_OK(app.handle_data(*this, input));
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_handshake_payload);
    BOOST_CHECK_EQUAL(app.handle_data(*this, payload), basp::ec::app_identifiers_mismatch);
}

BOOST_AUTO_TEST_CASE(repeated_handshake) {
    handle_handshake();
    consume_handshake();
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
    node_id no_nid;
    std::vector<std::string> no_ids;
    auto payload = to_buf(no_nid, no_ids);
    set_input(basp::header {basp::message_type::handshake, static_cast<uint32_t>(payload.size()), basp::version});
    BOOST_CHECK_EQUAL(app.handle_data(*this, input), none);
    BOOST_CHECK_EQUAL(app.handle_data(*this, payload), basp::ec::unexpected_handshake);
}

BOOST_AUTO_TEST_CASE(actor_message) {
    handle_handshake();
    consume_handshake();
    sys.registry().put(self->id(), self);
    BOOST_REQUIRE_EQUAL(self->mailbox().size(), 0u);
    MOCK(basp::message_type::actor_message, make_message_id().integer_value(), mars, actor_id {42}, self->id(),
         std::vector<strong_actor_ptr> {}, make_message("hello world!"));
    allow((monitor_atom, strong_actor_ptr), from(_).to(self).with(monitor_atom_v, _));
    expect((std::string), from(_).to(self).with("hello world!"));
}

BOOST_AUTO_TEST_CASE(resolve_request_without_result) {
    handle_handshake();
    consume_handshake();
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
    MOCK(basp::message_type::resolve_request, 42, std::string {"foo/bar"});
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
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
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
    MOCK(basp::message_type::resolve_request, 42, path);
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
    actor_id aid;
    std::set<std::string> ifs;
    RECEIVE(basp::message_type::resolve_response, 42u, aid, ifs);
    BOOST_CHECK_EQUAL(aid, self->id());
    BOOST_CHECK(ifs.empty());
}

BOOST_AUTO_TEST_CASE(resolve_request_on_name_with_result) {
    handle_handshake();
    consume_handshake();
    sys.registry().put("foo", self);
    std::string path = "name/foo";
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
    MOCK(basp::message_type::resolve_request, 42, path);
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
    actor_id aid;
    std::set<std::string> ifs;
    RECEIVE(basp::message_type::resolve_response, 42u, aid, ifs);
    BOOST_CHECK_EQUAL(aid, self->id());
    BOOST_CHECK(ifs.empty());
}

BOOST_AUTO_TEST_CASE(resolve_response_with_invalid_actor_handle) {
    handle_handshake();
    consume_handshake();
    app.resolve(*this, "foo/bar", self);
    std::string path;
    RECEIVE(basp::message_type::resolve_request, 1u, path);
    BOOST_CHECK_EQUAL(path, "foo/bar");
    actor_id aid = 0;
    std::set<std::string> ifs;
    MOCK(basp::message_type::resolve_response, 1u, aid, ifs);
    self->receive([&](strong_actor_ptr &hdl, std::set<std::string> &hdl_ifs) {
        BOOST_CHECK_EQUAL(hdl, nullptr);
        BOOST_CHECK_EQUAL(ifs, hdl_ifs);
    });
}

BOOST_AUTO_TEST_CASE(resolve_response_with_valid_actor_handle) {
    handle_handshake();
    consume_handshake();
    app.resolve(*this, "foo/bar", self);
    std::string path;
    RECEIVE(basp::message_type::resolve_request, 1u, path);
    BOOST_CHECK_EQUAL(path, "foo/bar");
    actor_id aid = 42;
    std::set<std::string> ifs;
    MOCK(basp::message_type::resolve_response, 1u, aid, ifs);
    self->receive([&](strong_actor_ptr &hdl, std::set<std::string> &hdl_ifs) {
        BOOST_REQUIRE(hdl != nullptr);
        BOOST_CHECK_EQUAL(ifs, hdl_ifs);
        BOOST_CHECK_EQUAL(hdl->id(), aid);
    });
}

BOOST_AUTO_TEST_CASE(heartbeat_message) {
    handle_handshake();
    consume_handshake();
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
    auto bytes = to_bytes(basp::header {basp::message_type::heartbeat, 0, 0});
    set_input(bytes);
    REQUIRE_OK(app.handle_data(*this, input));
    BOOST_CHECK_EQUAL(app.state(), basp::connection_state::await_header);
}

BOOST_AUTO_TEST_SUITE_END()
