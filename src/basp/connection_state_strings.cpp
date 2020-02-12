#include <nil/actor/network/basp/connection_state.hpp>

#include <string>

namespace nil {
    namespace actor {
        namespace net {
            namespace basp {

                std::string to_string(connection_state x) {
                    switch (x) {
                        default:
                            return "???";
                        case connection_state::await_handshake_header:
                            return "await_handshake_header";
                        case connection_state::await_handshake_payload:
                            return "await_handshake_payload";
                        case connection_state::await_header:
                            return "await_header";
                        case connection_state::await_payload:
                            return "await_payload";
                        case connection_state::shutdown:
                            return "shutdown";
                    };
                }
            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil
