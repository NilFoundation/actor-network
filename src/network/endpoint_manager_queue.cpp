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

#include <nil/actor/network/endpoint_manager_queue.hpp>

namespace nil {
    namespace actor {
        namespace network {

            endpoint_manager_queue::element::~element() {
                // nop
            }

            endpoint_manager_queue::event::event(uri locator, actor listener) :
                element(element_type::event), value(resolve_request {std::move(locator), std::move(listener)}) {
                // nop
            }

            endpoint_manager_queue::event::event(node_id peer, actor_id proxy_id) :
                element(element_type::event), value(new_proxy {peer, proxy_id}) {
                // nop
            }

            endpoint_manager_queue::event::event(node_id observing_peer, actor_id local_actor_id, error reason) :
                element(element_type::event),
                value(local_actor_down {observing_peer, local_actor_id, std::move(reason)}) {
                // nop
            }

            endpoint_manager_queue::event::event(std::string tag, uint64_t id) :
                element(element_type::event), value(timeout {std::move(tag), id}) {
                // nop
            }

            endpoint_manager_queue::event::~event() {
                // nop
            }

            size_t endpoint_manager_queue::event::task_size() const noexcept {
                return 1;
            }

            endpoint_manager_queue::message::message(mailbox_element_ptr msg,
                                                     strong_actor_ptr receiver,
                                                     std::vector<byte>
                                                         payload) :
                element(element_type::message),
                msg(std::move(msg)), receiver(std::move(receiver)), payload(std::move(payload)) {
                // nop
            }

            size_t endpoint_manager_queue::message::task_size() const noexcept {
                return message_policy::task_size(*this);
            }

            endpoint_manager_queue::message::~message() {
                // nop
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil