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

#include <array>
#include <cstdint>
#include <mutex>

#include <nil/actor/byte.hpp>
#include <nil/actor/network/pipe_socket.hpp>
#include <nil/actor/network/socket_manager.hpp>

namespace nil {
    namespace actor {
        namespace network {

            class pollset_updater : public socket_manager {
            public:
                // -- member types -----------------------------------------------------------

                using super = socket_manager;

                using msg_buf = std::array<byte, sizeof(intptr_t) + 1>;

                // -- constructors, destructors, and assignment operators --------------------

                pollset_updater(pipe_socket read_handle, const multiplexer_ptr &parent);

                ~pollset_updater() override;

                // -- properties -------------------------------------------------------------

                /// Returns the managed socket.
                pipe_socket handle() const noexcept {
                    return socket_cast<pipe_socket>(handle_);
                }

                // -- interface functions ----------------------------------------------------

                bool handle_read_event() override;

                bool handle_write_event() override;

                void handle_error(sec code) override;

            private:
                msg_buf buf_;
                size_t buf_size_;
            };

        }    // namespace network
    }        // namespace actor
}    // namespace nil