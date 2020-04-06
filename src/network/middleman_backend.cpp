//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/middleman_backend.hpp>

namespace nil {
    namespace actor {
        namespace network {

            middleman_backend::middleman_backend(std::string id) : id_(std::move(id)) {
                // nop
            }

            middleman_backend::~middleman_backend() {
                // nop
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil