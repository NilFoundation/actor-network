//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE transport_worker

#include <nil/actor/network/transport_worker.hpp>
#include <nil/actor/network/actor_proxy_impl.hpp>
#include <nil/actor/network/multiplexer.hpp>

#include <nil/actor/detail/scope_guard.hpp>

#include <nil/actor/test/dsl.hpp>

#include <nil/actor/binary_serializer.hpp>
#include <nil/actor/byte.hpp>
#include <nil/actor/ip_endpoint.hpp>
#include <nil/actor/make_actor.hpp>
#include <nil/actor/span.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace boost {
    namespace test_tools {
        namespace tt_detail {
            template<>
            struct print_log_value<actor> {
                void operator()(std::ostream &, actor const &) {
                }
            };
            template<>
            struct print_log_value<scoped_actor> {
                void operator()(std::ostream &, scoped_actor const &) {
                }
            };
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
            template<>
            struct print_log_value<ipv4_endpoint> {
                void operator()(std::ostream &, ipv4_endpoint const &) {
                }
            };
            template<>
            struct print_log_value<ipv6_endpoint> {
                void operator()(std::ostream &, ipv6_endpoint const &) {
                }
            };
        }    // namespace tt_detail
    }        // namespace test_tools
}    // namespace boost

namespace {

    constexpr string_view hello_test = "hello test!";

    struct application_result {
        bool initialized;
        byte_buffer data_buffer;
        std::string resolve_path;
        actor resolve_listener;
        std::string timeout_value;
        uint64_t timeout_id;
        sec err;
    };

    struct transport_result {
        std::vector<byte> packet_buffer;
        ip_endpoint ep;
    };

    class dummy_application {
    public:
        dummy_application(std::shared_ptr<application_result> res) :
            res_(std::move(res)) {
                // nop
            };

        ~dummy_application() = default;

        template<class Parent>
        error init(Parent &) {
            res_->initialized = true;
            return none;
        }

        template<class Parent>
        void write_message(Parent &parent, std::unique_ptr<endpoint_manager_queue::message> ptr) {
            auto payload_buf = parent.next_payload_buffer();
            binary_serializer sink(parent.system(), payload_buf);
            if (auto err = sink(ptr->msg->content()))
                BOOST_FAIL("serializing failed: " << err);
            BOOST_TEST_MESSAGE("before sending: " << ACTOR_ARG(ptr->msg->content()));
            parent.write_packet(payload_buf);
        }

        template<class Parent>
        error handle_data(Parent &, span<const byte> data) {
            auto &buf = res_->data_buffer;
            buf.clear();
            buf.insert(buf.begin(), data.begin(), data.end());
            return none;
        }

        template<class Parent>
        void resolve(Parent &, string_view path, const actor &listener) {
            res_->resolve_path.assign(path.begin(), path.end());
            res_->resolve_listener = listener;
        }

        template<class Parent>
        void timeout(Parent &, std::string value, uint64_t id) {
            res_->timeout_value = std::move(value);
            res_->timeout_id = id;
        }

        void handle_error(sec err) {
            res_->err = err;
        }

    private:
        std::shared_ptr<application_result> res_;
    };

    class dummy_transport {
    public:
        using transport_type = dummy_transport;

        using application_type = dummy_application;

        dummy_transport(spawner &sys, std::shared_ptr<transport_result> res) : sys_(sys), res_(std::move(res)) {
            // nop
        }

        void write_packet(ip_endpoint ep, span<byte_buffer *> buffers) {
            res_->ep = ep;
            auto &packet_buf = res_->packet_buffer;
            packet_buf.clear();
            for (auto *buf : buffers)
                packet_buf.insert(packet_buf.end(), buf->begin(), buf->end());
        }

        spawner &system() {
            return sys_;
        }

        transport_type &transport() {
            return *this;
        }

        byte_buffer next_header_buffer() {
            return {};
        }

        byte_buffer next_payload_buffer() {
            return {};
        }

    private:
        spawner &sys_;
        std::shared_ptr<transport_result> res_;
    };

    struct fixture : test_coordinator_fixture<>, host_fixture {
        using worker_type = transport_worker<dummy_application, ip_endpoint>;

        fixture() :
            transport_results {std::make_shared<transport_result>()},
            application_results {std::make_shared<application_result>()},
            transport(sys, transport_results), worker {dummy_application {application_results}} {
            mpx = std::make_shared<multiplexer>();
            if (auto err = mpx->init())
                BOOST_FAIL("mpx->init failed: " << sys.render(err));
            if (auto err = parse("[::1]:12345", ep))
                BOOST_FAIL("parse returned an error");
            worker = worker_type {dummy_application {application_results}, ep};
        }

        bool handle_io_event() override {
            return mpx->poll_once(false);
        }

        multiplexer_ptr mpx;
        std::shared_ptr<transport_result> transport_results;
        std::shared_ptr<application_result> application_results;
        dummy_transport transport;
        worker_type worker;
        ip_endpoint ep;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(endpoint_manager_tests, fixture)

BOOST_AUTO_TEST_CASE(construction_and_initialization) {
    BOOST_CHECK_EQUAL(worker.init(transport), none);
    BOOST_CHECK_EQUAL(application_results->initialized, true);
}

BOOST_AUTO_TEST_CASE(handle_data) {
    auto test_span = as_bytes(make_span(hello_test));
    worker.handle_data(transport, test_span);
    auto &buf = application_results->data_buffer;
    string_view result {reinterpret_cast<char *>(buf.data()), buf.size()};
    BOOST_CHECK_EQUAL(result, hello_test);
}

BOOST_AUTO_TEST_CASE(write_message) {
    std::string hello_test {"hello world!"};
    actor act;
    auto strong_actor = actor_cast<strong_actor_ptr>(act);
    mailbox_element::forwarding_stack stack;
    auto msg = make_message(hello_test);
    auto elem = make_mailbox_element(strong_actor, make_message_id(12345), stack, msg);
    using message_type = endpoint_manager_queue::message;
    auto message = detail::make_unique<message_type>(std::move(elem), nullptr);
    worker.write_message(transport, std::move(message));
    auto &buf = transport_results->packet_buffer;
    binary_deserializer source {sys, buf};
    nil::actor::message received_msg;
    BOOST_CHECK(!source(received_msg));
    BOOST_TEST_MESSAGE(ACTOR_ARG(received_msg));
    auto received_str = received_msg.get_as<std::string>(0);
    string_view result {received_str};
    BOOST_CHECK_EQUAL(result, hello_test);
    BOOST_CHECK_EQUAL(transport_results->ep, ep);
}

BOOST_AUTO_TEST_CASE(resolve) {
    worker.resolve(transport, "foo", self);
    BOOST_CHECK_EQUAL(application_results->resolve_path, "foo");
    BOOST_CHECK_EQUAL(application_results->resolve_listener, self);
}

BOOST_AUTO_TEST_CASE(timeout) {
    worker.timeout(transport, "bar", 42u);
    BOOST_CHECK_EQUAL(application_results->timeout_value, "bar");
    BOOST_CHECK_EQUAL(application_results->timeout_id, 42u);
}

BOOST_AUTO_TEST_CASE(handle_error) {
    worker.handle_error(sec::feature_disabled);
    BOOST_CHECK_EQUAL(application_results->err, sec::feature_disabled);
}

BOOST_AUTO_TEST_SUITE_END()
