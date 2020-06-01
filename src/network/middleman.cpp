//---------------------------------------------------------------------------//
// Copyright (c) 2011-2019 Dominik Charousset
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the terms and conditions of the BSD 3-Clause License or
// (at your option) under the terms and conditions of the Boost Software
// License 1.0. See accompanying files LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt.
//---------------------------------------------------------------------------//

#include <nil/actor/network/middleman.hpp>

#include <nil/actor/spawner_config.hpp>
#include <nil/actor/detail/set_thread_name.hpp>
#include <nil/actor/network/basp/ec.hpp>
#include <nil/actor/network/middleman_backend.hpp>
#include <nil/actor/network/multiplexer.hpp>
#include <nil/actor/raise_error.hpp>
#include <nil/actor/send.hpp>
#include <nil/actor/uri.hpp>

namespace nil {
    namespace actor {
        namespace network {

            middleman::middleman(spawner &sys) : sys_(sys) {
                mpx_ = std::make_shared<multiplexer>();
            }

            middleman::~middleman() {
                // nop
            }

            void middleman::start() {
                if (!get_or(config(), "middleman.manual-multiplexing", false)) {
                    auto mpx = mpx_;
                    auto *sys_ptr = &system();
                    mpx_thread_ = std::thread {[mpx, sys_ptr] {
                        ACTOR_SET_LOGGER_SYS(sys_ptr);
                        detail::set_thread_name("caf.multiplexer");
                        sys_ptr->thread_started();
                        mpx->set_thread_id();
                        mpx->run();
                        sys_ptr->thread_terminates();
                    }};
                }
            }

            void middleman::stop() {
                for (const auto &backend : backends_)
                    backend->stop();
                mpx_->shutdown();
                if (mpx_thread_.joinable())
                    mpx_thread_.join();
                else
                    mpx_->run();
            }

            void middleman::init(spawner_config &cfg) {
                if (auto err = mpx_->init()) {
                    ACTOR_LOG_ERROR("mgr->init() failed: " << system().render(err));
                    ACTOR_RAISE_ERROR("mpx->init() failed");
                }
                if (const auto *node_uri = get_if<uri>(&cfg, "middleman.this-node")) {
                    auto this_node = make_node_id(*node_uri);
                    sys_.node_.swap(this_node);
                } else {
                    ACTOR_RAISE_ERROR("no valid entry for middleman.this-node found");
                }
                for (auto &backend : backends_)
                    if (auto err = backend->init()) {
                        ACTOR_LOG_ERROR("failed to initialize backend: " << system().render(err));
                        ACTOR_RAISE_ERROR("failed to initialize backend");
                    }
            }

            middleman::module::id_t middleman::id() const {
                return module::network_manager;
            }

            void *middleman::subtype_ptr() {
                return this;
            }

            void middleman::resolve(const uri &locator, const actor &listener) {
                auto *ptr = backend(locator.scheme());
                if (ptr != nullptr)
                    ptr->resolve(locator, listener);
                else
                    anon_send(listener, error {basp::ec::invalid_scheme});
            }

            middleman_backend *middleman::backend(string_view scheme) const noexcept {
                auto predicate = [&](const middleman_backend_ptr &ptr) { return ptr->id() == scheme; };
                auto i = std::find_if(backends_.begin(), backends_.end(), predicate);
                if (i != backends_.end())
                    return i->get();
                return nullptr;
            }

        }    // namespace network
    }        // namespace actor
}    // namespace nil