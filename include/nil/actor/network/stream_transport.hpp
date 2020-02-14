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

#include <deque>
#include <vector>

#include <nil/actor/fwd.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/network/endpoint_manager.hpp>
#include <nil/actor/network/fwd.hpp>
#include <nil/actor/network/receive_policy.hpp>
#include <nil/actor/network/stream_socket.hpp>
#include <nil/actor/network/transport_base.hpp>
#include <nil/actor/network/transport_worker.hpp>
#include <nil/actor/sec.hpp>
#include <nil/actor/span.hpp>

namespace nil {
    namespace actor {
        namespace network {

            template<class Application>
            using stream_transport_base = transport_base<stream_transport<Application>, transport_worker<Application>,
                                                         stream_socket, Application, unit_t>;

            /// Implements a stream_transport that manages a stream socket.
            template<class Application>
            class stream_transport : public stream_transport_base<Application> {
            public:
                // -- member types -----------------------------------------------------------

                using application_type = Application;

                using worker_type = transport_worker<application_type>;

                using super = stream_transport_base<application_type>;

                using id_type = typename super::id_type;

                using buffer_type = typename super::buffer_type;

                using write_queue_type = std::deque<std::pair<bool, buffer_type>>;

                // -- constructors, destructors, and assignment operators --------------------

                stream_transport(stream_socket handle, application_type application) :
                    super(handle, std::move(application)), written_(0), read_threshold_(1024), collected_(0),
                    max_(1024), rd_flag_(network::receive_policy_flag::exactly) {
                    ACTOR_ASSERT(handle != invalid_socket);
                    nodelay(handle, true);
                }

                // -- member functions -------------------------------------------------------

                bool handle_read_event(endpoint_manager &) override {
                    ACTOR_LOG_TRACE(ACTOR_ARG2("handle", this->handle().id));
                    for (size_t reads = 0; reads < this->max_consecutive_reads_; ++reads) {
                        auto buf = this->read_buf_.data() + this->collected_;
                        size_t len = this->read_threshold_ - this->collected_;
                        ACTOR_LOG_DEBUG(ACTOR_ARG2("missing", len));
                        auto ret = read(this->handle_, make_span(buf, len));
                        // Update state.
                        if (auto num_bytes = get_if<size_t>(&ret)) {
                            ACTOR_LOG_DEBUG(ACTOR_ARG(len) << ACTOR_ARG(this->handle_.id) << ACTOR_ARG(*num_bytes));
                            this->collected_ += *num_bytes;
                            if (this->collected_ >= this->read_threshold_) {
                                if (auto err = this->next_layer_.handle_data(*this, make_span(this->read_buf_))) {
                                    ACTOR_LOG_ERROR("handle_data failed: " << ACTOR_ARG(err));
                                    return false;
                                }
                                this->prepare_next_read();
                            }
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
                    ACTOR_LOG_TRACE(ACTOR_ARG2("handle", this->handle_.id)
                                    << ACTOR_ARG2("queue-size", write_queue_.size()));
                    auto drain_write_queue = [this]() -> error_code<sec> {
                        // Helper function to sort empty buffers back into the right caches.
                        auto recycle = [this]() {
                            auto &front = this->write_queue_.front();
                            auto &is_header = front.first;
                            auto &buf = front.second;
                            written_ = 0;
                            buf.clear();
                            if (is_header) {
                                if (this->header_bufs_.size() < this->header_bufs_.capacity())
                                    this->header_bufs_.emplace_back(std::move(buf));
                            } else if (this->payload_bufs_.size() < this->payload_bufs_.capacity()) {
                                this->payload_bufs_.emplace_back(std::move(buf));
                            }
                            write_queue_.pop_front();
                        };
                        // Write buffers from the write_queue_ for as long as possible.
                        while (!write_queue_.empty()) {
                            auto &buf = write_queue_.front().second;
                            ACTOR_ASSERT(!buf.empty());
                            auto data = buf.data() + written_;
                            auto len = buf.size() - written_;
                            auto write_ret = write(this->handle(), make_span(data, len));
                            if (auto num_bytes = get_if<size_t>(&write_ret)) {
                                ACTOR_LOG_DEBUG(ACTOR_ARG(this->handle_.id) << ACTOR_ARG(*num_bytes));
                                written_ += *num_bytes;
                                if (written_ >= buf.size()) {
                                    recycle();
                                    written_ = 0;
                                }
                            } else {
                                auto err = get<sec>(write_ret);
                                if (err != sec::unavailable_or_would_block) {
                                    ACTOR_LOG_DEBUG("send failed" << ACTOR_ARG(err));
                                    this->next_layer_.handle_error(err);
                                }
                                return err;
                            }
                        }
                        return none;
                    };
                    auto fetch_next_message = [&] {
                        if (auto msg = manager.next_message()) {
                            this->next_layer_.write_message(*this, std::move(msg));
                            return true;
                        }
                        return false;
                    };
                    do {
                        if (auto err = drain_write_queue())
                            return err == sec::unavailable_or_would_block;
                    } while (fetch_next_message());
                    ACTOR_ASSERT(write_queue_.empty());
                    return false;
                }

                void write_packet(id_type, span<buffer_type *> buffers) override {
                    ACTOR_LOG_TRACE("");
                    ACTOR_ASSERT(!buffers.empty());
                    if (this->write_queue_.empty())
                        this->manager().register_writing();
                    // By convention, the first buffer is a header buffer. Every other buffer is
                    // a payload buffer.
                    auto i = buffers.begin();
                    this->write_queue_.emplace_back(true, std::move(*(*i++)));
                    while (i != buffers.end())
                        this->write_queue_.emplace_back(false, std::move(*(*i++)));
                }

                void configure_read(receive_policy::config cfg) override {
                    rd_flag_ = cfg.first;
                    max_ = cfg.second;
                    prepare_next_read();
                }

            private:
                // -- utility functions ------------------------------------------------------

                void prepare_next_read() {
                    collected_ = 0;
                    switch (rd_flag_) {
                        case network::receive_policy_flag::exactly:
                            if (this->read_buf_.size() != max_)
                                this->read_buf_.resize(max_);
                            read_threshold_ = max_;
                            break;
                        case network::receive_policy_flag::at_most:
                            if (this->read_buf_.size() != max_)
                                this->read_buf_.resize(max_);
                            read_threshold_ = 1;
                            break;
                        case network::receive_policy_flag::at_least: {
                            // read up to 10% more, but at least allow 100 bytes more
                            auto max_size = max_ + std::max<size_t>(100, max_ / 10);
                            if (this->read_buf_.size() != max_size)
                                this->read_buf_.resize(max_size);
                            read_threshold_ = max_;
                            break;
                        }
                    }
                }

                write_queue_type write_queue_;
                size_t written_;
                size_t read_threshold_;
                size_t collected_;
                size_t max_;
                receive_policy_flag rd_flag_;
            };
        }    // namespace network
    }        // namespace actor
}    // namespace nil
