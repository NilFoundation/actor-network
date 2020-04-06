//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/socket_manager.hpp>

#include <nil/actor/config.hpp>
#include <nil/actor/network/multiplexer.hpp>

namespace nil {
    namespace actor {
        namespace network {

            socket_manager::socket_manager(socket handle, const multiplexer_ptr &parent) :
                handle_(handle), mask_(operation::none), parent_(parent) {
                ACTOR_ASSERT(parent != nullptr);
                ACTOR_ASSERT(handle_ != invalid_socket);
            }

            socket_manager::~socket_manager() {
                close(handle_);
            }

            bool socket_manager::mask_add(operation flag) noexcept {
                ACTOR_ASSERT(flag != operation::none);
                auto x = mask();
                if ((x & flag) == flag)
                    return false;
                mask_ = x | flag;
                return true;
            }

            bool socket_manager::mask_del(operation flag) noexcept {
                ACTOR_ASSERT(flag != operation::none);
                auto x = mask();
                if ((x & flag) == operation::none)
                    return false;
                mask_ = x & ~flag;
                return true;
            }

            void socket_manager::register_reading() {
                if ((mask() & operation::read) == operation::read)
                    return;
                auto ptr = parent_.lock();
                if (ptr != nullptr)
                    ptr->register_reading(this);
            }

            void socket_manager::register_writing() {
                if ((mask() & operation::write) == operation::write)
                    return;
                auto ptr = parent_.lock();
                if (ptr != nullptr)
                    ptr->register_writing(this);
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil