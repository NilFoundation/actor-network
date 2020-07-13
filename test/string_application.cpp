//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE string_application

#include <nil/actor/network/stream_transport.hpp>

#include <nil/actor/test/dsl.hpp>

#include <vector>

#include <nil/actor/binary_deserializer.hpp>
#include <nil/actor/binary_serializer.hpp>
#include <nil/actor/byte.hpp>
#include <nil/actor/detail/scope_guard.hpp>
#include <nil/actor/make_actor.hpp>
#include <nil/actor/network/actor_proxy_impl.hpp>
#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/make_endpoint_manager.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/network/stream_socket.hpp>
#include <nil/actor/span.hpp>

using namespace nil::actor;
using namespace nil::actor::network;
using namespace nil::actor::policy;

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

    using buffer_type = std::vector<byte>;

    struct fixture : test_coordinator_fixture<>, host_fixture {
        fixture() {
            mpx = std::make_shared<multiplexer>();
            if (auto err = mpx->init())
                BOOST_FAIL("mpx->init failed: " << sys.render(err));
            mpx->set_thread_id();
        }

        bool handle_io_event() override {
            return mpx->poll_once(false);
        }

        multiplexer_ptr mpx;
    };

    struct string_application_header {
        uint32_t payload;
    };

    /// @relates header
    template<class Inspector>
    typename Inspector::result_type inspect(Inspector &f, string_application_header &hdr) {
        return f(meta::type_name("sa_header"), hdr.payload);
    }

    class string_application {
    public:
        using header_type = string_application_header;

        string_application(std::shared_ptr<std::vector<byte>> buf) : buf_(std::move(buf)) {
            // nop
        }

        template<class Parent>
        error init(Parent &) {
            return none;
        }

        template<class Parent>
        void handle_packet(Parent &parent, header_type &, span<const byte> payload) {
            binary_deserializer source {parent.system(), payload};
            message msg;
            if (auto err = msg.load(source))
                BOOST_FAIL("unable to deserialize message");
            if (!msg.match_elements<std::string>())
                BOOST_FAIL("unexpected message");
            auto &str = msg.get_as<std::string>(0);
            auto bytes = as_bytes(make_span(str));
            buf_->insert(buf_->end(), bytes.begin(), bytes.end());
        }

        template<class Parent>
        void write_message(Parent &parent, std::unique_ptr<endpoint_manager_queue::message> ptr) {
            // Ignore proxy announcement messages.
            if (ptr->msg == nullptr)
                return;
            auto header_buf = parent.next_header_buffer();
            auto payload_buf = parent.next_payload_buffer();
            binary_serializer payload_sink {parent.system(), payload_buf};
            if (auto err = payload_sink(ptr->msg->payload))
                BOOST_FAIL("serializing failed");
            binary_serializer header_sink {parent.system(), header_buf};
            if (auto err = header_sink(header_type {static_cast<uint32_t>(payload_buf.size())}))
                BOOST_FAIL("serializing failed: " << err);
            parent.write_packet(header_buf, payload_buf);
        }

        static expected<std::vector<byte>> serialize(spawner &sys, const message &x) {
            std::vector<byte> result;
            binary_serializer sink {sys, result};
            if (auto err = x.save(sink))
                return err.value();
            return result;
        }

    private:
        std::shared_ptr<std::vector<byte>> buf_;
    };    // namespace

    template<class Base, class Subtype>
    class stream_string_application : public Base {
    public:
        using header_type = typename Base::header_type;

        stream_string_application(std::shared_ptr<std::vector<byte>> buf) :
            Base(std::move(buf)), await_payload_(false) {
            // nop
        }

        template<class Parent>
        error init(Parent &parent) {
            parent.transport().configure_read(network::receive_policy::exactly(sizeof(header_type)));
            return Base::init(parent);
        }

        template<class Parent>
        error handle_data(Parent &parent, span<const byte> data) {
            if (await_payload_) {
                Base::handle_packet(parent, header_, data);
                await_payload_ = false;
            } else {
                if (data.size() != sizeof(header_type))
                    BOOST_FAIL("");
                binary_deserializer source {nullptr, data};
                if (auto err = source(header_))
                    BOOST_FAIL("serializing failed");
                if (header_.payload == 0)
                    Base::handle_packet(parent, header_, span<const byte> {});
                else
                    parent.transport().configure_read(network::receive_policy::exactly(header_.payload));
                await_payload_ = true;
            }
            return none;
        }

        template<class Parent>
        void resolve(Parent &parent, string_view path, actor listener) {
            actor_id aid = 42;
            auto hid = string_view("0011223344556677889900112233445566778899");
            auto nid = unbox(make_node_id(aid, hid));
            actor_config cfg;
            auto sys = &parent.system();
            auto mgr = &parent.manager();
            auto p = make_actor<network::actor_proxy_impl, strong_actor_ptr>(aid, nid, sys, cfg, mgr);
            anon_send(listener, resolve_atom_v, std::string {path.begin(), path.end()}, p);
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

        void handle_error(sec sec) {
            BOOST_FAIL("handle_error called");
        }

    private:
        header_type header_;
        bool await_payload_;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(endpoint_manager_tests, fixture)

BOOST_AUTO_TEST_CASE(receive) {
    using application_type = extend<string_application>::with<stream_string_application>;
    using transport_type = stream_transport<application_type>;
    std::vector<byte> read_buf(1024);
    BOOST_CHECK_EQUAL(mpx->num_socket_managers(), 1u);
    auto buf = std::make_shared<std::vector<byte>>();
    auto sockets = unbox(make_stream_socket_pair());
    nonblocking(sockets.second, true);
    BOOST_CHECK_EQUAL(get<sec>(read(sockets.second, read_buf)), sec::unavailable_or_would_block);
    BOOST_TEST_MESSAGE("adding both endpoint managers");
    auto mgr1 = make_endpoint_manager(mpx, sys, transport_type {sockets.first, application_type {sys, buf}});
    BOOST_CHECK_EQUAL(mgr1->init(), none);
    BOOST_CHECK_EQUAL(mpx->num_socket_managers(), 2u);
    auto mgr2 = make_endpoint_manager(mpx, sys, transport_type {sockets.second, application_type {sys, buf}});
    BOOST_CHECK_EQUAL(mgr2->init(), none);
    BOOST_CHECK_EQUAL(mpx->num_socket_managers(), 3u);
    BOOST_TEST_MESSAGE("resolve actor-proxy");
    mgr1->resolve(unbox(make_uri("test:/id/42")), self);
    run();
    self->receive(
        [&](resolve_atom, const std::string &, const strong_actor_ptr &p) {
            BOOST_TEST_MESSAGE("got a proxy, send a message to it");
            self->send(actor_cast<actor>(p), "hello proxy!");
        },
        after(std::chrono::seconds(0)) >> [&] { BOOST_FAIL("manager did not respond with a proxy."); });
    run();
    BOOST_CHECK_EQUAL(string_view(reinterpret_cast<char *>(buf->data()), buf->size()), "hello proxy!");
}

BOOST_AUTO_TEST_SUITE_END()
