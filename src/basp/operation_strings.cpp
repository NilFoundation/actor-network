#include <nil/actor/network/operation.hpp>

#include <string>

namespace nil {
    namespace actor {
        namespace network {

            std::string to_string(operation x) {
                switch (x) {
                    default:
                        return "???";
                    case operation::none:
                        return "none";
                    case operation::read:
                        return "read";
                    case operation::write:
                        return "write";
                    case operation::read_write:
                        return "read_write";
                    case operation::shutdown:
                        return "shutdown";
                };
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil