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

#include <cstddef>

// -- hard-coded default values for various =nil; Actor options ------------------------

namespace nil::actor::defaults::middleman {

    /// Maximum number of cached buffers for sending payloads.
    extern const size_t max_payload_buffers;

    /// Maximum number of cached buffers for sending headers.
    extern const size_t max_header_buffers;

}    // namespace nil::actor::defaults::middleman