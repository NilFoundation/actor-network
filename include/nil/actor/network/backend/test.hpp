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

#include <map>

#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/fwd.hpp>
#include <nil/actor/network/middleman_backend.hpp>
#include <nil/actor/network/stream_socket.hpp>
#include <nil/actor/node_id.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace backend {

                /// Minimal backend for unit testing.
                /// @warning this backend is *not* thread safe.
                class test : public middleman_backend {
                public:
                    // -- member types -----------------------------------------------------------

                    using peer_entry = std::pair<stream_socket, endpoint_manager_ptr>;

                    // -- constructors, destructors, and assignment operators --------------------

                    test(middleman &mm);

                    ~test() override;

                    // -- interface functions ----------------------------------------------------

                    error init() override;

                    void stop() override;

                    endpoint_manager_ptr peer(const node_id &id) override;

                    void resolve(const uri &locator, const actor &listener) override;

                    strong_actor_ptr make_proxy(node_id nid, actor_id aid) override;

                    void set_last_hop(node_id *) override;

                    // -- properties -------------------------------------------------------------

                    stream_socket socket(const node_id &peer_id) {
                        return get_peer(peer_id).first;
                    }

                    peer_entry &emplace(const node_id &peer_id, stream_socket first, stream_socket second);

                private:
                    peer_entry &get_peer(const node_id &id);

                    middleman &mm_;

                    std::map<node_id, peer_entry> peers_;

                    proxy_registry proxies_;
                };

            }    // namespace backend
        }        // namespace network
    }            // namespace actor
}    // namespace nil