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

#include <nil/actor/network/socket_id.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// Closes the guarded socket when destroyed.
            template<class Socket>
            class socket_guard {
            public:
                socket_guard() noexcept : sock_(invalid_socket_id) {
                    // nop
                }

                explicit socket_guard(Socket sock) noexcept : sock_(sock) {
                    // nop
                }

                socket_guard(socket_guard &&other) noexcept : sock_(other.release()) {
                    // nop
                }

                socket_guard(const socket_guard &) = delete;

                socket_guard &operator=(socket_guard &&other) noexcept {
                    reset(other.release());
                    return *this;
                }

                socket_guard &operator=(const socket_guard &) = delete;

                ~socket_guard() {
                    if (sock_.id != invalid_socket_id)
                        close(sock_);
                }

                void reset(Socket x) noexcept {
                    if (sock_.id != invalid_socket_id)
                        close(sock_);
                    sock_ = x;
                }

                Socket release() noexcept {
                    auto sock = sock_;
                    sock_.id = invalid_socket_id;
                    return sock;
                }

                Socket socket() const noexcept {
                    return sock_;
                }

            private:
                Socket sock_;
            };

            template<class Socket>
            socket_guard<Socket> make_socket_guard(Socket sock) {
                return socket_guard<Socket> {sock};
            }
        }    // namespace network
    }        // namespace actor
}    // namespace nil
