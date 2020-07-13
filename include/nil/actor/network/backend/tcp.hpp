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
#include <mutex>

#include <nil/actor/detail/net_export.hpp>
#include <nil/actor/error.hpp>
#include <nil/actor/expected.hpp>
#include <nil/actor/network/basp/application.hpp">
#include <nil/actor/network/fwd.hpp>
#include <nil/actor/network/make_endpoint_manager.hpp>
#include <nil/actor/network/middleman.hpp>
#include <nil/actor/network/middleman_backend.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/network/stream_transport.hpp>
#include <nil/actor/network/tcp_stream_socket.hpp>
#include <nil/actor/node_id.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace backend {

                /// Minimal backend for tcp communication.
                class BOOST_SYMBOL_VISIBLE tcp : public middleman_backend {
                public:
                    using peer_map = std::map<node_id, endpoint_manager_ptr>;

                    using emplace_return_type = std::pair<peer_map::iterator, bool>;

                    // -- constructors, destructors, and assignment operators --------------------

                    tcp(middleman &mm);

                    ~tcp() override;

                    // -- interface functions ----------------------------------------------------

                    error init() override;

                    void stop() override;

                    expected<endpoint_manager_ptr> get_or_connect(const uri &locator) override;

                    endpoint_manager_ptr peer(const node_id &id) override;

                    void resolve(const uri &locator, const actor &listener) override;

                    strong_actor_ptr make_proxy(node_id nid, actor_id aid) override;

                    void set_last_hop(node_id *) override;

                    // -- properties -------------------------------------------------------------

                    uint16_t port() const noexcept override;

                    template<class Handle>
                    expected<endpoint_manager_ptr> emplace(const node_id &peer_id, Handle socket_handle) {
                        using transport_type = stream_transport<basp::application>;
                        if (auto err = nonblocking(socket_handle, true))
                            return err;
                        auto mpx = mm_.mpx();
                        basp::application app {proxies_};
                        auto mgr =
                            make_endpoint_manager(mpx, mm_.system(), transport_type {socket_handle, std::move(app)});
                        if (auto err = mgr->init()) {
                            ACTOR_LOG_ERROR("mgr->init() failed: " << err);
                            return err;
                        }
                        mpx->register_reading(mgr);
                        emplace_return_type res;
                        {
                            const std::lock_guard<std::mutex> lock(lock_);
                            res = peers_.emplace(peer_id, std::move(mgr));
                        }
                        if (res.second)
                            return res.first->second;
                        else
                            return make_error(sec::runtime_error, "peer_id already exists");
                    }

                private:
                    endpoint_manager_ptr get_peer(const node_id &id);

                    middleman &mm_;

                    peer_map peers_;

                    proxy_registry proxies_;

                    uint16_t listening_port_;

                    std::mutex lock_;
                };

            }    // namespace backend
        }        // namespace network
    }            // namespace actor
}    // namespace nil