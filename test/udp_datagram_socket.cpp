//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE udp_datagram_socket

#include <nil/actor/network/udp_datagram_socket.hpp>
#include <nil/actor/network/ip.hpp>

#include <nil/actor/test/dsl.hpp>

#include <nil/actor/detail/net_syscall.hpp>
#include <nil/actor/detail/socket_sys_includes.hpp>

#include <nil/actor/binary_serializer.hpp>
#include <nil/actor/ip_address.hpp>
#include <nil/actor/ip_endpoint.hpp>

using namespace nil::actor;
using namespace nil::actor::network;
using namespace nil::actor::network::ip;

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

    constexpr string_view hello_test = "Hello test!";

    struct fixture : host_fixture {
        fixture() : host_fixture(), buf(1024) {
            addresses = local_addresses("localhost");
            BOOST_CHECK(!addresses.empty());
            ep = ip_endpoint(*addresses.begin(), 0);
            auto send_pair = unbox(make_udp_datagram_socket(ep));
            send_socket = send_pair.first;
            auto receive_pair = unbox(make_udp_datagram_socket(ep));
            receive_socket = receive_pair.first;
            ep.port(receive_pair.second);
        }

        ~fixture() {
            close(send_socket);
            close(receive_socket);
        }

        std::vector<ip_address> addresses;
        spawner_config cfg;
        spawner sys {cfg};
        ip_endpoint ep;
        udp_datagram_socket send_socket;
        udp_datagram_socket receive_socket;
        std::vector<byte> buf;
    };

    error read_from_socket(udp_datagram_socket sock, std::vector<byte> &buf) {
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

    struct header {
        header(size_t payload_size) : payload_size(payload_size) {
            // nop
        }

        header() : header(0) {
            // nop
        }

        template<class Inspector>
        friend typename Inspector::result_type inspect(Inspector &f, header &x) {
            return f(meta::type_name("header"), x.payload_size);
        }

        size_t payload_size;
    };

}    // namespace

BOOST_FIXTURE_TEST_SUITE(udp_datagram_socket_test, fixture)

BOOST_AUTO_TEST_CASE(socket_creation) {
    ip_endpoint ep;
    BOOST_CHECK_EQUAL(parse("0.0.0.0:0", ep), none);
    auto ret = make_udp_datagram_socket(ep);
    if (!ret)
        BOOST_FAIL("socket creation failed: " << ret.error());
    BOOST_CHECK_EQUAL(local_port(ret->first), ret->second);
}

BOOST_AUTO_TEST_CASE(read_write_using_span_byte) {
    if (auto err = nonblocking(socket_cast<network::socket>(receive_socket), true))
        BOOST_FAIL("setting socket to nonblocking failed");
    BOOST_CHECK_EQUAL(get<sec>(read(receive_socket, buf)), sec::unavailable_or_would_block);
    BOOST_TEST_MESSAGE("sending data to " << to_string(ep));
    BOOST_CHECK_EQUAL(get<unsigned long>(write(send_socket, as_bytes(make_span(hello_test)), ep)), hello_test.size());
    BOOST_CHECK_EQUAL(read_from_socket(receive_socket, buf), none);
    string_view received {reinterpret_cast<const char *>(buf.data()), buf.size()};
    BOOST_CHECK_EQUAL(received, hello_test);
}

BOOST_AUTO_TEST_CASE(read_write_using_span_std_vector_byte_ptr) {
    // generate header and payload in separate buffers
    header hdr {hello_test.size()};
    std::vector<byte> hdr_buf;
    binary_serializer sink(sys, hdr_buf);
    if (auto err = sink(hdr))
        BOOST_FAIL("serializing payload failed" << sys.render(err));
    auto bytes = as_bytes(make_span(hello_test));
    std::vector<byte> payload_buf(bytes.begin(), bytes.end());
    auto packet_size = hdr_buf.size() + payload_buf.size();
    std::vector<std::vector<byte> *> bufs {&hdr_buf, &payload_buf};
    BOOST_CHECK_EQUAL(get<unsigned long>(write(send_socket, bufs, ep)), packet_size);
    // receive both as one single packet.
    buf.resize(packet_size);
    BOOST_CHECK_EQUAL(read_from_socket(receive_socket, buf), none);
    BOOST_CHECK_EQUAL(buf.size(), packet_size);
    binary_deserializer source(nullptr, buf);
    header recv_hdr;
    if (auto err = source(recv_hdr))
        BOOST_FAIL("serializing failed");
    BOOST_CHECK_EQUAL(hdr.payload_size, recv_hdr.payload_size);
    string_view received {reinterpret_cast<const char *>(buf.data()) + sizeof(header), buf.size() - sizeof(header)};
    BOOST_CHECK_EQUAL(received, hello_test);
}

BOOST_AUTO_TEST_SUITE_END()
