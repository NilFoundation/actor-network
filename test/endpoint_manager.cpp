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

#define BOOST_TEST_MODULE endpoint_manager

#include <nil/actor/network/endpoint_manager.hpp>

#include <nil/actor/test/host_fixture.hpp>
#include <nil/actor/test/dsl.hpp>

#include <nil/actor/serialization/binary_serializer.hpp>
#include <nil/actor/byte.hpp>
#include <nil/actor/detail/scope_guard.hpp>
#include <nil/actor/make_actor.hpp>
#include <nil/actor/network/actor_proxy_impl.hpp>
#include <nil/actor/network/make_endpoint_manager.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/network/stream_socket.hpp>
#include <nil/actor/span.hpp>

using namespace nil::actor;
using namespace nil::actor::network;

namespace {

    string_view hello_manager {"hello manager!"};

    string_view hello_test {"hello test!"};

    struct fixture : test_coordinator_fixture<>, host_fixture {
        fixture() {
            mpx = std::make_shared<multiplexer>();
            mpx->set_thread_id();
            if (auto err = mpx->init())
                BOOST_FAIL("mpx->init failed: " << sys.render(err));
            if (mpx->num_socket_managers() != 1)
                BOOST_FAIL("mpx->num_socket_managers() != 1");
        }

        bool handle_io_event() override {
            return mpx->poll_once(false);
        }

        multiplexer_ptr mpx;
    };

    class dummy_application {
    public:
        static expected<std::vector<byte>> serialize(spawner &sys, const type_erased_tuple &x) {
            std::vector<byte> result;
            binary_serializer sink {sys, result};
            if (auto err = message::save(sink, x))
                return err.value();
            return result;
        }
    };

    class dummy_transport {
    public:
        using application_type = dummy_application;

        dummy_transport(stream_socket handle, std::shared_ptr<std::vector<byte>> data) :
            handle_(handle), data_(data), read_buf_(1024) {
            // nop
        }

        stream_socket handle() {
            return handle_;
        }

        template<class Manager>
        error init(Manager &manager) {
            auto test_bytes = as_bytes(make_span(hello_test));
            buf_.insert(buf_.end(), test_bytes.begin(), test_bytes.end());
            manager.register_writing();
            return none;
        }

        template<class Manager>
        bool handle_read_event(Manager &) {
            auto res = read(handle_, read_buf_);
            if (auto num_bytes = get_if<size_t>(&res)) {
                data_->insert(data_->end(), read_buf_.begin(), read_buf_.begin() + *num_bytes);
                return true;
            }
            return get<sec>(res) == sec::unavailable_or_would_block;
        }

        template<class Manager>
        bool handle_write_event(Manager &mgr) {
            for (auto x = mgr.next_message(); x != nullptr; x = mgr.next_message()) {
                auto &payload = x->payload;
                buf_.insert(buf_.end(), payload.begin(), payload.end());
            }
            auto res = write(handle_, buf_);
            if (auto num_bytes = get_if<size_t>(&res)) {
                buf_.erase(buf_.begin(), buf_.begin() + *num_bytes);
                return buf_.size() > 0;
            }
            return get<sec>(res) == sec::unavailable_or_would_block;
        }

        void handle_error(sec) {
            // nop
        }

        template<class Manager>
        void resolve(Manager &mgr, const uri &locator, const actor &listener) {
            actor_id aid = 42;
            auto hid = "0011223344556677889900112233445566778899";
            auto nid = unbox(make_node_id(42, hid));
            actor_config cfg;
            auto p = make_actor<actor_proxy_impl, strong_actor_ptr>(aid, nid, &mgr.system(), cfg, &mgr);
            std::string path {locator.path().begin(), locator.path().end()};
            anon_send(listener, resolve_atom_v, std::move(path), p);
        }

        template<class Manager>
        void timeout(Manager &, const std::string &, uint64_t) {
            // nop
        }

        template<class Parent>
        void new_proxy(Parent &, const node_id &, actor_id) {
            // nop
        }

        template<class Parent>
        void local_actor_down(Parent &, const node_id &, actor_id, error) {
            // nop
        }

    private:
        stream_socket handle_;

        std::shared_ptr<std::vector<byte>> data_;

        std::vector<byte> read_buf_;

        std::vector<byte> buf_;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(endpoint_manager_tests, fixture)

BOOST_AUTO_TEST_CASE(send and receive) {
    std::vector<byte> read_buf(1024);
    auto buf = std::make_shared<std::vector<byte>>();
    auto sockets = unbox(make_stream_socket_pair());
    nonblocking(sockets.second, true);
    BOOST_CHECK_EQUAL(read(sockets.second, read_buf), sec::unavailable_or_would_block);
    auto guard = detail::make_scope_guard([&] { close(sockets.second); });
    auto mgr = make_endpoint_manager(mpx, sys, dummy_transport {sockets.first, buf});
    BOOST_CHECK_EQUAL(mgr->mask(), operation::none);
    BOOST_CHECK_EQUAL(mgr->init(), none);
    BOOST_CHECK_EQUAL(mgr->mask(), operation::read_write);
    BOOST_CHECK_EQUAL(mpx->num_socket_managers(), 2u);
    BOOST_CHECK_EQUAL(write(sockets.second, as_bytes(make_span(hello_manager))), hello_manager.size());
    run();
    BOOST_CHECK_EQUAL(string_view(reinterpret_cast<char *>(buf->data()), buf->size()), hello_manager);
    BOOST_CHECK_EQUAL(read(sockets.second, read_buf), hello_test.size());
    BOOST_CHECK_EQUAL(string_view(reinterpret_cast<char *>(read_buf.data()), hello_test.size()), hello_test);
}

BOOST_AUTO_TEST_CASE(resolve and proxy communication) {
    std::vector<byte> read_buf(1024);
    auto buf = std::make_shared<std::vector<byte>>();
    auto sockets = unbox(make_stream_socket_pair());
    nonblocking(sockets.second, true);
    auto guard = detail::make_scope_guard([&] { close(sockets.second); });
    auto mgr = make_endpoint_manager(mpx, sys, dummy_transport {sockets.first, buf});
    BOOST_CHECK_EQUAL(mgr->init(), none);
    BOOST_CHECK_EQUAL(mgr->mask(), operation::read_write);
    run();
    BOOST_CHECK_EQUAL(read(sockets.second, read_buf), hello_test.size());
    mgr->resolve(unbox(make_uri("test:id/42")), self);
    run();
    self->receive(
        [&](resolve_atom, const std::string &, const strong_actor_ptr &p) {
            BOOST_TEST_MESSAGE("got a proxy, send a message to it");
            self->send(actor_cast<actor>(p), "hello proxy!");
        },
        after(std::chrono::seconds(0)) >> [&] { BOOST_FAIL("manager did not respond with a proxy."); });
    run();
    auto read_res = read(sockets.second, read_buf);
    if (!holds_alternative<size_t>(read_res)) {
        ACTOR_ERROR("read() returned an error: " << sys.render(get<sec>(read_res)));
        return;
    }
    read_buf.resize(get<size_t>(read_res));
    BOOST_TEST_MESSAGE("receive buffer contains " << read_buf.size() << " bytes");
    message msg;
    binary_deserializer source {sys, read_buf};
    BOOST_CHECK_EQUAL(source(msg), none);
    if (msg.match_elements<std::string>())
        BOOST_CHECK_EQUAL(msg.get_as<std::string>(0), "hello proxy!");
    else
        ACTOR_ERROR("expected a string, got: " << to_string(msg));
}

BOOST_AUTO_TEST_SUITE_END()