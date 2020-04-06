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


#include <nil/actor/make_counted.hpp>
#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/endpoint_manager_impl.hpp>

namespace nil {
    namespace actor {
        namespace network {

            template<class Transport>
            endpoint_manager_ptr make_endpoint_manager(const multiplexer_ptr &mpx, spawner &sys,
                                                                      Transport trans) {
                using impl = endpoint_manager_impl<Transport>;
                return make_counted<impl>(mpx, sys, std::move(trans));
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil