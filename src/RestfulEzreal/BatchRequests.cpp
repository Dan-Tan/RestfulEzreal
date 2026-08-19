#include "BatchRequests.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "simdjson.h"

#ifdef DEBUG_MODE
#define D(x) std::cerr << x << '\n'
#else
#define D(x)  
#endif

namespace restfulEz {

    PARAM_CONT json_access_info::get_param(const raw_json& response) const {
        PARAM_CONT param_extracted;
        if (response.empty()) return param_extracted;

        size_t valid_len = response.size();
        while (valid_len > 0 && response[valid_len - 1] == '\0') {
            valid_len--;
        }
        if (valid_len == 0) return param_extracted;

        try {
            simdjson::padded_string padded(response.data(), valid_len);
            simdjson::dom::parser parser;
            simdjson::dom::element elem;
            if (parser.parse(padded).get(elem) != simdjson::SUCCESS) {
                return param_extracted;
            }

            for (const KEY_CONT& key : this->keys) {
                if (key.key[0] == '\0') continue;
                elem = elem[key.key];
            }

            std::string_view sv;
            if (elem.get(sv) == simdjson::SUCCESS) {
                size_t copy_len = std::min(sv.length(), sizeof(param_extracted.param) - 1);
                memcpy(param_extracted.param, sv.data(), copy_len);
                param_extracted.param[copy_len] = '\0';
            } else {
                int64_t val_int = 0;
                if (elem.get(val_int) == simdjson::SUCCESS) {
                    std::string s = std::to_string(val_int);
                    size_t copy_len = std::min(s.length(), sizeof(param_extracted.param) - 1);
                    memcpy(param_extracted.param, s.data(), copy_len);
                    param_extracted.param[copy_len] = '\0';
                }
            }
        } catch (const std::exception& ex) {
            std::cerr << "[json_access_info] Parsing exception: " << ex.what() << std::endl;
        }

        return param_extracted;
    }

    static inline void access_params(const raw_json& response, std::vector<PARAM_CONT>& to_add, const std::vector<KEY_CONT>& keys_one, const std::vector<KEY_CONT>& keys_two, std::size_t iter_limit) {
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

            for (const KEY_CONT& key : keys_one) {
                if (key.key[0] == '\0') continue;
                elem = elem[key.key];
            }

            simdjson::dom::array arr;
            if (elem.get(arr) != simdjson::SUCCESS) {
                return;
            }

            std::size_t count = 0;
            for (simdjson::dom::element item : arr) {
                if (iter_limit > 0 && count >= iter_limit) break;

                for (const KEY_CONT& sec_key : keys_two) {
                    if (sec_key.key[0] == '\0') continue;
                    item = item[sec_key.key];
                }

                PARAM_CONT param_to_add;
                std::string_view sv;
                if (item.get(sv) == simdjson::SUCCESS) {
                    size_t copy_len = std::min(sv.length(), sizeof(param_to_add.param) - 1);
                    memcpy(param_to_add.param, sv.data(), copy_len);
                    param_to_add.param[copy_len] = '\0';
                    to_add.push_back(param_to_add);
                    count++;
                } else {
                    int64_t val_int = 0;
                    if (item.get(val_int) == simdjson::SUCCESS) {
                        std::string s = std::to_string(val_int);
                        size_t copy_len = std::min(s.length(), sizeof(param_to_add.param) - 1);
                        memcpy(param_to_add.param, s.data(), copy_len);
                        param_to_add.param[copy_len] = '\0';
                        to_add.push_back(param_to_add);
                        count++;
                    }
                }
            }
        } catch (const std::exception& ex) {
            std::cerr << "[access_params] Parsing exception: " << ex.what() << std::endl;
        }
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(const raw_json& response) const {
        std::vector<PARAM_CONT> to_ret;
        access_params(response, to_ret, this->keys, this->access_after_iter.keys, this->iter_limit);
        return to_ret;
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(const std::vector<json_ptr>& responses) const {
        std::vector<PARAM_CONT> to_ret;
        for (size_t i = 0; i < responses.size(); i++) {
            if (responses[i]) {
                access_params(*responses[i], to_ret, this->keys, this->access_after_iter.keys, this->iter_limit);
            }
        }
        return to_ret;
    }

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
                std::vector<PARAM_CONT> field_values;
                for (size_t i = 0; i < this->parent->_node->request_results.size(); i++) {
                    if (this->parent->_node->request_results[i]) {
                        field_values.push_back(noniter_link.get_param(*this->parent->_node->request_results[i]));
                    }
                }
                to_fill.push_back(std::move(field_values));
            }
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "[request_link] Error getting multi-result dependencies: " << ex.what() << std::endl;
            return false;
        }
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

    bool LinkedRequest::fill_request() {
        return this->fill_req_priv();
    }

    bool LinkedRequest::ready() {
        bool is_ready = true;
        for (const auto& dep : this->dependencies) {
            if (dep.parent) {
                is_ready &= dep.parent->sent;
            }
        }
        return is_ready;
    }

    bool IterativeRequest::ready() {
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

    bool LinkedRequest::fill_req_priv() {
        try {
            bool success = true;
            for (request_link& link : this->dependencies) {
                success &= link.get_dependencies(this->params);
            }
            return success;
        } catch (const std::exception& ex) {
            std::cerr << "[LinkedRequest] Error filling request: " << ex.what() << std::endl;
            return false;
        }
    }

    bool IterativeRequest::fill_request() {
        try {
            bool success = true;
            this->param_fields.clear();
            for (request_link& noniter_link : this->dependencies) {
                if (noniter_link.parent && noniter_link.parent->_node) {
                    if (noniter_link.parent->_node->request_results.size() <= 1) {
                        success &= noniter_link.get_dependencies(this->params);
                    } else {
                        success &= noniter_link.get_dependencies(this->param_fields);
                    }
                }
            }
            for (iter_request_link& iter_link : this->iter_dependencies) {
                success &= iter_link.get_dependencies(this->param_fields);
            }
            return success;
        } catch (const std::exception& ex) {
            std::cerr << "[IterativeRequest] Error filling iterative request: " << ex.what() << std::endl;
            return false;
        }
    }

    bool IterativeRequest::update_base() {
        if (this->param_fields.empty() || this->param_indices.empty()) {
            return false;
        }

        // Verify that all param_fields have items
        for (const auto& field : this->param_fields) {
            if (field.empty()) return false;
        }

        if (this->progress.empty()) {
            this->progress.resize(this->param_fields.size(), 0);
            for (size_t i = 0; i < this->param_indices.size() && i < this->param_fields.size(); i++) {
                if (this->param_indices[i] < this->params.size()) {
                    this->params[this->param_indices[i]] = this->param_fields[i][0];
                }
            }
            return true;
        }

        // Advance progress odometer
        for (int i = static_cast<int>(this->progress.size()) - 1; i >= 0; i--) {
            if (this->param_fields[i].empty()) continue;
            if (this->progress[i] + 1 >= this->param_fields[i].size()) {
                this->progress[i] = 0;
                if (i < static_cast<int>(this->param_indices.size()) && this->param_indices[i] < this->params.size()) {
                    this->params[this->param_indices[i]] = this->param_fields[i][0];
                }
            } else {
                this->progress[i]++;
                if (i < static_cast<int>(this->param_indices.size()) && this->param_indices[i] < this->params.size()) {
                    this->params[this->param_indices[i]] = this->param_fields[i][this->progress[i]];
                }
                return true;
            }
        }

        return false;
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
        this->sent = true;
        return to_send;
    }

    request BatchRequest::get_current() {
        if (!this->current_request) return BatchRequest::FINISHED;
        return this->current_request->get_base_copy();
    }

    request BatchRequest::get_next() {
        if (this->parent_requests->finished()) {
            return BatchRequest::FINISHED;
        }
        this->current_results = this->parent_requests->get_next();
        if (!this->current_results || !this->current_results->_node || !this->current_results->_node->unsent_request) {
            return BatchRequest::FINISHED;
        }
        this->current_request = this->current_results->send_request();
        return this->current_request->get_base_copy();
    }

    bool BatchRequest::insert_result(json_ptr result) {
        if (!this->current_request) return false;

        if (result && this->current_request->children.size() != 0) {
            this->current_results->_node->request_results.push_back(std::move(result));
        }

        // Check if there are more iterations for the current request
        if (!this->current_request->update_base()) {
            for (auto& child : this->current_request->children) {
                if (child && !child->sent && child->_node && child->_node->unsent_request) {
                    if (child->_node->unsent_request->ready()) {
                        try {
                            if (child->_node->unsent_request->fill_request()) {
                                child->_node->unsent_request->update_base();
                                this->parent_requests->insert(child);
                            }
                        } catch (const std::exception& err) {
                            std::cerr << "[BatchRequest] Error preparing child: " << err.what() << std::endl;
                        }
                    }
                }
            }
            this->parent_requests->remove();
            return false;
        }
        return true;
    }

    BatchRequest::BatchRequest(const std::vector<std::shared_ptr<RequestNode>>& requests) {
        for (const auto& req : requests) {
            if (req) {
                this->parent_requests->insert(req);
            }
        }
    }

    json_access_info iter_access_info::get_base() const {
        return json_access_info(this->keys);
    }
}

