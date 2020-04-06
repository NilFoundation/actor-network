//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/basp/message_queue.hpp>

#include <iterator>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                message_queue::message_queue() : next_id(0), next_undelivered(0) {
                    // nop
                }

                void message_queue::push(execution_unit *ctx, uint64_t id, strong_actor_ptr receiver,
                                         mailbox_element_ptr content) {
                    std::unique_lock<std::mutex> guard {lock};
                    ACTOR_ASSERT(id >= next_undelivered);
                    ACTOR_ASSERT(id < next_id);
                    auto first = pending.begin();
                    auto last = pending.end();
                    if (id == next_undelivered) {
                        // Dispatch current head.
                        if (receiver != nullptr)
                            receiver->enqueue(std::move(content), ctx);
                        auto next = id + 1;
                        // Check whether we can deliver more.
                        if (first == last || first->id != next) {
                            next_undelivered = next;
                            ACTOR_ASSERT(next_undelivered <= next_id);
                            return;
                        }
                        // Deliver everything until reaching a non-consecutive ID or the end.
                        auto i = first;
                        for (; i != last && i->id == next; ++i, ++next)
                            if (i->receiver != nullptr)
                                i->receiver->enqueue(std::move(i->content), ctx);
                        next_undelivered = next;
                        pending.erase(first, i);
                        ACTOR_ASSERT(next_undelivered <= next_id);
                        return;
                    }
                    // Get the insertion point.
                    auto pred = [&](const actor_msg &x) { return x.id >= id; };
                    pending.emplace(std::find_if(first, last, pred),
                                    actor_msg {id, std::move(receiver), std::move(content)});
                }

                void message_queue::drop(execution_unit *ctx, uint64_t id) {
                    push(ctx, id, nullptr, nullptr);
                }

                uint64_t message_queue::new_id() {
                    std::unique_lock<std::mutex> guard {lock};
                    return next_id++;
                }

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil