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

#define BOOST_TEST_MODULE transport_worker_dispatcher

#include <nil/actor/network/transport_worker_dispatcher.hpp>

#include <nil/actor/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

#include <nil/actor/ip_endpoint.hpp>
#include <nil/actor/make_actor.hpp>
#include <nil/actor/monitorable_actor.hpp>
#include <nil/actor/node_id.hpp>
#include <nil/actor/uri.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace {

    using buffer_type = std::vector<byte>;

    constexpr string_view hello_test = "hello_test";

    struct dummy_actor : public monitorable_actor {
        dummy_actor(actor_config &cfg) : monitorable_actor(cfg) {
            // nop
        }

        void enqueue(mailbox_element_ptr, execution_unit *) override {
            // nop
        }
    };

    class dummy_application {
    public:
        dummy_application(std::shared_ptr<buffer_type> rec_buf, uint8_t id) :
            rec_buf_(std::move(rec_buf)), id_(id) {
                                              // nop
                                          };

        ~dummy_application() = default;

        template<class Parent>
        error init(Parent &) {
            rec_buf_->push_back(static_cast<byte>(id_));
            return none;
        }

        template<class Parent>
        void write_message(Parent &parent, std::unique_ptr<endpoint_manager_queue::message> msg) {
            rec_buf_->push_back(static_cast<byte>(id_));
            auto header_buf = parent.next_header_buffer();
            parent.write_packet(header_buf, msg->payload);
        }

        template<class Parent>
        error handle_data(Parent &, span<const byte>) {
            rec_buf_->push_back(static_cast<byte>(id_));
            return none;
        }

        template<class Manager>
        void resolve(Manager &, const std::string &, actor) {
            rec_buf_->push_back(static_cast<byte>(id_));
        }

        template<class Transport>
        void timeout(Transport &, const std::string &, uint64_t) {
            rec_buf_->push_back(static_cast<byte>(id_));
        }

        void handle_error(sec) {
            rec_buf_->push_back(static_cast<byte>(id_));
        }

        static expected<buffer_type> serialize(spawner &, const type_erased_tuple &) {
            return buffer_type {};
        }

    private:
        std::shared_ptr<buffer_type> rec_buf_;
        uint8_t id_;
    };

    struct dummy_application_factory {
    public:
        using application_type = dummy_application;

        dummy_application_factory(std::shared_ptr<buffer_type> buf) : buf_(std::move(buf)), application_cnt_(0) {
            // nop
        }

        dummy_application make() {
            return dummy_application {buf_, application_cnt_++};
        }

    private:
        std::shared_ptr<buffer_type> buf_;
        uint8_t application_cnt_;
    };

    struct dummy_transport {
        using transport_type = dummy_transport;

        using factory_type = dummy_application_factory;

        using application_type = dummy_application;

        dummy_transport(std::shared_ptr<buffer_type> buf) : buf_(std::move(buf)) {
            // nop
        }

        template<class IdType>
        void write_packet(IdType, span<buffer_type *> buffers) {
            for (auto buf : buffers)
                buf_->insert(buf_->end(), buf->begin(), buf->end());
        }

        transport_type &transport() {
            return *this;
        }

        buffer_type next_header_buffer() {
            return {};
        }

        buffer_type next_payload_buffer() {
            return {};
        }

    private:
        std::shared_ptr<buffer_type> buf_;
    };

    struct testdata {
        testdata(uint8_t worker_id, node_id id, ip_endpoint ep) : worker_id(worker_id), nid(std::move(id)), ep(ep) {
            // nop
        }

        uint8_t worker_id;
        node_id nid;
        ip_endpoint ep;
    };

    // TODO: switch to std::operator""s when switching to C++14
    ip_endpoint operator"" _ep(const char *cstr, size_t cstr_len) {
        ip_endpoint ep;
        string_view str(cstr, cstr_len);
        if (auto err = parse(str, ep))
            BOOST_FAIL("parse returned error: " << err);
        return ep;
    }

    uri operator"" _u(const char *cstr, size_t cstr_len) {
        uri result;
        string_view str {cstr, cstr_len};
        auto err = parse(str, result);
        if (err)
            BOOST_FAIL("error while parsing " << str << ": " << to_string(err));
        return result;
    }

    struct fixture : host_fixture {
        using dispatcher_type = transport_worker_dispatcher<dummy_application_factory, ip_endpoint>;

        fixture() : buf {std::make_shared<buffer_type>()}, dispatcher {dummy_application_factory {buf}}, dummy {buf} {
            add_new_workers();
        }

        std::unique_ptr<network::endpoint_manager_queue::message> make_dummy_message(node_id nid) {
            actor_id aid = 42;
            actor_config cfg;
            auto p = make_actor<dummy_actor, strong_actor_ptr>(aid, nid, &sys, cfg);
            auto test_span = as_bytes(make_span(hello_test));
            buffer_type payload(test_span.begin(), test_span.end());
            auto receiver = actor_cast<strong_actor_ptr>(p);
            if (!receiver)
                BOOST_FAIL("failed to cast receiver to a strong_actor_ptr");
            mailbox_element::forwarding_stack stack;
            auto elem = make_mailbox_element(nullptr, make_message_id(12345), std::move(stack), make_message());
            return detail::make_unique<endpoint_manager_queue::message>(std::move(elem), receiver, payload);
        }

        bool contains(byte x) {
            return std::count(buf->begin(), buf->end(), x) > 0;
        }

        void add_new_workers() {
            for (auto &data : test_data) {
                auto worker = dispatcher.add_new_worker(dummy, data.nid, data.ep);
                if (!worker)
                    BOOST_FAIL("add_new_worker returned an error: " << worker.error());
            }
            buf->clear();
        }

        void test_write_message(testdata &testcase) {
            auto msg = make_dummy_message(testcase.nid);
            if (!msg->receiver)
                BOOST_FAIL("receiver is null");
            dispatcher.write_message(dummy, std::move(msg));
        }

        spawner_config cfg {};
        spawner sys {cfg};

        std::shared_ptr<buffer_type> buf;
        dispatcher_type dispatcher;
        dummy_transport dummy;

        std::vector<testdata> test_data {
            {0, make_node_id("http:file"_u), "[::1]:1"_ep},
            {1, make_node_id("http:file?a=1&b=2"_u), "[fe80::2:34]:12345"_ep},
            {2, make_node_id("http:file#42"_u), "[1234::17]:4444"_ep},
            {3, make_node_id("http:file?a=1&b=2#42"_u), "[2332::1]:12"_ep},
        };
    };

#define CHECK_HANDLE_DATA(testcase)                                       \
    dispatcher.handle_data(dummy, span<const byte> {}, testcase.ep);      \
    BOOST_CHECK_EQUAL(buf->size(), 1u);                                   \
    BOOST_CHECK_EQUAL(static_cast<byte>(testcase.worker_id), buf->at(0)); \
    buf->clear();

#define CHECK_WRITE_MESSAGE(testcase)                                                    \
    test_write_message(testcase);                                                        \
    BOOST_CHECK_EQUAL(buf->size(), hello_test.size() + 1u);                              \
    BOOST_CHECK_EQUAL(static_cast<byte>(testcase.worker_id), buf->at(0));                \
    BOOST_CHECK_EQUAL(memcmp(buf->data() + 1, hello_test.data(), hello_test.size()), 0); \
    buf->clear();

#define CHECK_TIMEOUT(testcase)                                           \
    dispatcher.set_timeout(1u, testcase.ep);                              \
    dispatcher.timeout(dummy, "dummy", 1u);                               \
    BOOST_CHECK_EQUAL(buf->size(), 1u);                                   \
    BOOST_CHECK_EQUAL(static_cast<byte>(testcase.worker_id), buf->at(0)); \
    buf->clear();

}    // namespace

BOOST_FIXTURE_TEST_SUITE(transport_worker_dispatcher_test, fixture)

BOOST_AUTO_TEST_CASE(init) {
    dispatcher_type dispatcher {dummy_application_factory {buf}};
    BOOST_CHECK_EQUAL(dispatcher.init(dummy), none);
}

BOOST_AUTO_TEST_CASE(handle_data) {
    CHECK_HANDLE_DATA(test_data.at(0));
    CHECK_HANDLE_DATA(test_data.at(1));
    CHECK_HANDLE_DATA(test_data.at(2));
    CHECK_HANDLE_DATA(test_data.at(3));
}

BOOST_AUTO_TEST_CASE(write_message write_packet) {
    CHECK_WRITE_MESSAGE(test_data.at(0));
    CHECK_WRITE_MESSAGE(test_data.at(1));
    CHECK_WRITE_MESSAGE(test_data.at(2));
    CHECK_WRITE_MESSAGE(test_data.at(3));
}

BOOST_AUTO_TEST_CASE(resolve) {
    // TODO think of a test for this
}

BOOST_AUTO_TEST_CASE(timeout) {
    CHECK_TIMEOUT(test_data.at(0));
    CHECK_TIMEOUT(test_data.at(1));
    CHECK_TIMEOUT(test_data.at(2));
    CHECK_TIMEOUT(test_data.at(3));
}

BOOST_AUTO_TEST_CASE(handle_error) {
    dispatcher.handle_error(sec::unavailable_or_would_block);
    BOOST_CHECK_EQUAL(buf->size(), 4u);
    BOOST_CHECK(contains(byte(0)));
    BOOST_CHECK(contains(byte(1)));
    BOOST_CHECK(contains(byte(2)));
    BOOST_CHECK(contains(byte(3)));
}

BOOST_AUTO_TEST_SUITE_END()
