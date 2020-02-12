//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2018-2020 Nil Foundation AG
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/defaults.hpp>

namespace nil {
    namespace actor {
        namespace defaults {
            namespace middleman {

                const size_t max_payload_buffers = 100;

                const size_t max_header_buffers = 10;

            }    // namespace middleman
        }        // namespace defaults
    }            // namespace actor
}    // namespace nil