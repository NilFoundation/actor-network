//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/middleman_config.hpp>

namespace nil {
    namespace actor {
        namespace network {
            middleman_config::middleman_config() :
                app_identifiers(nil::actor::defaults::middleman::app_identifiers),
                network_backend(nil::actor::defaults::middleman::network_backend),
                enable_automatic_connections(nil::actor::defaults::middleman::enable_automatic_connections),
                max_consecutive_reads(nil::actor::defaults::middleman::max_consecutive_reads),
                heartbeat_interval(nil::actor::defaults::middleman::heartbeat_interval),
                manual_multiplexing(nil::actor::defaults::middleman::manual_multiplexing),
                cached_udp_buffers(nil::actor::defaults::middleman::cached_udp_buffers),
                max_pending_msgs(nil::actor::defaults::middleman::max_pending_msgs),
                max_payload_buffers(nil::actor::defaults::middleman::max_payload_buffers),
                max_header_buffers(nil::actor::defaults::middleman::max_header_buffers),
                workers(nil::actor::defaults::middleman::workers) {
            }
            middleman_config::~middleman_config() {
            }
        }    // namespace network
    }        // namespace actor
}    // namespace nil
