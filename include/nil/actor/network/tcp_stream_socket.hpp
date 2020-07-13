//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#pragma once

#include <nil/actor/ip_endpoint.hpp>
#include <nil/actor/network/socket.hpp>
#include <nil/actor/network/stream_socket.hpp>
#include <nil/actor/uri.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// Represents a TCP connection.
            struct tcp_stream_socket : stream_socket {
                using super = stream_socket;

                using super::super;
            };

            /// Creates a `tcp_stream_socket` connected to given remote node.
            /// @param node Host and port of the remote node.
            /// @returns The connected socket or an error.
            /// @relates tcp_stream_socket
            expected<tcp_stream_socket> make_connected_tcp_stream_socket(ip_endpoint node);

            /// Create a `tcp_stream_socket` connected to `auth`.
            /// @param node Host and port of the remote node.
            /// @returns The connected socket or an error.
            /// @relates tcp_stream_socket
            expected<tcp_stream_socket> make_connected_tcp_stream_socket(const uri::authority_type &node);

        }    // namespace network
    }        // namespace actor
}    // namespace nil
