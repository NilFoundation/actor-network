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

#include <nil/actor/network/actor_proxy_impl.hpp>

#include <nil/actor/spawner.hpp>
#include <nil/actor/expected.hpp>
#include <nil/actor/logger.hpp>

namespace nil {
    namespace actor {
        namespace network {

            actor_proxy_impl::actor_proxy_impl(actor_config &cfg, endpoint_manager_ptr dst) :
                super(cfg), sf_(dst->serialize_fun()), dst_(std::move(dst)) {
                ACTOR_ASSERT(dst_ != nullptr);
                dst_->enqueue_event(node(), id());
            }

            actor_proxy_impl::~actor_proxy_impl() {
                // nop
            }

            void actor_proxy_impl::enqueue(mailbox_element_ptr what, execution_unit *) {
                ACTOR_PUSH_AID(0);
                ACTOR_ASSERT(what != nullptr);
                ACTOR_LOG_SEND_EVENT(what);
                if (auto payload = sf_(home_system(), what->content()))
                    dst_->enqueue(std::move(what), ctrl(), std::move(*payload));
                else
                    ACTOR_LOG_ERROR("unable to serialize payload: " << home_system().render(payload.error()));
            }

            void actor_proxy_impl::kill_proxy(execution_unit *ctx, error rsn) {
                cleanup(std::move(rsn), ctx);
            }
        }    // namespace network
    }        // namespace actor
}    // namespace nil