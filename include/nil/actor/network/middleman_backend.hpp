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

#include <string>


#include <nil/actor/fwd.hpp>
#include <nil/actor/network/fwd.hpp>
#include <nil/actor/proxy_registry.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// Technology-specific backend for connecting to and managing peer
            /// connections.
            /// @relates middleman
            class middleman_backend : public proxy_registry::backend {
            public:
                // -- constructors, destructors, and assignment operators --------------------

                middleman_backend(std::string id);

                virtual ~middleman_backend();

                // -- interface functions ----------------------------------------------------

                /// Initializes the backend.
                virtual error init() = 0;

                /// @returns The endpoint manager for `peer` on success, `nullptr` otherwise.
                virtual endpoint_manager_ptr peer(const node_id &id) = 0;

                /// Resolves a path to a remote actor.
                virtual void resolve(const uri &locator, const actor &listener) = 0;

                virtual void stop() = 0;

                // -- properties -------------------------------------------------------------

                const std::string &id() const noexcept {
                    return id_;
                }

            private:
                /// Stores the technology-specific identifier.
                std::string id_;
            };

            /// @relates middleman_backend
            using middleman_backend_ptr = std::unique_ptr<middleman_backend>;

        }    // namespace network
    }        // namespace actor
}    // namespace nil