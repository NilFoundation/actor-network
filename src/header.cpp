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

#include <nil/actor/network/basp/header.hpp>

#include <cstring>

#include <nil/actor/byte.hpp>
#include <nil/actor/detail/network_order.hpp>
#include <nil/actor/span.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                namespace {

                    void to_bytes_impl(const header &x, byte *ptr) {
                        *ptr = static_cast<byte>(x.type);
                        auto payload_len = detail::to_network_order(x.payload_len);
                        memcpy(ptr + 1, &payload_len, sizeof(payload_len));
                        auto operation_data = detail::to_network_order(x.operation_data);
                        memcpy(ptr + 5, &operation_data, sizeof(operation_data));
                    }

                }    // namespace

                int header::compare(header other) const noexcept {
                    auto x = to_bytes(*this);
                    auto y = to_bytes(other);
                    return memcmp(x.data(), y.data(), header_size);
                }

                header header::from_bytes(span<const byte> bytes) {
                    ACTOR_ASSERT(bytes.size() >= header_size);
                    header result;
                    auto ptr = bytes.data();
                    result.type = *reinterpret_cast<const message_type *>(ptr);
                    uint32_t payload_len = 0;
                    memcpy(&payload_len, ptr + 1, 4);
                    result.payload_len = detail::from_network_order(payload_len);
                    uint64_t operation_data;
                    memcpy(&operation_data, ptr + 5, 8);
                    result.operation_data = detail::from_network_order(operation_data);
                    return result;
                }

                std::array<byte, header_size> to_bytes(header x) {
                    std::array<byte, header_size> result {};
                    to_bytes_impl(x, result.data());
                    return result;
                }

                void to_bytes(header x, std::vector<byte> &buf) {
                    buf.resize(header_size);
                    to_bytes_impl(x, buf.data());
                }

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil