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

#include <nil/actor/config.hpp>

namespace nil {
    namespace actor {
        namespace network {

#ifdef BOOST_OS_WINDOWS_AVAILABLE

            using setsockopt_ptr = const char *;
            using getsockopt_ptr = char *;
            using socket_send_ptr = const char *;
            using socket_recv_ptr = char *;
            using socket_size_type = int;

#else    // BOOST_OS_WINDOWS_AVAILABLE

            using setsockopt_ptr = const void *;
            using getsockopt_ptr = void *;
            using socket_send_ptr = const void *;
            using socket_recv_ptr = void *;
            using socket_size_type = unsigned;

#endif    // BOOST_OS_WINDOWS_AVAILABLE

        }    // namespace network
    }        // namespace actor
}    // namespace nil
