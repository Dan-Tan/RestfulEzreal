#include "BatchRequests.h"
#include "db/DatabaseManager.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "simdjson.h"

#ifdef DEBUG_MODE
#define D(x) std::cerr << x << '\n'
#else
#define D(x)  
#endif

namespace restfulEz {

    // =========================================================================
    // VALUE POOL HELPERS
    // =========================================================================

    std::function<bool(const std::string&)> ValuePool::SkipExistingPlayersInDB() {
        return [](const std::string& puuid) -> bool {
            if (!DatabaseManager::GetInstance().IsInitialized()) return true;
            return !DatabaseManager::GetInstance().HasPlayerPuuid(puuid);
        };
    }

    std::function<bool(const std::string&)> ValuePool::SkipExistingMatchesInDB() {
        return [](const std::string& match_id) -> bool {
            if (!DatabaseManager::GetInstance().IsInitialized()) return true;
            return !DatabaseManager::GetInstance().HasMatch(match_id);
        };
    }

    std::function<bool(const std::string&)> ValuePool::SkipExistingTimelinesInDB() {
        return [](const std::string& match_id) -> bool {
            if (!DatabaseManager::GetInstance().IsInitialized()) return true;
            return !DatabaseManager::GetInstance().HasTimeline(match_id);
        };
    }

    std::shared_ptr<ValuePool> ValuePool::FromDelimitedString(const std::string& input, char delimiter, bool deduplicate) {
        auto pool = std::make_shared<ValuePool>(deduplicate);
        std::stringstream ss(input);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            size_t first = token.find_first_not_of(" \t\n\r\"'");
            if (first == std::string::npos) continue;
            size_t last = token.find_last_not_of(" \t\n\r\"'");
            std::string cleaned = token.substr(first, last - first + 1);
            if (!cleaned.empty()) {
                pool->push(cleaned);
            }
        }
        return pool;
    }

    // =========================================================================
    // UNIFIED RECURSIVE JSON ELEMENT -> VALUE POOL EXTRACTOR
    // =========================================================================

    static bool element_to_param(simdjson::dom::element elem, PARAM_CONT& out) {
        std::string_view sv;
        if (elem.get(sv) == simdjson::SUCCESS) {
            size_t copy_len = std::min(sv.length(), sizeof(out.param) - 1);
            memcpy(out.param, sv.data(), copy_len);
            out.param[copy_len] = '\0';
            return true;
        }
        int64_t val_int = 0;
        if (elem.get(val_int) == simdjson::SUCCESS) {
            std::string s = std::to_string(val_int);
            size_t copy_len = std::min(s.length(), sizeof(out.param) - 1);
            memcpy(out.param, s.data(), copy_len);
            out.param[copy_len] = '\0';
            return true;
        }
        uint64_t val_uint = 0;
        if (elem.get(val_uint) == simdjson::SUCCESS) {
            std::string s = std::to_string(val_uint);
            size_t copy_len = std::min(s.length(), sizeof(out.param) - 1);
            memcpy(out.param, s.data(), copy_len);
            out.param[copy_len] = '\0';
            return true;
        }
        bool val_bool = false;
        if (elem.get(val_bool) == simdjson::SUCCESS) {
            out = val_bool ? "true" : "false";
            return true;
        }
        return false;
    }

    static void extract_elements_recursive(
        simdjson::dom::element elem,
        const std::vector<KEY_CONT>& all_keys,
        std::size_t key_idx,
        ValuePool& pool,
        std::size_t per_response_limit,
        std::size_t& response_count
    ) {
        if (per_response_limit > 0 && response_count >= per_response_limit) {
            return;
        }

        // Skip empty keys
        while (key_idx < all_keys.size() && all_keys[key_idx].key[0] == '\0') {
            key_idx++;
        }

        // If current element is an array, iterate through items and apply remaining keys
        simdjson::dom::array arr;
        if (elem.get(arr) == simdjson::SUCCESS) {
            for (simdjson::dom::element item : arr) {
                if (per_response_limit > 0 && response_count >= per_response_limit) break;
                extract_elements_recursive(item, all_keys, key_idx, pool, per_response_limit, response_count);
            }
            return;
        }

        // If keys remain, traverse object field
        if (key_idx < all_keys.size()) {
            simdjson::dom::element child;
            if (elem[all_keys[key_idx].key].get(child) == simdjson::SUCCESS) {
                extract_elements_recursive(child, all_keys, key_idx + 1, pool, per_response_limit, response_count);
            }
            return;
        }

        // Base case: all keys consumed and element is scalar
        PARAM_CONT extracted;
        if (element_to_param(elem, extracted)) {
            if (pool.push(extracted)) {
                response_count++;
            }
        }
    }

    static void parse_and_extract(const raw_json& response, const std::vector<KEY_CONT>& all_keys, ValuePool& pool, std::size_t per_response_limit) {
        if (response.empty()) return;

        size_t valid_len = response.size();
        while (valid_len > 0 && response[valid_len - 1] == '\0') {
            valid_len--;
        }
        if (valid_len == 0) return;

        try {
            simdjson::padded_string padded(response.data(), valid_len);
            simdjson::dom::parser parser;
            simdjson::dom::element elem;
            if (parser.parse(padded).get(elem) != simdjson::SUCCESS) {
                return;
            }
            std::size_t count = 0;
            extract_elements_recursive(elem, all_keys, 0, pool, per_response_limit, count);
        } catch (const std::exception& ex) {
            std::cerr << "[ValuePool] JSON parsing exception: " << ex.what() << std::endl;
        }
    }

    PARAM_CONT json_access_info::get_param(const raw_json& response) const {
        ValuePool temp_pool(false, 1);
        extract_into_pool(response, temp_pool, 1);
        if (temp_pool.empty()) return PARAM_CONT();
        return temp_pool[0];
    }

    void json_access_info::extract_into_pool(const raw_json& response, ValuePool& pool, std::size_t per_response_limit) const {
        parse_and_extract(response, this->keys, pool, per_response_limit);
    }

    void json_access_info::extract_into_pool(const std::vector<json_ptr>& responses, ValuePool& pool, std::size_t per_response_limit) const {
        for (const auto& resp : responses) {
            if (resp) {
                parse_and_extract(*resp, this->keys, pool, per_response_limit);
            }
        }
    }

    std::vector<KEY_CONT> iter_access_info::combined_keys() const {
        std::vector<KEY_CONT> all_keys;
        all_keys.reserve(this->keys.size() + this->access_after_iter.keys.size());
        for (const auto& k : this->keys) {
            if (k.key[0] != '\0') all_keys.push_back(k);
        }
        for (const auto& k : this->access_after_iter.keys) {
            if (k.key[0] != '\0') all_keys.push_back(k);
        }
        return all_keys;
    }

    void iter_access_info::extract_into_pool(const raw_json& response, ValuePool& pool) const {
        parse_and_extract(response, combined_keys(), pool, this->iter_limit);
    }

    void iter_access_info::extract_into_pool(const std::vector<json_ptr>& responses, ValuePool& pool) const {
        std::vector<KEY_CONT> all_keys = combined_keys();
        for (const auto& resp : responses) {
            if (resp) {
                parse_and_extract(*resp, all_keys, pool, this->iter_limit);
            }
        }
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(const raw_json& response) const {
        ValuePool temp_pool(false);
        extract_into_pool(response, temp_pool);
        return temp_pool.values();
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(const std::vector<json_ptr>& responses) const {
        ValuePool temp_pool(false);
        extract_into_pool(responses, temp_pool);
        return temp_pool.values();
    }

    json_access_info iter_access_info::get_base() const {
        return json_access_info{this->keys};
    }

    // =========================================================================
    // LINK IMPLEMENTATIONS
    // =========================================================================

    bool request_link::get_dependencies(std::vector<PARAM_CONT>& to_fill) {
        if (!this->parent || !this->parent->_node || this->parent->_node->request_results.empty() || !this->parent->_node->request_results[0]) {
            return false;
        }
        try {
            std::size_t index = 0;
            for (const json_access_info& info : this->link_info) {
                if (index < this->param_indices.size() && this->param_indices[index] < to_fill.size()) {
                    to_fill[this->param_indices[index]] = info.get_param(*this->parent->_node->request_results[0]);
                }
                index++;
            }
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "[request_link] Error getting dependencies: " << ex.what() << std::endl;
            return false;
        }
    }

    bool request_link::get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) {
        if (!this->parent || !this->parent->_node || this->parent->_node->request_results.empty()) {
            return false;
        }
        try {
            for (const json_access_info& noniter_link : this->link_info) {
                ValuePool pool(false);
                noniter_link.extract_into_pool(this->parent->_node->request_results, pool, 1);
                to_fill.push_back(pool.values());
            }
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "[request_link] Error getting multi-result dependencies: " << ex.what() << std::endl;
            return false;
        }
    }

    bool request_link::populate_pools(std::vector<std::shared_ptr<ValuePool>>& pools, const std::vector<std::size_t>& fallback_indices, std::size_t& fallback_cursor) {
        if (!this->parent || !this->parent->_node) {
            return false;
        }
        const auto& results = this->parent->_node->request_results;
        // Ordering-only barrier link (no field extraction)
        if (this->link_info.empty() && this->param_indices.empty()) {
            this->consumed_results = results.size();
            return !results.empty();
        }
        if (results.empty()) {
            return false;
        }

        std::size_t start_idx = this->consumed_results;
        for (std::size_t i = 0; i < this->link_info.size(); ++i) {
            std::size_t target_idx = 0;
            bool has_idx = false;
            if (i < this->param_indices.size()) {
                target_idx = this->param_indices[i];
                has_idx = true;
            } else if (fallback_cursor < fallback_indices.size()) {
                target_idx = fallback_indices[fallback_cursor++];
                has_idx = true;
            }
            if (has_idx && target_idx < pools.size() && pools[target_idx]) {
                for (std::size_t r = start_idx; r < results.size(); ++r) {
                    if (results[r]) {
                        this->link_info[i].extract_into_pool(*results[r], *pools[target_idx], 1);
                    }
                }
            }
        }
        this->consumed_results = results.size();
        return true;
    }

    bool iter_request_link::get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) {
        if (!this->parent || !this->parent->_node || this->parent->_node->request_results.empty()) {
            return false;
        }
        try {
            for (const iter_access_info& iter_link : this->iter_link_info) {
                to_fill.push_back(iter_link.get_params(this->parent->_node->request_results));
            }
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "[iter_request_link] Error getting dependencies: " << ex.what() << std::endl;
            return false;
        }
    }

    bool iter_request_link::populate_pools(std::vector<std::shared_ptr<ValuePool>>& pools, const std::vector<std::size_t>& fallback_indices, std::size_t& fallback_cursor) {
        if (!this->parent || !this->parent->_node || this->parent->_node->request_results.empty()) {
            return false;
        }
        const auto& results = this->parent->_node->request_results;
        std::size_t start_idx = this->consumed_results;
        for (std::size_t i = 0; i < this->iter_link_info.size(); ++i) {
            std::size_t target_idx = 0;
            bool has_idx = false;
            if (i < this->param_indices.size()) {
                target_idx = this->param_indices[i];
                has_idx = true;
            } else if (fallback_cursor < fallback_indices.size()) {
                target_idx = fallback_indices[fallback_cursor++];
                has_idx = true;
            }
            if (has_idx && target_idx < pools.size() && pools[target_idx]) {
                for (std::size_t r = start_idx; r < results.size(); ++r) {
                    if (results[r]) {
                        this->iter_link_info[i].extract_into_pool(*results[r], *pools[target_idx]);
                    }
                }
            }
        }
        this->consumed_results = results.size();
        return true;
    }

    // =========================================================================
    // UNIFIED LINKED REQUEST / POOL EXECUTION ENGINE
    // =========================================================================

    void LinkedRequest::ensure_pools_sized() {
        if (this->param_pools.size() < this->params.size()) {
            this->param_pools.resize(this->params.size(), nullptr);
        }
        for (std::size_t i = 0; i < this->params.size(); ++i) {
            if (!this->param_pools[i]) {
                this->param_pools[i] = std::make_shared<ValuePool>();
            }
        }
    }

    std::shared_ptr<ValuePool> LinkedRequest::get_or_create_pool(std::size_t param_idx) {
        if (param_idx >= this->params.size()) {
            this->params.resize(param_idx + 1);
        }
        ensure_pools_sized();
        return this->param_pools[param_idx];
    }

    void LinkedRequest::set_param_pool(std::size_t param_idx, std::shared_ptr<ValuePool> pool) {
        if (param_idx >= this->params.size()) {
            this->params.resize(param_idx + 1);
        }
        ensure_pools_sized();
        this->param_pools[param_idx] = std::move(pool);
    }

    bool LinkedRequest::ready() {
        bool is_ready = true;
        for (const auto& dep : this->dependencies) {
            if (dep.parent) {
                is_ready &= dep.parent->sent;
            }
        }
        for (const auto& iter_dep : this->iter_dependencies) {
            if (iter_dep.parent) {
                is_ready &= iter_dep.parent->sent;
            }
        }
        return is_ready;
    }

    bool LinkedRequest::all_parents_completed() const {
        for (const auto& dep : this->dependencies) {
            if (dep.parent && (!dep.parent->sent || dep.parent->in_progress)) {
                return false;
            }
        }
        for (const auto& iter_dep : this->iter_dependencies) {
            if (iter_dep.parent && (!iter_dep.parent->sent || iter_dep.parent->in_progress)) {
                return false;
            }
        }
        return true;
    }

    bool LinkedRequest::can_stream_from_parents() const {
        if (this->dependencies.empty() && this->iter_dependencies.empty()) {
            return false;
        }
        for (const auto& dep : this->dependencies) {
            if (!dep.parent || !dep.parent->sent || !dep.parent->_node || dep.parent->_node->request_results.empty()) {
                return false;
            }
        }
        for (const auto& iter_dep : this->iter_dependencies) {
            if (!iter_dep.parent || !iter_dep.parent->sent || !iter_dep.parent->_node || iter_dep.parent->_node->request_results.empty()) {
                return false;
            }
        }
        if (this->bind_mode == PoolBindMode::ZIP) {
            return true;
        }
        // In CROSS mode, streaming while a parent is still running is exact when at most 1 parameter slot is linked
        std::unordered_set<std::size_t> linked_slots;
        std::size_t cursor_check = 0;
        for (const auto& iter_link : this->iter_dependencies) {
            for (std::size_t i = 0; i < iter_link.iter_link_info.size(); ++i) {
                if (i < iter_link.param_indices.size()) linked_slots.insert(iter_link.param_indices[i]);
                else if (cursor_check < this->param_indices.size()) linked_slots.insert(this->param_indices[cursor_check++]);
            }
        }
        for (const auto& noniter_link : this->dependencies) {
            for (std::size_t i = 0; i < noniter_link.link_info.size(); ++i) {
                if (i < noniter_link.param_indices.size()) linked_slots.insert(noniter_link.param_indices[i]);
                else if (cursor_check < this->param_indices.size()) linked_slots.insert(this->param_indices[cursor_check++]);
            }
        }
        return linked_slots.size() <= 1;
    }

    bool LinkedRequest::fill_request() {
        return this->fill_req_priv();
    }

    bool LinkedRequest::fill_req_priv() {
        try {
            ensure_pools_sized();

            // Determine which parameter indices are fed by parent links
            std::vector<bool> is_linked_slot(this->params.size(), false);
            std::size_t cursor_check = 0;

            for (const auto& iter_link : this->iter_dependencies) {
                for (std::size_t i = 0; i < iter_link.iter_link_info.size(); ++i) {
                    if (i < iter_link.param_indices.size() && iter_link.param_indices[i] < is_linked_slot.size()) {
                        is_linked_slot[iter_link.param_indices[i]] = true;
                    } else if (cursor_check < this->param_indices.size() && this->param_indices[cursor_check] < is_linked_slot.size()) {
                        is_linked_slot[this->param_indices[cursor_check++]] = true;
                    }
                }
            }
            for (const auto& noniter_link : this->dependencies) {
                for (std::size_t i = 0; i < noniter_link.link_info.size(); ++i) {
                    if (i < noniter_link.param_indices.size() && noniter_link.param_indices[i] < is_linked_slot.size()) {
                        is_linked_slot[noniter_link.param_indices[i]] = true;
                    } else if (cursor_check < this->param_indices.size() && this->param_indices[cursor_check] < is_linked_slot.size()) {
                        is_linked_slot[this->param_indices[cursor_check++]] = true;
                    }
                }
            }

            // Initialize static pools (for unlinked slots that haven't been pre-populated)
            // and clear linked slots before extracting from parent responses
            for (std::size_t i = 0; i < this->params.size(); ++i) {
                if (is_linked_slot[i]) {
                    this->param_pools[i]->clearValues();
                } else if (this->param_pools[i]->empty()) {
                    this->param_pools[i]->push(this->params[i], true);
                }
            }

            bool success = true;
            std::size_t fallback_cursor = 0;

            for (iter_request_link& iter_link : this->iter_dependencies) {
                iter_link.consumed_results = 0;
                success &= iter_link.populate_pools(this->param_pools, this->param_indices, fallback_cursor);
            }
            for (request_link& noniter_link : this->dependencies) {
                noniter_link.consumed_results = 0;
                success &= noniter_link.populate_pools(this->param_pools, this->param_indices, fallback_cursor);
            }

            if (!success) return false;

            // Verify all parameter pools have at least 1 value
            for (std::size_t i = 0; i < this->param_pools.size(); ++i) {
                if (!this->param_pools[i] || this->param_pools[i]->empty()) {
                    return false;
                }
            }

            // Sync legacy param_fields for any external inspection
            this->param_fields.clear();
            for (std::size_t i = 0; i < this->param_pools.size(); ++i) {
                this->param_fields.push_back(this->param_pools[i]->values());
            }

            return apply_first_combination();
        } catch (const std::exception& ex) {
            std::cerr << "[LinkedRequest] Error filling request pools: " << ex.what() << std::endl;
            return false;
        }
    }

    bool LinkedRequest::append_from_parents() {
        try {
            ensure_pools_sized();
            std::vector<std::size_t> old_sizes;
            old_sizes.reserve(this->param_pools.size());
            for (const auto& p : this->param_pools) {
                old_sizes.push_back(p ? p->size() : 0);
            }

            std::size_t fallback_cursor = 0;
            for (iter_request_link& iter_link : this->iter_dependencies) {
                iter_link.populate_pools(this->param_pools, this->param_indices, fallback_cursor);
            }
            for (request_link& noniter_link : this->dependencies) {
                noniter_link.populate_pools(this->param_pools, this->param_indices, fallback_cursor);
            }

            bool grew = false;
            this->param_fields.clear();
            for (std::size_t i = 0; i < this->param_pools.size(); ++i) {
                this->param_fields.push_back(this->param_pools[i]->values());
                if (this->param_pools[i]->size() > old_sizes[i]) {
                    grew = true;
                }
            }
            if (grew) {
                update_tracker_total();
            }
            return grew;
        } catch (...) {
            return false;
        }
    }

    void LinkedRequest::update_tracker_total() {
        std::size_t total_combinations = 1;
        if (this->bind_mode == PoolBindMode::ZIP) {
            std::size_t max_steps = 1;
            bool has_multi = false;
            for (const auto& pool : this->param_pools) {
                if (!pool || pool->empty()) { total_combinations = 0; break; }
                if (pool->size() > 1) {
                    if (!has_multi || pool->size() < max_steps) {
                        max_steps = pool->size();
                        has_multi = true;
                    }
                }
            }
            total_combinations = has_multi ? max_steps : (this->param_pools.empty() ? 0 : 1);
        } else {
            for (const auto& pool : this->param_pools) {
                if (!pool || pool->empty()) { total_combinations = 0; break; }
                total_combinations *= pool->size();
            }
        }
        if (this->tracker) {
            this->tracker->set_total(static_cast<int>(total_combinations));
        }
    }

    bool LinkedRequest::apply_first_combination() {
        this->progress.assign(this->param_pools.size(), 0);
        this->zip_cursor = 0;
        this->pools_initialized = true;

        for (std::size_t i = 0; i < this->param_pools.size() && i < this->params.size(); ++i) {
            if (this->param_pools[i]->empty()) return false;
            this->params[i] = (*this->param_pools[i])[0];
        }

        update_tracker_total();
        return true;
    }

    bool LinkedRequest::update_base() {
        if (!this->pools_initialized) {
            return fill_request();
        }
        if (this->param_pools.empty()) {
            return false;
        }

        if (this->bind_mode == PoolBindMode::ZIP) {
            // Find the minimum size > 1 among multi-element pools
            std::size_t max_steps = 1;
            bool has_multi = false;
            for (const auto& pool : this->param_pools) {
                if (!pool || pool->empty()) return false;
                if (pool->size() > 1) {
                    if (!has_multi || pool->size() < max_steps) {
                        max_steps = pool->size();
                        has_multi = true;
                    }
                }
            }
            if (!has_multi || this->zip_cursor + 1 >= max_steps) {
                return false;
            }
            this->zip_cursor++;
            for (std::size_t i = 0; i < this->param_pools.size() && i < this->params.size(); ++i) {
                if (this->param_pools[i]->size() == 1) {
                    this->params[i] = (*this->param_pools[i])[0]; // Broadcast 1-element pool
                } else {
                    this->params[i] = (*this->param_pools[i])[this->zip_cursor];
                }
            }
            return true;
        }

        // PoolBindMode::CROSS (Cartesian odometer across all parameter pools)
        if (this->progress.size() != this->param_pools.size()) {
            return apply_first_combination();
        }

        int advance_pos = -1;
        for (int i = static_cast<int>(this->param_pools.size()) - 1; i >= 0; i--) {
            if (!this->param_pools[i] || this->param_pools[i]->empty()) return false;
            if (this->progress[i] + 1 < this->param_pools[i]->size()) {
                advance_pos = i;
                break;
            }
        }

        if (advance_pos < 0) {
            return false;
        }

        this->progress[advance_pos]++;
        this->params[advance_pos] = (*this->param_pools[advance_pos])[this->progress[advance_pos]];
        for (std::size_t j = static_cast<std::size_t>(advance_pos + 1); j < this->param_pools.size(); ++j) {
            this->progress[j] = 0;
            this->params[j] = (*this->param_pools[j])[0];
        }
        return true;
    }

    request::request(const int game, const int endpoint, const int endpoint_method, const std::vector<PARAM_CONT>& pars, const std::vector<P_NAME>& opt_names, const std::vector<PARAM_CONT>& opt_inputs, int* succ_ptr) {
        this->_game = game;
        this->_endpoint = endpoint;
        this->_endpoint_method = endpoint_method;
        this->params = pars;
        this->optional_names = opt_names;
        this->optional_inputs = opt_inputs;
        this->success_ptr = succ_ptr;
    }

    request LinkedRequest::get_base_copy() {
        request new_req = {this->_game, this->_endpoint, this->_endpoint_method, this->params, this->optional_names, this->optional_inputs};
        return new_req;
    }

    request::request(const int game, const int endpoint, const int endpoint_method) {
        this->_game = game;
        this->_endpoint = endpoint;
        this->_endpoint_method = endpoint_method;
    }

    request BatchRequest::FINISHED = request(-1, -1, -1);

    std::unique_ptr<LinkedRequest> RequestNode::send_request() {
        std::unique_ptr<LinkedRequest> to_send = std::move(this->_node->unsent_request);
        if (to_send && this->tracker && !to_send->tracker) {
            to_send->tracker = this->tracker;
        }
        this->sent = true;
        return to_send;
    }

    void BatchRequest::promote_ready_nodes() {
        while (!this->parent_requests->finished()) {
            auto node_handle = this->parent_requests->get_next();
            this->parent_requests->remove();
            if (!node_handle || !node_handle->_node || !node_handle->_node->unsent_request) {
                continue;
            }
            if (!node_handle->_node->unsent_request->pools_initialized) {
                if (!node_handle->_node->unsent_request->fill_request()) {
                    if (node_handle->tracker) {
                        node_handle->tracker->state.store(NodeExecState::FAILED);
                    }
                    continue;
                }
            }
            node_handle->in_progress = true;
            auto req = node_handle->send_request();
            if (req) {
                if (!req->tracker && node_handle->tracker) {
                    req->tracker = node_handle->tracker;
                }
                ActiveNodeSlot slot;
                slot.node_handle = node_handle;
                slot.req = std::move(req);
                slot.current_dispatched = false;
                this->active_slots_.push_back(std::move(slot));
            }
        }
    }

    void BatchRequest::finalize_completed_slots() {
        bool changed = true;
        while (changed) {
            changed = false;
            for (std::size_t i = 0; i < this->active_slots_.size(); ) {
                auto& slot = this->active_slots_[i];
                if (slot.current_dispatched) {
                    slot.req->append_from_parents();
                    if (slot.req->update_base()) {
                        slot.current_dispatched = false;
                    }
                }

                if (slot.current_dispatched && slot.req->all_parents_completed()) {
                    if (slot.node_handle) {
                        slot.node_handle->in_progress = false;
                        slot.node_handle->completed = true;
                    }
                    if (slot.req->tracker) {
                        if (slot.req->tracker->state.load() != NodeExecState::FAILED) {
                            slot.req->tracker->state.store(NodeExecState::COMPLETED);
                        }
                    }

                    for (auto& child : slot.req->children) {
                        if (!child) continue;
                        // If child was already opportunistically started, refresh its pools
                        for (auto& active_child : this->active_slots_) {
                            if (active_child.node_handle == child && active_child.req) {
                                active_child.req->append_from_parents();
                                if (active_child.current_dispatched && active_child.req->update_base()) {
                                    active_child.current_dispatched = false;
                                }
                            }
                        }
                        if (!child->sent && child->_node && child->_node->unsent_request) {
                            if (child->_node->unsent_request->ready() && child->_node->unsent_request->all_parents_completed()) {
                                try {
                                    if (child->_node->unsent_request->fill_request()) {
                                        if (child->tracker) {
                                            child->tracker->state.store(NodeExecState::READY);
                                        }
                                        this->parent_requests->insert(child);
                                    }
                                } catch (const std::exception& err) {
                                    std::cerr << "[BatchRequest] Error preparing child pools: " << err.what() << std::endl;
                                    if (child->tracker) {
                                        child->tracker->state.store(NodeExecState::FAILED);
                                        child->tracker->error_message = err.what();
                                    }
                                }
                            }
                        }
                    }

                    this->active_slots_.erase(this->active_slots_.begin() + static_cast<std::ptrdiff_t>(i));
                    if (this->current_slot_idx_ > i) {
                        this->current_slot_idx_--;
                    } else if (this->current_slot_idx_ >= this->active_slots_.size() && !this->active_slots_.empty()) {
                        this->current_slot_idx_ = 0;
                    }
                    promote_ready_nodes();
                    changed = true;
                } else {
                    ++i;
                }
            }
        }
    }

    request BatchRequest::get_current() {
        if (this->current_slot_idx_ >= this->active_slots_.size()) return BatchRequest::FINISHED;
        auto& slot = this->active_slots_[this->current_slot_idx_];
        if (!slot.req || slot.current_dispatched) return BatchRequest::FINISHED;
        if (slot.req->tracker) {
            slot.req->tracker->state.store(NodeExecState::EXECUTING);
        }
        return slot.req->get_base_copy();
    }

    request BatchRequest::get_next() {
        promote_ready_nodes();
        finalize_completed_slots();

        for (std::size_t i = 0; i < this->active_slots_.size(); ++i) {
            if (!this->active_slots_[i].current_dispatched && this->active_slots_[i].req) {
                this->current_slot_idx_ = i;
                if (this->active_slots_[i].req->tracker) {
                    this->active_slots_[i].req->tracker->state.store(NodeExecState::EXECUTING);
                }
                return this->active_slots_[i].req->get_base_copy();
            }
        }
        this->completed_.store(true);
        return BatchRequest::FINISHED;
    }

    request BatchRequest::select_next_ready(const std::function<int(const request&)>& wait_time_fn) {
        promote_ready_nodes();
        finalize_completed_slots();

        if (this->active_slots_.empty()) {
            this->completed_.store(true);
            return BatchRequest::FINISHED;
        }

        // 1. Check if current slot is ready to dispatch without rate limit wait
        if (this->current_slot_idx_ < this->active_slots_.size() &&
            !this->active_slots_[this->current_slot_idx_].current_dispatched) {
            request cand = this->active_slots_[this->current_slot_idx_].req->get_base_copy();
            int wait_sec = wait_time_fn ? wait_time_fn(cand) : 0;
            if (wait_sec <= 0) {
                if (this->active_slots_[this->current_slot_idx_].req->tracker) {
                    this->active_slots_[this->current_slot_idx_].req->tracker->state.store(NodeExecState::EXECUTING);
                }
                return cand;
            }
        }

        // 2. Current slot is rate-limited (or waiting for parent items); search active_slots_ for minimum wait time
        std::size_t best_idx = this->active_slots_.size();
        int best_wait = 999999;
        for (std::size_t i = 0; i < this->active_slots_.size(); ++i) {
            if (this->active_slots_[i].current_dispatched || !this->active_slots_[i].req) continue;
            request cand = this->active_slots_[i].req->get_base_copy();
            int w = wait_time_fn ? wait_time_fn(cand) : 0;
            if (w < best_wait) {
                best_wait = w;
                best_idx = i;
                if (w <= 0) break;
            }
        }

        if (best_idx < this->active_slots_.size() && best_wait <= 0) {
            this->current_slot_idx_ = best_idx;
            if (this->active_slots_[best_idx].req->tracker) {
                this->active_slots_[best_idx].req->tracker->state.store(NodeExecState::EXECUTING);
            }
            return this->active_slots_[best_idx].req->get_base_copy();
        }

        // 3. All active slots are currently rate-limited (best_wait > 0).
        // Check if any child node of an active slot can opportunistically stream already-available parent results
        // on a non-rate-limited endpoint/region bucket!
        if (wait_time_fn) {
            for (std::size_t i = 0; i < this->active_slots_.size(); ++i) {
                for (auto& child : this->active_slots_[i].req->children) {
                    if (!child || child->sent || !child->_node || !child->_node->unsent_request) continue;
                    if (!child->_node->unsent_request->can_stream_from_parents()) continue;

                    request probe = child->_node->unsent_request->get_base_copy();
                    if (wait_time_fn(probe) <= 0) {
                        if (child->_node->unsent_request->fill_request()) {
                            request filled_probe = child->_node->unsent_request->get_base_copy();
                            if (wait_time_fn(filled_probe) <= 0) {
                                child->in_progress = true;
                                auto req = child->send_request();
                                if (req) {
                                    if (!req->tracker && child->tracker) {
                                        req->tracker = child->tracker;
                                    }
                                    if (req->tracker) {
                                        req->tracker->state.store(NodeExecState::EXECUTING);
                                    }
                                    ActiveNodeSlot slot;
                                    slot.node_handle = child;
                                    slot.req = std::move(req);
                                    slot.current_dispatched = false;
                                    this->active_slots_.push_back(std::move(slot));
                                    this->current_slot_idx_ = this->active_slots_.size() - 1;
                                    return this->active_slots_.back().req->get_base_copy();
                                }
                            }
                        }
                    }
                }
            }
        }

        if (best_idx < this->active_slots_.size()) {
            this->current_slot_idx_ = best_idx;
            if (this->active_slots_[best_idx].req->tracker) {
                this->active_slots_[best_idx].req->tracker->state.store(NodeExecState::EXECUTING);
            }
            return this->active_slots_[best_idx].req->get_base_copy();
        }

        this->completed_.store(true);
        return BatchRequest::FINISHED;
    }

    bool BatchRequest::insert_result(json_ptr result) {
        if (this->current_slot_idx_ >= this->active_slots_.size()) return false;
        auto& slot = this->active_slots_[this->current_slot_idx_];
        if (!slot.req) return false;

        if (slot.req->tracker) {
            slot.req->tracker->increment_dispatched();
        }

        if (result && !slot.req->children.empty() && slot.node_handle && slot.node_handle->_node) {
            slot.node_handle->_node->request_results.push_back(std::move(result));
            // Notify any already-streaming child in active_slots_
            for (auto& child : slot.req->children) {
                for (auto& active_slot : this->active_slots_) {
                    if (active_slot.node_handle == child && active_slot.req) {
                        active_slot.req->append_from_parents();
                        if (active_slot.current_dispatched && active_slot.req->update_base()) {
                            active_slot.current_dispatched = false;
                        }
                    }
                }
            }
        }

        // Advance to next value combination in the current request's ValuePools
        if (slot.req->update_base()) {
            slot.current_dispatched = false;
            return true;
        }

        slot.current_dispatched = true;
        finalize_completed_slots();
        return false;
    }

    BatchRequest::BatchRequest(const std::vector<std::shared_ptr<RequestNode>>& requests) {
        for (const auto& req : requests) {
            if (req) {
                if (req->tracker) {
                    req->tracker->state.store(NodeExecState::READY);
                }
                this->parent_requests->insert(req);
            }
        }
    }
}
