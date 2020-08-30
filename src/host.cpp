//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/host.hpp>
#include <nil/actor/network/socket.hpp>

#include <nil/actor/detail/net_syscall.hpp>
#include <nil/actor/detail/socket_sys_includes.hpp>

#include <nil/actor/config.hpp>
#include <nil/actor/error.hpp>
#include <nil/actor/message.hpp>
#include <nil/actor/none.hpp>

namespace nil {
    namespace actor {
        namespace network {

#ifdef BOOST_OS_WINDOWS_AVAILABLE

            error this_host::startup() {
                WSADATA WinsockData;
                ACTOR_NET_SYSCALL("WSAStartup", result, !=, 0, WSAStartup(MAKEWORD(2, 2), &WinsockData));
                return none;
            }

            void this_host::cleanup() {
                WSACleanup();
            }

#else    // BOOST_OS_WINDOWS_AVAILABLE

            error this_host::startup() {
                return none;
            }

            void this_host::cleanup() {
                // nop
            }

#endif    // BOOST_OS_WINDOWS_AVAILABLE

        }    // namespace network
    }        // namespace actor
}    // namespace nil