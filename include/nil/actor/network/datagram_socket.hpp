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

#include <nil/actor/network/network_socket.hpp>
#include <nil/actor/variant.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// A datagram-oriented network communication endpoint.
            struct datagram_socket : network_socket {
                using super = network_socket;

                using super::super;
            };

            /// Enables or disables `SIO_UDP_CONNRESET` error on `x`.
            /// @relates datagram_socket
            error allow_connreset(datagram_socket x, bool new_value);

            /// Converts the result from I/O operation on a ::datagram_socket to either an
            /// error code or a integer greater or equal to zero.
            /// @relates datagram_socket
            variant<size_t, sec> check_datagram_socket_io_res(std::make_signed<size_t>::type res);

        }    // namespace network
    }        // namespace actor
}    // namespace nil