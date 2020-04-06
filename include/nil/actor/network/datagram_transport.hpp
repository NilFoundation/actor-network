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

#include <deque>
#include <vector>

#include <nil/actor/fwd.hpp>
#include <nil/actor/ip_endpoint.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/sec.hpp>

#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/fwd.hpp>
#include <nil/actor/network/transport_base.hpp>
#include <nil/actor/network/transport_worker_dispatcher.hpp>
#include <nil/actor/network/udp_datagram_socket.hpp>

namespace nil {
    namespace actor {
        namespace network {

            template<class Factory>
            using datagram_transport_base =
                transport_base<datagram_transport<Factory>, transport_worker_dispatcher<Factory, ip_endpoint>,
                               udp_datagram_socket, Factory, ip_endpoint>;

            /// Implements a udp_transport policy that manages a datagram socket.
            template<class Factory>
            class datagram_transport : public datagram_transport_base<Factory> {
            public:
                // Maximal UDP-packet size
                static constexpr size_t max_datagram_size = std::numeric_limits<uint16_t>::max();

                // -- member types -----------------------------------------------------------

                using factory_type = Factory;

                using id_type = ip_endpoint;

                using application_type = typename factory_type::application_type;

                using super = datagram_transport_base<factory_type>;

                using buffer_type = typename super::buffer_type;

                using buffer_cache_type = typename super::buffer_cache_type;

                // -- constructors, destructors, and assignment operators --------------------

                datagram_transport(udp_datagram_socket handle, factory_type factory) :
                    super(handle, std::move(factory)) {
                    // nop
                }

                // -- public member functions ------------------------------------------------

                error init(endpoint_manager &manager) override {
                    ACTOR_LOG_TRACE("");
                    if (auto err = super::init(manager))
                        return err;
                    prepare_next_read();
                    return none;
                }

                bool handle_read_event(endpoint_manager &) override {
                    ACTOR_LOG_TRACE(ACTOR_ARG(this->handle_.id));
                    for (size_t reads = 0; reads < this->max_consecutive_reads_; ++reads) {
                        auto ret = read(this->handle_, this->read_buf_);
                        if (auto res = get_if<std::pair<size_t, ip_endpoint>>(&ret)) {
                            auto &[num_bytes, ep] = *res;
                            ACTOR_LOG_DEBUG("received " << num_bytes << " bytes");
                            this->read_buf_.resize(num_bytes);
                            this->next_layer_.handle_data(*this, this->read_buf_, std::move(ep));
                            prepare_next_read();
                        } else {
                            auto err = get<sec>(ret);
                            if (err == sec::unavailable_or_would_block) {
                                break;
                            } else {
                                ACTOR_LOG_DEBUG("read failed" << ACTOR_ARG(err));
                                this->next_layer_.handle_error(err);
                                return false;
                            }
                        }
                    }
                    return true;
                }

                bool handle_write_event(endpoint_manager &manager) override {
                    ACTOR_LOG_TRACE(ACTOR_ARG2("handle", this->handle_.id) << ACTOR_ARG2("queue-size", packet_queue_.size()));
                    auto fetch_next_message = [&] {
                        if (auto msg = manager.next_message()) {
                            this->next_layer_.write_message(*this, std::move(msg));
                            return true;
                        }
                        return false;
                    };
                    do {
                        if (auto err = write_some())
                            return err == sec::unavailable_or_would_block;
                    } while (fetch_next_message());
                    return !packet_queue_.empty();
                }

                // TODO: remove this function. `resolve` should add workers when needed.
                error add_new_worker(node_id node, id_type id) {
                    auto worker = this->next_layer_.add_new_worker(*this, node, id);
                    if (!worker)
                        return worker.error();
                    return none;
                }

                void write_packet(id_type id, span<buffer_type *> buffers) override {
                    ACTOR_LOG_TRACE("");
                    ACTOR_ASSERT(!buffers.empty());
                    if (packet_queue_.empty())
                        this->manager().register_writing();
                    // By convention, the first buffer is a header buffer. Every other buffer is
                    // a payload buffer.
                    packet_queue_.emplace_back(id, buffers);
                }

                /// Helper struct for managing outgoing packets
                struct packet {
                    id_type id;
                    buffer_cache_type bytes;
                    size_t size;

                    packet(id_type id, span<buffer_type *> bufs) : id(id) {
                        size = 0;
                        for (auto buf : bufs) {
                            size += buf->size();
                            bytes.emplace_back(std::move(*buf));
                        }
                    }

                    std::vector<std::vector<byte> *> get_buffer_ptrs() {
                        std::vector<std::vector<byte> *> ptrs;
                        for (auto &buf : bytes)
                            ptrs.emplace_back(&buf);
                        return ptrs;
                    }
                };

            private:
                // -- utility functions ------------------------------------------------------

                void prepare_next_read() {
                    this->read_buf_.resize(max_datagram_size);
                }

                error write_some() {
                    // Helper function to sort empty buffers back into the right caches.
                    auto recycle = [&]() {
                        auto &front = packet_queue_.front();
                        auto &bufs = front.bytes;
                        auto it = bufs.begin();
                        if (this->header_bufs_.size() < this->header_bufs_.capacity()) {
                            it->clear();
                            this->header_bufs_.emplace_back(std::move(*it++));
                        }
                        for (; it != bufs.end() && this->payload_bufs_.size() < this->payload_bufs_.capacity(); ++it) {
                            it->clear();
                            this->payload_bufs_.emplace_back(std::move(*it));
                        }
                        packet_queue_.pop_front();
                    };
                    // Write as many bytes as possible.
                    while (!packet_queue_.empty()) {
                        auto &packet = packet_queue_.front();
                        auto ptrs = packet.get_buffer_ptrs();
                        auto write_ret = write(this->handle_, ptrs, packet.id);
                        if (auto num_bytes = get_if<size_t>(&write_ret)) {
                            ACTOR_LOG_DEBUG(ACTOR_ARG(this->handle_.id) << ACTOR_ARG(*num_bytes));
                            ACTOR_LOG_WARNING_IF(*num_bytes < packet.size, "packet was not sent completely");
                            recycle();
                        } else {
                            auto err = get<sec>(write_ret);
                            if (err != sec::unavailable_or_would_block) {
                                ACTOR_LOG_ERROR("write failed" << ACTOR_ARG(err));
                                this->next_layer_.handle_error(err);
                            }
                            return err;
                        }
                    }
                    return none;
                }

                std::deque<packet> packet_queue_;
            };

        }    // namespace network
    }        // namespace actor
}    // namespace nil