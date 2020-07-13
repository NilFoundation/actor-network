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

#include <nil/actor/error.hpp>
#include <nil/actor/network/basp/application.hpp>
#include <nil/actor/proxy_registry.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                /// Factory for basp::application.
                /// @relates doorman
                class BOOST_SYMBOL_VISIBLE application_factory {
                public:
                    using application_type = basp::application;

                    application_factory(proxy_registry &proxies) : proxies_(proxies) {
                        // nop
                    }

                    template<class Parent>
                    error init(Parent &) {
                        return none;
                    }

                    application_type make() const {
                        return application_type {proxies_};
                    }

                private:
                    proxy_registry &proxies_;
                };

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil