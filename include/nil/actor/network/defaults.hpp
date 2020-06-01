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

#include <vector>
#include <thread>

#include <boost/config.hpp>

#include <nil/actor/string_view.hpp>

// -- hard-coded default values for various CAF options ------------------------

namespace nil {
    namespace actor {
        namespace defaults {
            namespace middleman {

                BOOST_SYMBOL_VISIBLE static const std::vector<std::string> app_identifiers = {"default-application"};
                BOOST_SYMBOL_VISIBLE static const char *network_backend = "default";
                BOOST_SYMBOL_VISIBLE static const bool enable_automatic_connections = false;
                BOOST_SYMBOL_VISIBLE static const std::size_t max_consecutive_reads = 50;
                BOOST_SYMBOL_VISIBLE static const std::size_t heartbeat_interval = 0;
                BOOST_SYMBOL_VISIBLE static const bool manual_multiplexing = false;
                BOOST_SYMBOL_VISIBLE static const std::size_t cached_udp_buffers = 10;
                BOOST_SYMBOL_VISIBLE static const std::size_t max_pending_msgs = 10;
                BOOST_SYMBOL_VISIBLE static const std::size_t workers =
                    std::min(3u, std::thread::hardware_concurrency() / 4u) + 1;

                /// Maximum number of cached buffers for sending payloads.
                BOOST_SYMBOL_VISIBLE static const std::size_t max_payload_buffers = 100;

                /// Maximum number of cached buffers for sending headers.
                BOOST_SYMBOL_VISIBLE static const std::size_t max_header_buffers = 10;

            }    // namespace middleman
        }        // namespace defaults
    }            // namespace actor
}    // namespace nil