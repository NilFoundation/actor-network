//---------------------------------------------------------------------------//
// Copyright (c) 2011-2018 Dominik Charousset
// Copyright (c) 2018-2020 Nil Foundation AG
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#pragma once

#include <cstdio>
#include <cstdlib>

#include <nil/actor/error.hpp>
#include <nil/actor/sec.hpp>

/// Calls a C functions and returns an error if `var op rhs` returns `true`.
#define ACTOR_NET_SYSCALL(funname, var, op, rhs, expr) \
    auto var = expr;                                   \
    if (var op rhs)                                    \
    return make_error(sec::network_syscall_failed, funname, last_socket_error_as_string())

/// Calls a C functions and calls exit() if `var op rhs` returns `true`.
#define ACTOR_NET_CRITICAL_SYSCALL(funname, var, op, rhs, expr)                                         \
    auto var = expr;                                                                                    \
    if (var op rhs) {                                                                                   \
        fprintf(stderr, "[FATAL] %s:%u: syscall failed: %s returned %s\n", __FILE__, __LINE__, funname, \
                last_socket_error_as_string().c_str());                                                 \
        abort();                                                                                        \
    }                                                                                                   \
    static_cast<void>(0)
