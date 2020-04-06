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

#include <string>

namespace nil {
    namespace actor {
        namespace network {

            /// Values for representing bitmask of I/O operations.
            enum class operation {
                none = 0x00,
                read = 0x01,
                write = 0x02,
                read_write = 0x03,
                shutdown = 0x04,
            };

            constexpr operation operator|(operation x, operation y) {
                return static_cast<operation>(static_cast<int>(x) | static_cast<int>(y));
            }

            constexpr operation operator&(operation x, operation y) {
                return static_cast<operation>(static_cast<int>(x) & static_cast<int>(y));
            }

            constexpr operation operator^(operation x, operation y) {
                return static_cast<operation>(static_cast<int>(x) ^ static_cast<int>(y));
            }

            constexpr operation operator~(operation x) {
                return static_cast<operation>(~static_cast<int>(x));
            }

            std::string to_string(operation x);

        }    // namespace network
    }        // namespace actor
}    // namespace nil