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

#include <cstddef>
#include <cstdint>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                /// @addtogroup BASP

                /// The current BASP version.
                /// @note BASP is not backwards compatible.
                constexpr uint64_t version = 1;

                /// Size of a BASP header in serialized form.
                constexpr size_t header_size = 13;

                /// @}

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil
