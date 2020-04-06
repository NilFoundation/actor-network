//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/detail/convert_ip_endpoint.hpp>

#include <cstring>

#include <nil/actor/error.hpp>
#include <nil/actor/ipv4_endpoint.hpp>
#include <nil/actor/ipv6_endpoint.hpp>
#include <nil/actor/sec.hpp>

namespace nil {
    namespace actor {
        namespace detail {

            void convert(const ip_endpoint &src, sockaddr_storage &dst) {
                memset(&dst, 0, sizeof(sockaddr_storage));
                if (src.address().embeds_v4()) {
                    auto sockaddr4 = reinterpret_cast<sockaddr_in *>(&dst);
                    sockaddr4->sin_family = AF_INET;
                    sockaddr4->sin_port = ntohs(src.port());
                    sockaddr4->sin_addr.s_addr = src.address().embedded_v4().bits();
                } else {
                    auto sockaddr6 = reinterpret_cast<sockaddr_in6 *>(&dst);
                    sockaddr6->sin6_family = AF_INET6;
                    sockaddr6->sin6_port = ntohs(src.port());
                    memcpy(&sockaddr6->sin6_addr, src.address().bytes().data(), src.address().bytes().size());
                }
            }

            error convert(const sockaddr_storage &src, ip_endpoint &dst) {
                if (src.ss_family == AF_INET) {
                    auto sockaddr4 = reinterpret_cast<const sockaddr_in &>(src);
                    ipv4_address ipv4_addr;
                    memcpy(ipv4_addr.data().data(), &sockaddr4.sin_addr, ipv4_addr.size());
                    dst = ip_endpoint {ipv4_addr, htons(sockaddr4.sin_port)};
                } else if (src.ss_family == AF_INET6) {
                    auto sockaddr6 = reinterpret_cast<const sockaddr_in6 &>(src);
                    ipv6_address ipv6_addr;
                    memcpy(ipv6_addr.bytes().data(), &sockaddr6.sin6_addr, ipv6_addr.bytes().size());
                    dst = ip_endpoint {ipv6_addr, htons(sockaddr6.sin6_port)};
                } else {
                    return sec::invalid_argument;
                }
                return none;
            }

        }    // namespace detail
    }        // namespace actor
}    // namespace nil