//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/basp/worker.hpp>

#include <nil/actor/spawner.hpp>
#include <nil/actor/byte.hpp>
#include <nil/actor/network/basp/message_queue.hpp>
#include <nil/actor/proxy_registry.hpp>
#include <nil/actor/scheduler/abstract_coordinator.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                // -- constructors, destructors, and assignment operators ----------------------

                worker::worker(hub_type &hub, message_queue &queue, proxy_registry &proxies) :
                    hub_(&hub), queue_(&queue), proxies_(&proxies), system_(&proxies.system()) {
                    ACTOR_IGNORE_UNUSED(pad_);
                }

                worker::~worker() {
                    // nop
                }

                // -- management ---------------------------------------------------------------

                void worker::launch(const node_id &last_hop, const basp::header &hdr, span<const byte> payload) {
                    msg_id_ = queue_->new_id();
                    last_hop_ = last_hop;
                    memcpy(&hdr_, &hdr, sizeof(basp::header));
                    payload_.assign(payload.begin(), payload.end());
                    ref();
                    system_->scheduler().enqueue(this);
                }

                // -- implementation of resumable ----------------------------------------------

                resumable::resume_result worker::resume(execution_unit *ctx, size_t) {
                    ctx->proxy_registry_ptr(proxies_);
                    handle_remote_message(ctx);
                    hub_->push(this);
                    return resumable::awaiting_message;
                }

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil