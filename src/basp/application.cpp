//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/basp/application.hpp>
#include <nil/actor/network/basp/constants.hpp>
#include <nil/actor/network/basp/ec.hpp>

#include <nil/actor/network/packet_writer.hpp>

#include <vector>

#include <nil/actor/detail/network_order.hpp>
#include <nil/actor/detail/parse.hpp>

#include <nil/actor/spawner.hpp>
#include <nil/actor/spawner_config.hpp>
#include <nil/actor/binary_deserializer.hpp>
#include <nil/actor/binary_serializer.hpp>
#include <nil/actor/byte.hpp>
#include <nil/actor/defaults.hpp>
#include <nil/actor/error.hpp>
#include <nil/actor/expected.hpp>
#include <nil/actor/logger.hpp>
#include <nil/actor/no_stages.hpp>
#include <nil/actor/none.hpp>
#include <nil/actor/sec.hpp>
#include <nil/actor/send.hpp>
#include <nil/actor/string_algorithms.hpp>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                application::application(proxy_registry &proxies) :
                    proxies_(proxies), queue_ {new message_queue}, hub_ {new hub_type} {
                    // nop
                }

                error application::write_message(packet_writer &writer,
                                                 std::unique_ptr<endpoint_manager_queue::message>
                                                     ptr) {
                    ACTOR_ASSERT(ptr != nullptr);
                    ACTOR_ASSERT(ptr->msg != nullptr);
                    ACTOR_LOG_TRACE(ACTOR_ARG2("content", ptr->msg->content()));
                    const auto &src = ptr->msg->sender;
                    const auto &dst = ptr->receiver;
                    if (dst == nullptr) {
                        // TODO: valid?
                        return none;
                    }
                    auto payload_buf = writer.next_payload_buffer();
                    binary_serializer sink {system(), payload_buf};
                    if (src != nullptr) {
                        auto src_id = src->id();
                        system().registry().put(src_id, src);
                        if (auto err = sink(src->node(), src_id, dst->id(), ptr->msg->stages))
                            return err;
                    } else {
                        if (auto err = sink(node_id {}, actor_id {0}, dst->id(), ptr->msg->stages))
                            return err;
                    }
                    if (auto err = sink(ptr->msg->content()))
                        return err;
                    auto hdr = writer.next_header_buffer();
                    to_bytes(header {message_type::actor_message, static_cast<uint32_t>(payload_buf.size()),
                                     ptr->msg->mid.integer_value()},
                             hdr);
                    writer.write_packet(hdr, payload_buf);
                    return none;
                }

                void application::resolve(packet_writer &writer, string_view path, const actor &listener) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(path) << ACTOR_ARG(listener));
                    auto payload = writer.next_payload_buffer();
                    binary_serializer sink {&executor_, payload};
                    if (auto err = sink(path)) {
                        ACTOR_LOG_ERROR("unable to serialize path" << ACTOR_ARG(err));
                        return;
                    }
                    auto req_id = next_request_id_++;
                    auto hdr = writer.next_header_buffer();
                    to_bytes(header {message_type::resolve_request, static_cast<uint32_t>(payload.size()), req_id},
                             hdr);
                    writer.write_packet(hdr, payload);
                    pending_resolves_.emplace(req_id, listener);
                }

                void application::new_proxy(packet_writer &writer, actor_id id) {
                    auto hdr = writer.next_header_buffer();
                    to_bytes(header {message_type::monitor_message, 0, static_cast<uint64_t>(id)}, hdr);
                    writer.write_packet(hdr);
                }

                void application::local_actor_down(packet_writer &writer, actor_id id, error reason) {
                    auto payload = writer.next_payload_buffer();
                    binary_serializer sink {system(), payload};
                    if (auto err = sink(reason))
                        ACTOR_RAISE_ERROR("unable to serialize an error");
                    auto hdr = writer.next_header_buffer();
                    to_bytes(header {message_type::down_message, static_cast<uint32_t>(payload.size()),
                                     static_cast<uint64_t>(id)},
                             hdr);
                    writer.write_packet(hdr, payload);
                }

                strong_actor_ptr application::resolve_local_path(string_view path) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(path));
                    // We currently support two path formats: `id/<actor_id>` and `name/<atom>`.
                    static constexpr string_view id_prefix = "id/";
                    if (starts_with(path, id_prefix)) {
                        path.remove_prefix(id_prefix.size());
                        actor_id aid;
                        if (auto err = detail::parse(path, aid))
                            return nullptr;
                        return system().registry().get(aid);
                    }
                    static constexpr string_view name_prefix = "name/";
                    if (starts_with(path, name_prefix)) {
                        path.remove_prefix(name_prefix.size());
                        std::string name;
                        if (auto err = detail::parse(path, name))
                            return nullptr;
                        return system().registry().get(name);
                    }
                    return nullptr;
                }

                error application::handle(size_t &next_read_size, packet_writer &writer, byte_span bytes) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(state_) << ACTOR_ARG2("bytes.size", bytes.size()));
                    switch (state_) {
                        case connection_state::await_handshake_header: {
                            if (bytes.size() != header_size)
                                return ec::unexpected_number_of_bytes;
                            hdr_ = header::from_bytes(bytes);
                            if (hdr_.type != message_type::handshake)
                                return ec::missing_handshake;
                            if (hdr_.operation_data != version)
                                return ec::version_mismatch;
                            if (hdr_.payload_len == 0)
                                return ec::missing_payload;
                            state_ = connection_state::await_handshake_payload;
                            next_read_size = hdr_.payload_len;
                            return none;
                        }
                        case connection_state::await_handshake_payload: {
                            if (auto err = handle_handshake(writer, hdr_, bytes))
                                return err;
                            state_ = connection_state::await_header;
                            return none;
                        }
                        case connection_state::await_header: {
                            if (bytes.size() != header_size)
                                return ec::unexpected_number_of_bytes;
                            hdr_ = header::from_bytes(bytes);
                            if (hdr_.payload_len == 0)
                                return handle(writer, hdr_, byte_span {});
                            next_read_size = hdr_.payload_len;
                            state_ = connection_state::await_payload;
                            return none;
                        }
                        case connection_state::await_payload: {
                            if (bytes.size() != hdr_.payload_len)
                                return ec::unexpected_number_of_bytes;
                            state_ = connection_state::await_header;
                            return handle(writer, hdr_, bytes);
                        }
                        default:
                            return ec::illegal_state;
                    }
                }

                error application::handle(packet_writer &writer, header hdr, byte_span payload) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(hdr) << ACTOR_ARG2("payload.size", payload.size()));
                    switch (hdr.type) {
                        case message_type::handshake:
                            return ec::unexpected_handshake;
                        case message_type::actor_message:
                            return handle_actor_message(writer, hdr, payload);
                        case message_type::resolve_request:
                            return handle_resolve_request(writer, hdr, payload);
                        case message_type::resolve_response:
                            return handle_resolve_response(writer, hdr, payload);
                        case message_type::monitor_message:
                            return handle_monitor_message(writer, hdr, payload);
                        case message_type::down_message:
                            return handle_down_message(writer, hdr, payload);
                        case message_type::heartbeat:
                            return none;
                        default:
                            return ec::unimplemented;
                    }
                }

                error application::handle_handshake(packet_writer &, header hdr, byte_span payload) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(hdr) << ACTOR_ARG2("payload.size", payload.size()));
                    if (hdr.type != message_type::handshake)
                        return ec::missing_handshake;
                    if (hdr.operation_data != version)
                        return ec::version_mismatch;
                    node_id peer_id;
                    std::vector<std::string> app_ids;
                    binary_deserializer source {&executor_, payload};
                    if (auto err = source(peer_id, app_ids))
                        return err;
                    if (!peer_id || app_ids.empty())
                        return ec::invalid_handshake;
                    auto ids =
                        get_or(system().config(), "middleman.app-identifiers", defaults::middleman::app_identifiers);
                    auto predicate = [=](const std::string &x) {
                        return std::find(ids.begin(), ids.end(), x) != ids.end();
                    };
                    if (std::none_of(app_ids.begin(), app_ids.end(), predicate))
                        return ec::app_identifiers_mismatch;
                    peer_id_ = std::move(peer_id);
                    state_ = connection_state::await_header;
                    return none;
                }

                error application::handle_actor_message(packet_writer &, header hdr, byte_span payload) {
                    auto worker = hub_->pop();
                    if (worker != nullptr) {
                        ACTOR_LOG_DEBUG("launch BASP worker for deserializing an actor_message");
                        worker->launch(node_id {}, hdr, payload);
                    } else {
                        ACTOR_LOG_DEBUG("out of BASP workers, continue deserializing an actor_message");
                        // If no worker is available then we have no other choice than to take
                        // the performance hit and deserialize in this thread.
                        struct handler : remote_message_handler<handler> {
                            handler(message_queue *queue, proxy_registry *proxies, spawner *system, node_id last_hop,
                                    basp::header &hdr, byte_span payload) :
                                queue_(queue),
                                proxies_(proxies), system_(system), last_hop_(std::move(last_hop)), hdr_(hdr),
                                payload_(payload) {
                                msg_id_ = queue_->new_id();
                            }
                            message_queue *queue_;
                            proxy_registry *proxies_;
                            spawner *system_;
                            node_id last_hop_;
                            basp::header &hdr_;
                            byte_span payload_;
                            uint64_t msg_id_;
                        };
                        handler f {queue_.get(), &proxies_, system_, node_id {}, hdr, payload};
                        f.handle_remote_message(&executor_);
                    }
                    return none;
                }

                error application::handle_resolve_request(packet_writer &writer, header rec_hdr, byte_span received) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(rec_hdr) << ACTOR_ARG2("received.size", received.size()));
                    ACTOR_ASSERT(rec_hdr.type == message_type::resolve_request);
                    size_t path_size = 0;
                    binary_deserializer source {&executor_, received};
                    if (auto err = source.begin_sequence(path_size))
                        return err;
                    // We expect the received buffer to contain the path only.
                    if (path_size != source.remaining())
                        return ec::invalid_payload;
                    auto remainder = source.remainder();
                    string_view path {reinterpret_cast<const char *>(remainder.data()), remainder.size()};
                    // Write result.
                    auto result = resolve_local_path(path);
                    actor_id aid;
                    std::set<std::string> ifs;
                    if (result) {
                        aid = result->id();
                        system().registry().put(aid, result);
                    } else {
                        aid = 0;
                    }
                    // TODO: figure out how to obtain messaging interface.
                    auto payload = writer.next_payload_buffer();
                    binary_serializer sink {&executor_, payload};
                    if (auto err = sink(aid, ifs))
                        return err;
                    auto hdr = writer.next_header_buffer();
                    to_bytes(header {message_type::resolve_response, static_cast<uint32_t>(payload.size()),
                                     rec_hdr.operation_data},
                             hdr);
                    writer.write_packet(hdr, payload);
                    return none;
                }

                error application::handle_resolve_response(packet_writer &, header received_hdr, byte_span received) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(received_hdr) << ACTOR_ARG2("received.size", received.size()));
                    ACTOR_ASSERT(received_hdr.type == message_type::resolve_response);
                    auto i = pending_resolves_.find(received_hdr.operation_data);
                    if (i == pending_resolves_.end()) {
                        ACTOR_LOG_ERROR("received unknown ID in resolve_response message");
                        return none;
                    }
                    auto guard = detail::make_scope_guard([&] { pending_resolves_.erase(i); });
                    actor_id aid;
                    std::set<std::string> ifs;
                    binary_deserializer source {&executor_, received};
                    if (auto err = source(aid, ifs)) {
                        anon_send(i->second, sec::remote_lookup_failed);
                        return err;
                    }
                    if (aid == 0) {
                        anon_send(i->second, strong_actor_ptr {nullptr}, std::move(ifs));
                        return none;
                    }
                    anon_send(i->second, proxies_.get_or_put(peer_id_, aid), std::move(ifs));
                    return none;
                }

                error application::handle_monitor_message(packet_writer &writer,
                                                          header received_hdr,
                                                          byte_span received) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(received_hdr) << ACTOR_ARG2("received.size", received.size()));
                    if (!received.empty())
                        return ec::unexpected_payload;
                    auto aid = static_cast<actor_id>(received_hdr.operation_data);
                    auto hdl = system().registry().get(aid);
                    if (hdl != nullptr) {
                        endpoint_manager_ptr mgr = manager_;
                        auto nid = peer_id_;
                        hdl->get()->attach_functor([mgr, nid, aid](error reason) mutable {
                            mgr->enqueue_event(std::move(nid), aid, std::move(reason));
                        });
                    } else {
                        error reason = exit_reason::unknown;
                        auto payload = writer.next_payload_buffer();
                        binary_serializer sink {&executor_, payload};
                        if (auto err = sink(reason))
                            return err;
                        auto hdr = writer.next_header_buffer();
                        to_bytes(header {message_type::down_message, static_cast<uint32_t>(payload.size()),
                                         received_hdr.operation_data},
                                 hdr);
                        writer.write_packet(hdr, payload);
                    }
                    return none;
                }

                error application::handle_down_message(packet_writer &, header received_hdr, byte_span received) {
                    ACTOR_LOG_TRACE(ACTOR_ARG(received_hdr) << ACTOR_ARG2("received.size", received.size()));
                    error reason;
                    binary_deserializer source {&executor_, received};
                    if (auto err = source(reason))
                        return err;
                    proxies_.erase(peer_id_, received_hdr.operation_data, std::move(reason));
                    return none;
                }

                error application::generate_handshake(std::vector<byte> &buf) {
                    binary_serializer sink {&executor_, buf};
                    return sink(
                        system().node(),
                        get_or(system().config(), "middleman.app-identifiers", defaults::middleman::app_identifiers));
                }

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil