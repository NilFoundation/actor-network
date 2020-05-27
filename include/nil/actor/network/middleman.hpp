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

#include <thread>

#include <nil/actor/spawner.hpp>

#include <nil/actor/detail/type_list.hpp>
#include <nil/actor/fwd.hpp>
#include <nil/actor/network/fwd.hpp>

namespace nil {
    namespace actor {
        namespace network {

            class middleman : public spawner_module {
            public:
                // -- member types -----------------------------------------------------------

                using module = spawner_module;

                using module_ptr = spawner::module_ptr;

                using middleman_backend_list = std::vector<middleman_backend_ptr>;

                // -- constructors, destructors, and assignment operators --------------------

                ~middleman() override;

                // -- interface functions ----------------------------------------------------

                void start() override;

                void stop() override;

                void init(spawner_config &) override;

                id_t id() const override;

                void *subtype_ptr() override;

                // -- factory functions ------------------------------------------------------

                template<class... Ts>
                static module *make(spawner &sys, detail::type_list<Ts...> token) {
                    std::unique_ptr<middleman> result {new middleman(sys)};
                    if (sizeof...(Ts) > 0) {
                        result->backends_.reserve(sizeof...(Ts));
                        create_backends(*result, token);
                    }
                    return result.release();
                }

                // -- remoting ---------------------------------------------------------------

                /// Resolves a path to a remote actor.
                void resolve(const uri &locator, const actor &listener);

                // -- properties -------------------------------------------------------------

                spawner &system() {
                    return sys_;
                }

                const spawner_config &config() const noexcept {
                    return sys_.config();
                }

                const multiplexer_ptr &mpx() const noexcept {
                    return mpx_;
                }

                middleman_backend *backend(string_view scheme) const noexcept;

            private:
                // -- constructors, destructors, and assignment operators --------------------

                explicit middleman(spawner &sys);

                // -- utility functions ------------------------------------------------------

                static void create_backends(middleman &, detail::type_list<>) {
                    // End of recursion.
                }

                template<class T, class... Ts>
                static void create_backends(middleman &mm, detail::type_list<T, Ts...>) {
                    mm.backends_.emplace_back(new T(mm));
                    create_backends(mm, detail::type_list<Ts...> {});
                }

                // -- member variables -------------------------------------------------------

                /// Points to the parent system.
                spawner &sys_;

                /// Stores the global socket I/O multiplexer.
                multiplexer_ptr mpx_;

                /// Stores all available backends for managing peers.
                middleman_backend_list backends_;

                /// Runs the multiplexer's event loop
                std::thread mpx_thread_;
            };

        }    // namespace network
    }        // namespace actor
}    // namespace nil