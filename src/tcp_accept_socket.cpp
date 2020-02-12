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

#include <nil/actor/network/tcp_accept_socket.hpp>

#include <nil/actor/detail/net_syscall.hpp>
#include <nil/actor/detail/sockaddr_members.hpp>
#include <nil/actor/detail/socket_sys_aliases.hpp>
#include <nil/actor/detail/socket_sys_includes.hpp>
#include <nil/actor/expected.hpp>
#include <nil/actor/ip_address.hpp>
#include <nil/actor/ipv4_address.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/network/ip.hpp>
#include <nil/actor/network/socket_guard.hpp>
#include <nil/actor/network/tcp_stream_socket.hpp>
#include <nil/actor/sec.hpp>

namespace nil {
    namespace actor {
        namespace network {

            namespace {

                error set_inaddr_any(socket, sockaddr_in &sa) {
                    sa.sin_addr.s_addr = INADDR_ANY;
                    return none;
                }

                error set_inaddr_any(socket x, sockaddr_in6 &sa) {
                    sa.sin6_addr = in6addr_any;
                    // Also accept ipv4 connections on this socket.
                    int off = 0;
                    ACTOR_NET_SYSCALL("setsockopt", res, !=, 0,
                                    setsockopt(x.id, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<setsockopt_ptr>(&off),
                                               static_cast<socket_size_type>(sizeof(off))));
                    return none;
                }

                template<int Family>
                expected<tcp_accept_socket> new_tcp_acceptor_impl(uint16_t port, const char *addr, bool reuse_addr,
                                                                  bool any) {
                    static_assert(Family == AF_INET || Family == AF_INET6, "invalid family");
                    ACTOR_LOG_TRACE(ACTOR_ARG(port) << ", addr = " << (addr ? addr : "nullptr"));
                    int socktype = SOCK_STREAM;
#ifdef SOCK_CLOEXEC
                    socktype |= SOCK_CLOEXEC;
#endif
                    ACTOR_NET_SYSCALL("socket", fd, ==, -1, ::socket(Family, socktype, 0));
                    tcp_accept_socket sock {fd};
                    // sguard closes the socket in case of exception
                    auto sguard = make_socket_guard(tcp_accept_socket {fd});
                    child_process_inherit(sock, false);
                    if (reuse_addr) {
                        int on = 1;
                        ACTOR_NET_SYSCALL("setsockopt", tmp1, !=, 0,
                                        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<setsockopt_ptr>(&on),
                                                   static_cast<socket_size_type>(sizeof(on))));
                    }
                    using sockaddr_type = typename std::conditional<Family == AF_INET, sockaddr_in, sockaddr_in6>::type;
                    sockaddr_type sa;
                    memset(&sa, 0, sizeof(sockaddr_type));
                    detail::family_of(sa) = Family;
                    if (any)
                        if (auto err = set_inaddr_any(sock, sa))
                            return err;
                    ACTOR_NET_SYSCALL("inet_pton", tmp, !=, 1, inet_pton(Family, addr, &detail::addr_of(sa)));
                    detail::port_of(sa) = htons(port);
                    ACTOR_NET_SYSCALL(
                        "bind", res, !=, 0,
                        bind(fd, reinterpret_cast<sockaddr *>(&sa), static_cast<socket_size_type>(sizeof(sa))));
                    return sguard.release();
                }

            }    // namespace

            expected<tcp_accept_socket> make_tcp_accept_socket(ip_endpoint node, bool reuse_addr) {
                ACTOR_LOG_TRACE(ACTOR_ARG(node));
                auto addr = to_string(node.address());
                auto make_acceptor =
                    node.address().embeds_v4() ? new_tcp_acceptor_impl<AF_INET> : new_tcp_acceptor_impl<AF_INET6>;
                auto p = make_acceptor(node.port(), addr.c_str(), reuse_addr, node.address().zero());
                if (!p) {
                    ACTOR_LOG_WARNING("could not create tcp socket for: " << to_string(node));
                    return make_error(sec::cannot_open_port, "tcp socket creation failed", to_string(node));
                }
                auto sock = socket_cast<tcp_accept_socket>(*p);
                auto sguard = make_socket_guard(sock);
                ACTOR_NET_SYSCALL("listen", tmp, !=, 0, listen(sock.id, SOMAXCONN));
                ACTOR_LOG_DEBUG(ACTOR_ARG(sock.id));
                return sguard.release();
            }

            expected<tcp_accept_socket> make_tcp_accept_socket(const uri::authority_type &node, bool reuse_addr) {
                if (auto ip = get_if<ip_address>(&node.host))
                    return make_tcp_accept_socket(ip_endpoint {*ip, node.port}, reuse_addr);
                auto host = get<std::string>(node.host);
                auto addrs = ip::local_addresses(host);
                if (addrs.empty())
                    return make_error(sec::cannot_open_port, "no local interface available", to_string(node));
                for (auto &addr : addrs) {
                    if (auto sock = make_tcp_accept_socket(ip_endpoint {addr, node.port}, reuse_addr))
                        return *sock;
                }
                return make_error(sec::cannot_open_port, "tcp socket creation failed", to_string(node));
            }

            expected<tcp_stream_socket> accept(tcp_accept_socket x) {
                auto sock = ::accept(x.id, nullptr, nullptr);
                if (sock == network::invalid_socket_id) {
                    auto err = network::last_socket_error();
                    if (err != std::errc::operation_would_block && err != std::errc::resource_unavailable_try_again) {
                        return nil::actor::make_error(sec::unavailable_or_would_block);
                    }
                    return nil::actor::make_error(sec::socket_operation_failed, "tcp accept failed");
                }
                return tcp_stream_socket {sock};
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil