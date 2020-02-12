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

#include <cstdint>
#include <string>
#include <system_error>
#include <type_traits>

#include <nil/actor/config.hpp>

#include <nil/actor/fwd.hpp>
#include <nil/actor/network/socket.hpp>
#include <nil/actor/network/socket_id.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// A bidirectional network communication endpoint.
            struct network_socket : socket {
                using super = socket;

                using super::super;
            };

            /// Enables or disables `SIGPIPE` events from `x`.
            /// @relates network_socket
            error allow_sigpipe(network_socket x, bool new_value);

            /// Enables or disables `SIO_UDP_CONNRESET`error on `x`.
            /// @relates network_socket
            error allow_udp_connreset(network_socket x, bool new_value);

            /// Get the socket buffer size for `x`.
            /// @pre `x != invalid_socket`
            /// @relates network_socket
            expected<size_t> send_buffer_size(network_socket x);

            /// Set the socket buffer size for `x`.
            /// @relates network_socket
            error send_buffer_size(network_socket x, size_t capacity);

            /// Returns the locally assigned port of `x`.
            /// @relates network_socket
            expected<uint16_t> local_port(network_socket x);

            /// Returns the locally assigned address of `x`.
            /// @relates network_socket
            expected<std::string> local_addr(network_socket x);

            /// Returns the port used by the remote host of `x`.
            /// @relates network_socket
            expected<uint16_t> remote_port(network_socket x);

            /// Returns the remote host address of `x`.
            /// @relates network_socket
            expected<std::string> remote_addr(network_socket x);

            /// Closes the read channel for a socket.
            /// @relates network_socket
            void shutdown_read(network_socket x);

            /// Closes the write channel for a socket.
            /// @relates network_socket
            void shutdown_write(network_socket x);

            /// Closes the both read and write channel for a socket.
            /// @relates network_socket
            void shutdown(network_socket x);

        }    // namespace network
    }        // namespace actor
}    // namespace nil