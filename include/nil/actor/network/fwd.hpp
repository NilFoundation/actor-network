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

#include <memory>

#include <nil/actor/intrusive_ptr.hpp>

namespace nil {
    namespace actor {
        namespace network {

            // -- templates ----------------------------------------------------------------

            template<class Application>
            class stream_transport;

            template<class Factory>
            class datagram_transport;

            template<class Application, class IdType = unit_t>
            class transport_worker;

            template<class Transport, class IdType = unit_t>
            class transport_worker_dispatcher;

            // -- classes ------------------------------------------------------------------

            class endpoint_manager;
            class middleman;
            class middleman_backend;
            class multiplexer;
            class socket_manager;

            // -- structs ------------------------------------------------------------------

            struct network_socket;
            struct pipe_socket;
            struct socket;
            struct stream_socket;
            struct tcp_accept_socket;
            struct tcp_stream_socket;
            struct datagram_socket;
            struct udp_datagram_socket;

            // -- smart pointers -----------------------------------------------------------

            using endpoint_manager_ptr = intrusive_ptr<endpoint_manager>;
            using middleman_backend_ptr = std::unique_ptr<middleman_backend>;
            using multiplexer_ptr = std::shared_ptr<multiplexer>;
            using socket_manager_ptr = intrusive_ptr<socket_manager>;
            using weak_multiplexer_ptr = std::weak_ptr<multiplexer>;

        }    // namespace network
    }        // namespace actor
}    // namespace nil
