//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/tcp_stream_socket.hpp>

#include <nil/actor/detail/net_syscall.hpp>
#include <nil/actor/detail/sockaddr_members.hpp>
#include <nil/actor/detail/socket_sys_includes.hpp>
#include <nil/actor/expected.hpp>
#include <nil/actor/ipv4_address.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/network/ip.hpp>
#include <nil/actor/network/socket_guard.hpp>
#include <nil/actor/sec.hpp>
#include <nil/actor/variant.hpp>

namespace nil {
    namespace actor {
        namespace network {

            namespace {

                template<int Family>
                bool ip_connect(stream_socket fd, std::string host, uint16_t port) {
                    ACTOR_LOG_TRACE("Family =" << (Family == AF_INET ? "AF_INET" : "AF_INET6") << ACTOR_ARG(fd.id)
                                             << ACTOR_ARG(host) << ACTOR_ARG(port));
                    static_assert(Family == AF_INET || Family == AF_INET6, "invalid family");
                    using sockaddr_type = typename std::conditional<Family == AF_INET, sockaddr_in, sockaddr_in6>::type;
                    sockaddr_type sa;
                    memset(&sa, 0, sizeof(sockaddr_type));
                    inet_pton(Family, host.c_str(), &detail::addr_of(sa));
                    detail::family_of(sa) = Family;
                    detail::port_of(sa) = htons(port);
                    using sa_ptr = const sockaddr *;
                    return ::connect(fd.id, reinterpret_cast<sa_ptr>(&sa), sizeof(sa)) == 0;
                }

            }    // namespace

            expected<tcp_stream_socket> make_connected_tcp_stream_socket(ip_endpoint node) {
                ACTOR_LOG_DEBUG("tcp connect to: " << to_string(node));
                auto proto = node.address().embeds_v4() ? AF_INET : AF_INET6;
                int socktype = SOCK_STREAM;
#ifdef SOCK_CLOEXEC
                socktype |= SOCK_CLOEXEC;
#endif
                ACTOR_NET_SYSCALL("socket", fd, ==, -1, ::socket(proto, socktype, 0));
                tcp_stream_socket sock {fd};
                child_process_inherit(sock, false);
                auto sguard = make_socket_guard(sock);
                if (proto == AF_INET6) {
                    if (ip_connect<AF_INET6>(sock, to_string(node.address()), node.port())) {
                        ACTOR_LOG_INFO("successfully connected to (IPv6):" << to_string(node));
                        return sguard.release();
                    }
                } else if (ip_connect<AF_INET>(sock, to_string(node.address().embedded_v4()), node.port())) {
                    ACTOR_LOG_INFO("successfully connected to (IPv4):" << to_string(node));
                    return sguard.release();
                }
                ACTOR_LOG_WARNING("could not connect to: " << to_string(node));
                return make_error(sec::cannot_connect_to_node);
            }

            expected<tcp_stream_socket> make_connected_tcp_stream_socket(const uri::authority_type &node) {
                auto port = node.port;
                if (port == 0)
                    return make_error(sec::cannot_connect_to_node, "port is zero");
                std::vector<ip_address> addrs;
                if (auto str = get_if<std::string>(&node.host))
                    addrs = ip::resolve(*str);
                else if (auto addr = get_if<ip_address>(&node.host))
                    addrs.push_back(*addr);
                if (addrs.empty())
                    return make_error(sec::cannot_connect_to_node, "empty authority");
                for (auto &addr : addrs) {
                    if (auto sock = make_connected_tcp_stream_socket(ip_endpoint {addr, port}))
                        return *sock;
                }
                return make_error(sec::cannot_connect_to_node, to_string(node));
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil