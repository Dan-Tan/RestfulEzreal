#include "BatchRequests.h"
#include <json/value.h>
#include <stdexcept>

namespace restfulEz {

    const Json::Value* json_access_info::index_json(const Json::Value* to_index) const {
        const Json::Value* to_return = to_index;
        try {
            for (const KEY_CONT& key : keys) {
                to_index = &(*to_index)[key.key];
            }
        }
        catch (Json::Exception& ex) {
            throw std::runtime_error("Invalid json keys given");
        }
        return to_return;
    }

    static inline void access_params(const Json::Value* response, std::vector<PARAM_CONT>& to_add, const std::vector<KEY_CONT>& keys, std::size_t iter_limit) {
        const Json::Value* access;

        if (!response->isArray()) {
            throw std::runtime_error("Invalid sequence of keys, Json is not array and cannot be iterated through");
        }
        std::size_t counter = 0;
        for (auto& json :*response) {
             
            if (counter >= iter_limit) {break;}

            access = &json;
            for (auto& key : keys) {
                access = &(*access)[key.key]; // not very nice
            }
            to_add.push_back((*access).asCString());
            counter++;
        }
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(std::shared_ptr<Json::Value> response) const {
        std::vector<PARAM_CONT> to_ret;

        const Json::Value* array_json = this->index_json(response.get());

        access_params(response.get(), to_ret, this->access_after_iter.keys, this->iter_limit);

        return to_ret;
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(std::vector<std::shared_ptr<Json::Value>> responses) const {
        std::vector<PARAM_CONT> to_ret;

        const Json::Value* array_json = nullptr;

        for (std::shared_ptr<Json::Value> result : responses) {
            array_json = this->index_json(result.get());
            access_params(result.get(), to_ret, this->access_after_iter.keys, this->iter_limit);
        }
        return to_ret;
    }

    bool request_link::get_dependencies(std::vector<PARAM_CONT>& to_fill) {
        try {
            std::size_t index = 0;
            for (json_access_info& info : this->link_info) {
                to_fill[this->param_indices[index]] = info.get_param(this->parent->request_results[0]);
                index++;
            }
        }
        catch (std::out_of_range &ex) {
            throw std::out_of_range("Improper construction of links and dependencies, check constructor, or parent request not sent yet");
        }
        catch (std::runtime_error &ex) {
            // user passed invalid json keys
            return false;
        }
        return true;
    }

    bool iter_request_link::get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) {
        try {
            std::size_t counter = 0;
            for (iter_access_info& iter_link : this->iter_link_info) {
                to_fill[this->param_indices[counter]] = iter_link.get_params(this->parent->request_results);
                counter++;
            }
            return true;
        }
        catch (std::out_of_range &ex) {
            throw std::out_of_range((std::string("Improper construction of links and dependencies, check construction")+ std::string(ex.what())).c_str());
        }
        catch(std::runtime_error &ex) {
            // user passed invalid json keys
            return false;
        }
        return false;
    }

    bool LinkedRequest::fill_request() {
        try {
            bool success = true;
            for (request_link& link : this->dependencies) {
                success &= link.get_dependencies(this->params);
            }
            return success;
        }
        catch (std::runtime_error &ex) {
            // user invalid json access key
            return false;
        }
        catch (std::out_of_range &ex) {
            // parent request has not been sent yet!!!
            return false;
        }
    }

    bool IterativeRequest::iter_fill_request() {
        try {

            this->fill_request();
            bool success = true;
            for (iter_request_link& iter_link : this->iter_dependencies) {
                success &= iter_link.get_dependencies(this->param_fields); 
            }
            return success;
        }
        catch (std::runtime_error &ex) {
            // user passed invalid json keys
            return false;
        }
        catch (std::out_of_range &ex) {
            // parent request has not been sent yet
            return false;
        };
        return false;
    }

    static inline std::shared_ptr<ListNode> insert_node(std::shared_ptr<ListNode> current_node, std::shared_ptr<RequestNode> to_insert) {
        std::shared_ptr<ListNode> new_node = std::make_shared<ListNode>(nullptr, to_insert, nullptr);
        if (!current_node) {
            current_node = new_node;
            current_node->previous = current_node;
            current_node->next = current_node;
            return current_node;
        }

        new_node->previous = current_node->previous;
        new_node->next = current_node;

        new_node->previous->next = new_node;
        new_node->next->previous = new_node;

        return current_node;

    }

    void CLinkedList::insert(std::shared_ptr<RequestNode> to_insert) {
        this->length++;
        if (!this->beginning) {
            this->beginning = insert_node(this->beginning, to_insert);

            this->end = this->beginning;
            this->current_position = this->beginning;
            return;
        }
        if (this->current_position == this->beginning) {
            this->beginning = insert_node(this->beginning, to_insert);
            return;
        }

        std::shared_ptr<ListNode> new_node = std::make_shared<ListNode>(this->current_position->previous, to_insert, this->current_position);
        this->current_position->previous->next = new_node;
        this->current_position->previous = new_node;
        return;
    };

    void CLinkedList::remove() {
        // if I decide to switch to non-circular linked list the conditionals are mostly not required but leave them for now
        // removing the last remaining element
        if (this->beginning == this->end) {
            this->beginning = nullptr;
            this->current_position = nullptr;
            this->end = nullptr;
            this->length = 0;
            return;
        }
        // removing end element
        if (this->end == this->current_position) {
            this->end = this->current_position->previous;
            this->end->next = this->beginning;
            return;
        }
        // remove beginning element
        if (this->beginning == this->current_position) {
            // move the beginning forward one
            this->beginning = this->current_position->next;
            // reassigns the end->next to the new beginning
            this->end->next = this->beginning;
            // reassign the current position to the new beginning
            this->current_position = this->beginning;
            return;
        }
        this->current_position->previous = this->current_position->next;
        this->current_position->next = this->current_position->previous;
        this->current_position = this->current_position->next;
    }

    request BatchRequest::FINISHED = request();

    request& BatchRequest::get_next() {
        if (this->parent_requests->finished()) {
            bool finished = this->reformat_parents(); 
            if (finished) {
                return BatchRequest::FINISHED;
            }
        }
        this->current_results = this->parent_requests->get_next();
        this->current_request = this->current_results->send_request();
        return *current_request;
    }

    void BatchRequest::insert_result(std::shared_ptr<Json::Value> result) {
        this->current_results->request_results.push_back(result);
    }
}
