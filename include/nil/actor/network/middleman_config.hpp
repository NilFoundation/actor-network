//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#ifndef DBMS_MIDDLEMAN_CONFIG_HPP
#define DBMS_MIDDLEMAN_CONFIG_HPP

#include <thread>

#include <nil/actor/spawner.hpp>
#include <nil/actor/detail/type_list.hpp>
#include <nil/actor/fwd.hpp>
#include <nil/actor/network/fwd.hpp>
#include <nil/actor/network/defaults.hpp>

namespace nil {
    namespace actor {
        namespace network {
            struct middleman_config {
                middleman_config();

                virtual ~middleman_config();

                const std::vector<std::string> app_identifiers;
                const char *network_backend;
                const bool enable_automatic_connections;
                const size_t max_consecutive_reads;
                const size_t heartbeat_interval;
                const bool manual_multiplexing;
                const size_t cached_udp_buffers;
                const size_t max_pending_msgs;
                const size_t max_payload_buffers;
                const size_t max_header_buffers;
                const size_t workers;
                const uri this_node;
            };
        }    // namespace network
    }        // namespace actor
}    // namespace nil

#endif    // DBMS_MIDDLEMAN_CONFIG_HPP
