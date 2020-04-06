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

#include <vector>

#include <nil/actor/network/basp/header.hpp>

#include <nil/actor/detail/scope_guard.hpp>
#include <nil/actor/detail/sync_request_bouncer.hpp>

#include <nil/actor/actor_control_block.hpp>
#include <nil/actor/actor_proxy.hpp>
#include <nil/actor/binary_deserializer.hpp>
#include <nil/actor/config.hpp>
#include <nil/actor/execution_unit.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/message.hpp>
#include <nil/actor/message_id.hpp>
#include <nil/actor/node_id.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                template<class Subtype>
                class remote_message_handler {
                public:
                    void handle_remote_message(execution_unit *ctx) {
                        // Local variables.
                        auto &dref = static_cast<Subtype &>(*this);
                        auto &payload = dref.payload_;
                        auto &hdr = dref.hdr_;
                        auto &registry = dref.system_->registry();
                        auto &proxies = *dref.proxies_;
                        ACTOR_LOG_TRACE(ACTOR_ARG(hdr) << ACTOR_ARG2("payload.size", payload.size()));
                        // Deserialize payload.
                        actor_id src_id = 0;
                        node_id src_node;
                        actor_id dst_id = 0;
                        std::vector<strong_actor_ptr> fwd_stack;
                        message content;
                        binary_deserializer source {ctx, payload};
                        if (auto err = source(src_node, src_id, dst_id, fwd_stack, content)) {
                            ACTOR_LOG_ERROR("could not deserialize payload: " << ACTOR_ARG(err));
                            return;
                        }
                        // Sanity checks.
                        if (dst_id == 0)
                            return;
                        // Try to fetch the receiver.
                        auto dst_hdl = registry.get(dst_id);
                        if (dst_hdl == nullptr) {
                            ACTOR_LOG_DEBUG("no actor found for given ID, drop message");
                            return;
                        }
                        // Try to fetch the sender.
                        strong_actor_ptr src_hdl;
                        if (src_node != none && src_id != 0)
                            src_hdl = proxies.get_or_put(src_node, src_id);
                        // Ship the message.
                        auto ptr = make_mailbox_element(std::move(src_hdl), make_message_id(hdr.operation_data),
                                                        std::move(fwd_stack), std::move(content));
                        dref.queue_->push(ctx, dref.msg_id_, std::move(dst_hdl), std::move(ptr));
                    }
                };
            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil
