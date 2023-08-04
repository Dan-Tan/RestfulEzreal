#include "BatchRequests.h"
#include <json/value.h>
#include <stdexcept>

namespace restfulEz {

    void fill_iterative_dependencies(std::shared_ptr<Linked_Request>& req, const param_dependence_info& dep, Json::Value& response) {
        std::shared_ptr<Iterative_Request> request = std::reinterpret_pointer_cast<Iterative_Request>(req);
        if (!response) {
            throw std::invalid_argument("Nullptr passed to fill iterative dependencies. Make sure this function is only called from the structure method");
        }

        Json::Value& iterative_json = response;
        try {
            int iter_count = 0;
            for (const PARAM_CONT& ind_key : dep.json_keys) {
                iterative_json = iterative_json[ind_key.param];
                if (iter_count == dep.iter_index - 1) {
                    break;
                }
            }
            if (!iterative_json.isArray()) { // User gave invalid json keys
                throw std::runtime_error("Sequence of json keys or iter index gave non-iterative json object");
            }

            Json::Value temp_json = iterative_json;
            request->param_indices.push_back(dep.param_index);
            std::vector<PARAM_CONT> from_parent;

            for (int i = 0; i < iterative_json.size(); i++) { // problem here
                temp_json = iterative_json[i];
                for (int i = dep.iter_index; i < dep.json_keys.size(); i++) {
                    temp_json = temp_json[dep.json_keys.at(i).param];
                }
                from_parent.push_back(temp_json.asCString());
            }
            request->params_from_parent.push_back(from_parent);
            request->completed_dependencies[dep.param_index] = 1;
        }
        catch (Json::Exception& ex) {
            throw std::runtime_error("Invalid Json keys given, please re-check documentation");
        }
    }

    void fill_noniter_dependencies(std::shared_ptr<Linked_Request>& req, const param_dependence_info& dep, Json::Value& response) {
        if (!response) {
            throw std::invalid_argument("Nullptr passed to fill non-iter dependencies. Make sure this function is only called from the structure method");
        }
        try {
            Json::Value& resp = response;
            for (const PARAM_CONT& j_key : dep.json_keys) {
                resp = resp[j_key.param];
            }
            req->params[dep.param_index] = resp.asCString();
            req->completed_dependencies[dep.param_index] = 1;
        }
        catch (Json::Exception& ex) {
            throw std::runtime_error("User has passed the invalid json keys");
        }
    }

    void Linked_Request::fill_dependencies() {
        // check that the request has executed and the pointer to the reponse is not nullptr
        if (!this->response) {
            throw std::logic_error("Attempting to fill child dependencies whilst the parent pointer to response is nullptr. Make sure the parent has executed it's request before attempting to fill child dependencies");
        }
        // extract up to iteration
        for (request_link& link : this->child_links) {
            for (param_dependence_info& dep : link.dependence_information) {
                if (dep.iterative) {
                    fill_iterative_dependencies(link.request, dep, this->response);
                } else  {
                    fill_noniter_dependencies(link.request, dep, this->response);
                }
            }
        }
    }

    bool Linked_Request::ready() {
        bool ready_to_send = true;

        if (this->required_dependencies.size() != this->completed_dependencies.size()) {
            throw std::logic_error("Mismatched required and completed dependencies. Check for proper construction");
        }
        // can you just check for vector equality here?
        for (int i = 0; i < this->required_dependencies.size(); i++) {
            ready_to_send &= (this->required_dependencies[i] == this->completed_dependencies[i]);
        }

        return ready_to_send;
    }

    Linked_Request::Linked_Request(const std::vector<std::size_t>& required_params) : required_dependencies(required_params) {
        for (int i = 0; i < required_params.size(); i++) {
            this->completed_dependencies.push_back(0);
        }
    }

    Linked_Request::Linked_Request(const request& base, const std::vector<std::size_t>& required_params) : required_dependencies(required_params), request(base) {
        for (int i = 0; i < required_params.size(); i++) {
            this->completed_dependencies.push_back(0);
        }
    }

    bool Iterative_Request::fill_next() {
        // if first request initialise beginning of progress
        if (this->iter_progress.size() == 0) {
            for (int i = 0; i < this->params_from_parent.size(); i++) {
                this->iter_progress.push_back(0);
            }
        }

        // fill the base class
        int counter = 0;
        for (auto& ind : this->param_indices) {
            this->params[ind] = this->params_from_parent[counter][this->iter_progress[ind]];
        }

        bool finished = false;
        // update progress
        for (int i = this->iter_progress.size() - 1;  i >= 0; i--) {
            // if current progress on given param has finished reset and continue iteration
            if (this->iter_progress[i] == this->params_from_parent[i].size() - 1) {
                this->iter_progress[i] = 0;
                // if the first param has finished then all combinations have been executed
                if (i == 0) {
                    finished = true;
                }
            } else {
                // increment progress
                this->iter_progress[i]++;
                break;
            }
        };
        
        return finished;
    }

    void insert_request(std::shared_ptr<Batch_Request> current_node, std::shared_ptr<Linked_Request> child_node) {
        if (!current_node->request_node) { // passed empty linked list, replace nullptr
            current_node->previous = current_node;
            current_node->next = current_node;
            current_node->request_node = child_node;
            return;
        }
        std::shared_ptr<Batch_Request> new_node = std::make_shared<Batch_Request>(nullptr, child_node, nullptr); 
        std::shared_ptr<Batch_Request> previous_node = current_node->previous;
        new_node->previous = previous_node;
        previous_node->next = new_node;
        current_node->previous = new_node;
    }

    std::shared_ptr<Batch_Request> remove_request(std::shared_ptr<Batch_Request> node) {
        if (node->previous == node) {
            return nullptr;
        } else if (node->next == node) {
            throw std::logic_error("Linked list passed was not circular, this method expects circular linked lists");
        }
        std::shared_ptr<Batch_Request> previous_node = node->previous;
        std::shared_ptr<Batch_Request> next_node = node->next;
        previous_node->next = next_node;
        next_node->previous = previous_node;
        return next_node;
    }

}
