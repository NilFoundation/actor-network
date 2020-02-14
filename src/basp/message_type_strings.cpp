#include <nil/actor/network/basp/message_type.hpp>

#include <string>

namespace nil {
    namespace actor {
        namespace network {
            namespace basp {

                std::string to_string(message_type x) {
                    switch (x) {
                        default:
                            return "???";
                        case message_type::handshake:
                            return "handshake";
                        case message_type::actor_message:
                            return "actor_message";
                        case message_type::resolve_request:
                            return "resolve_request";
                        case message_type::resolve_response:
                            return "resolve_response";
                        case message_type::monitor_message:
                            return "monitor_message";
                        case message_type::down_message:
                            return "down_message";
                        case message_type::heartbeat:
                            return "heartbeat";
                    };
                }

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil