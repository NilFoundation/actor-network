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

#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

#include <nil/actor/config.hpp>

namespace nil {
    namespace actor {
        namespace network {

#ifdef ACTOR_WINDOWS

            /// Platform-specific representation of a socket.
            /// @relates socket
            using socket_id = size_t;

            /// Identifies the invalid socket.
            constexpr socket_id invalid_socket_id = std::numeric_limits<socket_id>::max();

#else    // ACTOR_WINDOWS

            /// Platform-specific representation of a socket.
            /// @relates socket
            using socket_id = int;

            /// Identifies the invalid socket.
            constexpr socket_id invalid_socket_id = -1;

#endif    // ACTOR_WINDOWS

            /// Signed counterpart of `socket_id`.
            /// @relates socket
            using signed_socket_id = std::make_signed<socket_id>::type;
        }    // namespace network
    }        // namespace actor
}    // namespace nil
