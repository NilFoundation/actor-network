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

#include <cstdint>
#include <string>

#include <nil/actor/error_category.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                /// BASP-specific error codes.
                enum class ec : uint8_t {
                    invalid_magic_number = 1,
                    unexpected_number_of_bytes,
                    unexpected_payload,
                    missing_payload,
                    illegal_state,
                    invalid_handshake,
                    missing_handshake,
                    unexpected_handshake,
                    version_mismatch,
                    unimplemented = 10,
                    app_identifiers_mismatch,
                    invalid_payload,
                    invalid_scheme,
                    invalid_locator,
                };

                /// @relates ec
                std::string to_string(ec x);

            }    // namespace basp
        }        // namespace network

        template<>
        struct error_category<network::basp::ec> {
            static constexpr uint8_t value = 4;
        };
    }    // namespace actor
}    // namespace nil
