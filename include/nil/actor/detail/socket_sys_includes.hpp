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

// This convenience header pulls in platform-specific headers for the C socket
// API. Do *not* include this header in other headers.

#pragma once

#include <nil/actor/config.hpp>

// clang-format off
#ifdef ACTOR_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif // ACTOR_WINDOWS
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif // NOMINMAX
#  ifdef ACTOR_MINGW
#    undef _WIN32_WINNT
#    undef WINVER
#    define _WIN32_WINNT WindowsVista
#    define WINVER WindowsVista
#    include <w32api.h>
#  endif // ACTOR_MINGW
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2ipdef.h>
#  include <ws2tcpip.h>
#else // ACTOR_WINDOWS
#  include <sys/types.h>
#  include <arpa/inet.h>
#  include <cerrno>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <netinet/ip.h>
#  include <netinet/tcp.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <sys/uio.h>
#endif
// clang-format on
