#include <nil/actor/network/basp/ec.hpp>

#include <string>

namespace nil {
    namespace actor {
        namespace net {
            namespace basp {

                std::string to_string(ec x) {
                    switch (x) {
                        default:
                            return "???";
                        case ec::invalid_magic_number:
                            return "invalid_magic_number";
                        case ec::unexpected_number_of_bytes:
                            return "unexpected_number_of_bytes";
                        case ec::unexpected_payload:
                            return "unexpected_payload";
                        case ec::missing_payload:
                            return "missing_payload";
                        case ec::illegal_state:
                            return "illegal_state";
                        case ec::invalid_handshake:
                            return "invalid_handshake";
                        case ec::missing_handshake:
                            return "missing_handshake";
                        case ec::unexpected_handshake:
                            return "unexpected_handshake";
                        case ec::version_mismatch:
                            return "version_mismatch";
                        case ec::unimplemented:
                            return "unimplemented";
                        case ec::app_identifiers_mismatch:
                            return "app_identifiers_mismatch";
                        case ec::invalid_payload:
                            return "invalid_payload";
                        case ec::invalid_scheme:
                            return "invalid_scheme";
                        case ec::invalid_locator:
                            return "invalid_locator";
                    };
                }

            }    // namespace basp
        }        // namespace network
    }            // namespace actor
}    // namespace nil