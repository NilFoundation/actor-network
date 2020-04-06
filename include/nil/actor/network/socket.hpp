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

#include <string>
#include <system_error>
#include <type_traits>

#include <nil/actor/config.hpp>
#include <nil/actor/detail/comparable.hpp>

#include <nil/actor/fwd.hpp>
#include <nil/actor/network/socket_id.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// An internal endpoint for sending or receiving data. Can be either a
            /// ::network_socket, ::pipe_socket, ::stream_socket, or ::datagram_socket.
            struct socket : detail::comparable<socket> {
                socket_id id;

                constexpr socket() noexcept : id(invalid_socket_id) {
                    // nop
                }

                constexpr explicit socket(socket_id id) noexcept : id(id) {
                    // nop
                }

                constexpr socket(const socket &other) noexcept = default;

                socket &operator=(const socket &other) noexcept = default;

                constexpr signed_socket_id compare(socket other) const noexcept {
                    return static_cast<signed_socket_id>(id) - static_cast<signed_socket_id>(other.id);
                }
            };

            /// @relates socket
            template<class Inspector>
            typename Inspector::result_type inspect(Inspector &f, socket &x) {
                return f(x.id);
            }

            /// Denotes the invalid socket.
            constexpr auto invalid_socket = socket {invalid_socket_id};

            /// Converts between different socket types.
            template<class To, class From>
            To socket_cast(From x) {
                return To {x.id};
            }

            /// Close socket `x`.
            /// @relates socket
            void close(socket x);

            /// Returns the last socket error in this thread as an integer.
            /// @relates socket
            std::errc last_socket_error();

            /// Returns the last socket error as human-readable string.
            /// @relates socket
            std::string last_socket_error_as_string();

            /// Sets x to be inherited by child processes if `new_value == true`
            /// or not if `new_value == false`.  Not implemented on Windows.
            /// @relates socket
            error child_process_inherit(socket x, bool new_value);

            /// Enables or disables nonblocking I/O on `x`.
            /// @relates socket
            error nonblocking(socket x, bool new_value);

        }    // namespace network
    }        // namespace actor
}    // namespace nil
