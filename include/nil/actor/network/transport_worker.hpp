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

#include <nil/actor/network/endpoint_manager_queue.hpp>
#include <nil/actor/network/fwd.hpp>
#include <nil/actor/network/packet_writer_decorator.hpp>
#include <nil/actor/unit.hpp>

namespace nil {
    namespace actor {
        namespace network {

            /// Implements a worker for transport protocols.
            template<class Application, class IdType>
            class transport_worker {
            public:
                // -- member types -----------------------------------------------------------

                using id_type = IdType;

                using application_type = Application;

                // -- constructors, destructors, and assignment operators --------------------

                explicit transport_worker(application_type application, id_type id = id_type {}) :
                    application_(std::move(application)), id_(std::move(id)) {
                    // nop
                }

                // -- properties -------------------------------------------------------------

                application_type &application() noexcept {
                    return application_;
                }

                const application_type &application() const noexcept {
                    return application_;
                }

                const id_type &id() const noexcept {
                    return id_;
                }

                // -- member functions -------------------------------------------------------

                template<class Parent>
                error init(Parent &parent) {
                    auto writer = make_packet_writer_decorator(*this, parent);
                    return application_.init(writer);
                }

                template<class Parent>
                error handle_data(Parent &parent, span<const byte> data) {
                    auto writer = make_packet_writer_decorator(*this, parent);
                    return application_.handle_data(writer, data);
                }

                template<class Parent>
                void write_message(Parent &parent, std::unique_ptr<endpoint_manager_queue::message> msg) {
                    auto writer = make_packet_writer_decorator(*this, parent);
                    application_.write_message(writer, std::move(msg));
                }

                template<class Parent>
                void resolve(Parent &parent, string_view path, const actor &listener) {
                    auto writer = make_packet_writer_decorator(*this, parent);
                    application_.resolve(writer, path, listener);
                }

                template<class Parent>
                void new_proxy(Parent &parent, const node_id &, actor_id id) {
                    auto writer = make_packet_writer_decorator(*this, parent);
                    application_.new_proxy(writer, id);
                }

                template<class Parent>
                void local_actor_down(Parent &parent, const node_id &, actor_id id, error reason) {
                    auto writer = make_packet_writer_decorator(*this, parent);
                    application_.local_actor_down(writer, id, std::move(reason));
                }

                template<class Parent>
                void timeout(Parent &parent, std::string tag, uint64_t id) {
                    auto writer = make_packet_writer_decorator(*this, parent);
                    application_.timeout(writer, std::move(tag), id);
                }

                void handle_error(sec error) {
                    application_.handle_error(error);
                }

            private:
                application_type application_;
                id_type id_;
            };

            template<class Application, class IdType = unit_t>
            using transport_worker_ptr = std::shared_ptr<transport_worker<Application, IdType>>;

        }    // namespace network
    }        // namespace actor
}    // namespace nil
