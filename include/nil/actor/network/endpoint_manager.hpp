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

#include <cstddef>
#include <cstdint>
#include <memory>

#include <nil/actor/actor.hpp>
#include <nil/actor/actor_clock.hpp>
#include <nil/actor/byte.hpp>

#include <nil/actor/fwd.hpp>
#include <nil/actor/intrusive/drr_queue.hpp>
#include <nil/actor/intrusive/fifo_inbox.hpp>
#include <nil/actor/intrusive/singly_linked.hpp>
#include <nil/actor/mailbox_element.hpp>
#include <nil/actor/network/endpoint_manager_queue.hpp>
#include <nil/actor/network/socket_manager.hpp>
#include <nil/actor/variant.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// Manages a communication endpoint.
            class endpoint_manager : public socket_manager {
            public:
                // -- member types -----------------------------------------------------------

                using super = socket_manager;

                /// Represents either an error or a serialized payload.
                using maybe_buffer = expected<std::vector<byte>>;

                /// A function type for serializing message payloads.
                using serialize_fun_type = maybe_buffer (*)(spawner &, const message &);

                // -- constructors, destructors, and assignment operators --------------------

                endpoint_manager(socket handle, const multiplexer_ptr &parent, spawner &sys);

                ~endpoint_manager() override;

                // -- properties -------------------------------------------------------------

                spawner &system() {
                    return sys_;
                }

                endpoint_manager_queue::message_ptr next_message();

                // -- event management -------------------------------------------------------

                /// Resolves a path to a remote actor.
                void resolve(uri locator, actor listener);

                /// Enqueues a message to the endpoint.
                void enqueue(mailbox_element_ptr msg, strong_actor_ptr receiver, std::vector<byte> payload);

                /// Enqueues an event to the endpoint.
                template<class... Ts>
                void enqueue_event(Ts &&... xs) {
                    enqueue(new endpoint_manager_queue::event(std::forward<Ts>(xs)...));
                }

                // -- pure virtual member functions ------------------------------------------

                /// Initializes the manager before adding it to the multiplexer's event loop.
                virtual error init() = 0;

                /// @returns the protocol-specific function for serializing payloads.
                virtual serialize_fun_type serialize_fun() const noexcept = 0;

            protected:
                bool enqueue(endpoint_manager_queue::element *ptr);

                /// Points to the hosting actor system.
                spawner &sys_;

                /// Stores control events and outbound messages.
                endpoint_manager_queue::type queue_;

                /// Stores a proxy for interacting with the actor clock.
                actor timeout_proxy_;
            };

            using endpoint_manager_ptr = intrusive_ptr<endpoint_manager>;

        }    // namespace network
    }        // namespace actor
}    // namespace nil