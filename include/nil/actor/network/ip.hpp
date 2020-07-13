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
#include <vector>

#include <nil/actor/fwd.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace ip {

                /// Returns all IP addresses of `host` (if any).
                std::vector<ip_address> resolve(string_view host);

                /// Returns all IP addresses of `host` (if any).
                std::vector<ip_address> resolve(ip_address host);

                /// Returns the IP addresses for a local endpoint, which is either an address,
                /// an interface name, or the string "localhost".
                std::vector<ip_address> local_addresses(string_view host);

                /// Returns the IP addresses for a local endpoint address.
                std::vector<ip_address> local_addresses(ip_address host);

                /// Returns the hostname of this device.
                std::string hostname();

            }    // namespace ip
        }        // namespace network
    }            // namespace actor
}    // namespace nil