//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2018-2020 Nil Foundation AG
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#pragma once

#include <nil/actor/detail/socket_sys_includes.hpp>

namespace nil {
    namespace actor {
        namespace detail {

            inline auto addr_of(sockaddr_in &what) -> decltype(what.sin_addr) & {
                return what.sin_addr;
            }

            inline auto family_of(sockaddr_in &what) -> decltype(what.sin_family) & {
                return what.sin_family;
            }

            inline auto port_of(sockaddr_in &what) -> decltype(what.sin_port) & {
                return what.sin_port;
            }

            inline auto addr_of(sockaddr_in6 &what) -> decltype(what.sin6_addr) & {
                return what.sin6_addr;
            }

            inline auto family_of(sockaddr_in6 &what) -> decltype(what.sin6_family) & {
                return what.sin6_family;
            }

            inline auto port_of(sockaddr_in6 &what) -> decltype(what.sin6_port) & {
                return what.sin6_port;
            }

        }    // namespace detail
    }        // namespace actor
}    // namespace nil