//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#pragma once

#include <vector>

#include <nil/actor/fwd.hpp>
#include <nil/actor/error.hpp>
#include <nil/actor/stream.hpp>
#include <nil/actor/type_id.hpp>

#include <nil/actor/network/host.hpp>

ACTOR_BEGIN_TYPE_ID_BLOCK(incubator_test, nil::actor::first_custom_type_id)

ACTOR_ADD_TYPE_ID(incubator_test, (nil::actor::stream<int>))
ACTOR_ADD_TYPE_ID(incubator_test, (std::vector<int>))

ACTOR_END_TYPE_ID_BLOCK(incubator_test)

struct host_fixture {
    host_fixture() {
        nil::actor::init_global_meta_objects<nil::actor::id_block::incubator_test>();
        nil::actor::init_global_meta_objects<nil::actor::id_block::core_module>();

        if (auto err = nil::actor::network::this_host::startup())
            throw std::logic_error("this_host::startup failed");
    }

    ~host_fixture() {
        nil::actor::network::this_host::cleanup();
    }
};