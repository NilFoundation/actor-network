//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/pipe_socket.hpp>
#include <nil/actor/network/stream_socket.hpp>

#include <cstdio>
#include <utility>

#include <nil/actor/detail/scope_guard.hpp>
#include <nil/actor/detail/socket_sys_aliases.hpp>
#include <nil/actor/detail/socket_sys_includes.hpp>

#include <nil/actor/byte.hpp>
#include <nil/actor/expected.hpp>
#include <nil/actor/message.hpp>
#include <nil/actor/sec.hpp>
#include <nil/actor/span.hpp>
#include <nil/actor/variant.hpp>

namespace nil {
    namespace actor {
        namespace network {

#ifdef BOOST_OS_WINDOWS_AVAILABLE

            expected<std::pair<pipe_socket, pipe_socket>> make_pipe() {
                // Windows has no support for unidirectional pipes. Emulate pipes by using a
                // pair of regular TCP sockets with read/write channels closed appropriately.
                if (auto result = make_stream_socket_pair()) {
                    shutdown_write(result->first);
                    shutdown_read(result->second);
                    return std::make_pair(socket_cast<pipe_socket>(result->first),
                                          socket_cast<pipe_socket>(result->second));
                } else {
                    return std::move(result.error());
                }
            }

            variant<size_t, sec> write(pipe_socket x, span<const byte> buf) {
                // On Windows, a pipe consists of two stream sockets.
                return write(socket_cast<stream_socket>(x), buf);
            }

            variant<size_t, sec> read(pipe_socket x, span<byte> buf) {
                // On Windows, a pipe consists of two stream sockets.
                return read(socket_cast<stream_socket>(x), buf);
            }

#else    // BOOST_OS_WINDOWS_AVAILABLE

            expected<std::pair<pipe_socket, pipe_socket>> make_pipe() {
                socket_id pipefds[2];
                if (pipe(pipefds) != 0)
                    return make_error(sec::network_syscall_failed, "pipe", last_socket_error_as_string());
                auto guard = detail::make_scope_guard([&] {
                    close(socket {pipefds[0]});
                    close(socket {pipefds[1]});
                });
                // Note: for pipe2 it is better to avoid races by setting CLOEXEC (but not on
                // POSIX).
                if (auto err = child_process_inherit(socket {pipefds[0]}, false))
                    return err;
                if (auto err = child_process_inherit(socket {pipefds[1]}, false))
                    return err;
                guard.disable();
                return std::make_pair(pipe_socket {pipefds[0]}, pipe_socket {pipefds[1]});
            }

            variant<size_t, sec> write(pipe_socket x, span<const byte> buf) {
                auto res = ::write(x.id, reinterpret_cast<socket_send_ptr>(buf.data()), buf.size());
                return check_pipe_socket_io_res(res);
            }

            variant<size_t, sec> read(pipe_socket x, span<byte> buf) {
                auto res = ::read(x.id, reinterpret_cast<socket_recv_ptr>(buf.data()), buf.size());
                return check_pipe_socket_io_res(res);
            }

#endif    // BOOST_OS_WINDOWS_AVAILABLE

            variant<size_t, sec> check_pipe_socket_io_res(std::make_signed<size_t>::type res) {
                return check_stream_socket_io_res(res);
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil