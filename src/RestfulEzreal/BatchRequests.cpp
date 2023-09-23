#include "BatchRequests.h"
#include <json/value.h>
#include <stdexcept>
#include "simdjson.h"

#ifdef DEBUG_MODE
#include <iostream>
#define D(x) std::cerr << x <<'\n'
#else
#define D(x)  
#endif

namespace restfulEz {

    static simdjson::ondemand::parser parser; //todo convert to unique_ptr in BatchRequest
    
    PARAM_CONT json_access_info::get_param(const raw_json& response) const {

        simdjson::ondemand::document doc = parser.iterate(response.data(), strlen(response.data()), sizeof(response.data()));
        simdjson::ondemand::value obj_temp = doc;

        for (const KEY_CONT& key : this->keys) {
            obj_temp = obj_temp.find_field(key.key);
        }

        PARAM_CONT param_extracted;
        const std::string_view param_in_json = obj_temp.get_string().value();
        param_in_json.copy(param_extracted.param, param_in_json.length());

        return param_extracted;
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(const raw_json& response) const {
        std::vector<PARAM_CONT> to_ret;

        simdjson::ondemand::document doc = parser.iterate(response.data(), strlen(response.data()), sizeof(response.data()));
        simdjson::ondemand::value obj_temp = doc;

        for (const KEY_CONT& key : this->keys) {
            obj_temp = obj_temp.find_field(key.key);
        }

        if (!(obj_temp.type() == simdjson::ondemand::json_type::array)) {
            throw std::runtime_error("User passed invalid keys not pointing to array");
        }

        std::string_view param_to_add;

        for (simdjson::ondemand::value tmp : obj_temp) {
            for (const KEY_CONT& sec_key : this->access_after_iter.keys) {
                tmp = tmp.find_field(sec_key.key);
            }
            param_to_add = tmp.get_string().value();
            to_ret.push_back({});
            param_to_add.copy(to_ret.back().param, param_to_add.length());
        }
        return to_ret;
    }

    static inline void access_params(const raw_json& response, std::vector<PARAM_CONT>& to_add, const std::vector<KEY_CONT>& keys_one, const std::vector<KEY_CONT>& keys_two, std::size_t iter_limit) {
        simdjson::ondemand::document doc = parser.iterate(response.data(), strlen(response.data()), sizeof(response.data()));
        simdjson::ondemand::value obj_temp = doc;

        for (const KEY_CONT& key : keys_one) {
            obj_temp = obj_temp.find_field(key.key);
        }

        if (!(obj_temp.type() == simdjson::ondemand::json_type::array)) {
            throw std::runtime_error("User passed invalid keys not pointing to array");
        }

        std::string_view param_to_add;

        for (simdjson::ondemand::value tmp : obj_temp) {
            for (const KEY_CONT& sec_key : keys_two) {
                tmp = tmp.find_field(sec_key.key);
            }
            param_to_add = tmp.get_string().value();
            to_add.push_back({});
            param_to_add.copy(to_add.back().param, param_to_add.length());
        }
        
    }

    std::vector<PARAM_CONT> iter_access_info::get_params(const std::vector<json_ptr>& responses) const {
        std::vector<PARAM_CONT> to_ret;

        for (int i = 0; i < responses.size(); i++) {
            access_params(*responses[i], to_ret, this->keys, this->access_after_iter.keys, this->iter_limit);
        }
        return to_ret;
    }

    bool request_link::get_dependencies(std::vector<PARAM_CONT>& to_fill) {
        try {
            std::size_t index = 0;
            for (json_access_info& info : this->link_info) {
                to_fill[this->param_indices[index]] = info.get_param(*this->parent->_node->request_results[0]);
                index++;
            }
        }
        catch (std::out_of_range &ex) {
            std::cerr <<  "Improper construction of links and dependencies, check constructor, or parent request not sent yet. \n ERROR: " << ex.what() << std::endl;;
        }
        catch (std::runtime_error &ex) {
            // user passed invalid json keys
            return false;
        }
        return true;
    }

    bool iter_request_link::get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) {
        try {
            for (iter_access_info& iter_link : this->iter_link_info) {
                to_fill.push_back(iter_link.get_params(this->parent->_node->request_results));
            }
            return true;
        }
        catch (std::out_of_range &ex) {
            std::cerr <<  "Improper construction of links and dependencies, check construction. \nERROR: " << ex.what() << std::endl;
        }
        catch(std::runtime_error &ex) {
            // user passed invalid json keys
            return false;
        }
        return false;
    }

    bool LinkedRequest::fill_request() {
        return this->fill_req_priv();
    }

    bool LinkedRequest::ready() {
        bool ready = true;
        for (auto& dep : this->dependencies) {
            ready &= dep.parent->sent;
        }
        return ready;
    }

    bool IterativeRequest::ready() {
        bool ready = true;
        for (auto& dep : this->dependencies) {
            ready &= dep.parent->sent;
        }
        for (auto& iter_dep : this->iter_dependencies) {
            ready &= iter_dep.parent->sent;
        }
        return ready;
    }

    bool LinkedRequest::fill_req_priv() {
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

    bool request_link::get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) {
        try {
            std::size_t counter = 0;
            for (json_access_info& noniter_link : this->link_info) {
                to_fill.push_back({});
                for (int i = 0; i < this->parent->_node->request_results.size(); i++) {
                    to_fill.back().push_back(noniter_link.get_param(*this->parent->_node->request_results[i]));
                }
                counter++;
            }
            return true;
        }
        catch (std::out_of_range &ex) {
            std::cerr << "Improper construction of links and dependencies, check construction. \nError: " << ex.what() << std::endl;
            return false;
        }
        catch(std::runtime_error &ex) {
            // user passed invalid json keys
            return false;
        }
    }

    bool IterativeRequest::fill_request() {
        try {
            bool success = true;
            D("Filling Dependencies in iterative request");
            D("#Non-iterative links: " << this->dependencies.size());
            D("#Iterative links    : " << this->iter_dependencies.size());
            for (request_link& noniter_link : this->dependencies) {
                // if a parent was iterative we treat the link as iterative but access the params in a non iterative manner;
                if (noniter_link.parent->_node->request_results.size() == 1) {
                    D("Regular non-iterative link");
                    success &= noniter_link.get_dependencies(this->params);
                } else {
                    D("Iterative non-iterative link");
                    success &= noniter_link.get_dependencies(this->param_fields);
                    this->param_indices.insert(this->param_indices.end(), noniter_link.param_indices.begin(), noniter_link.param_indices.end());
                }
            }
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

    bool IterativeRequest::update_base() {
        // create vector filled with 0 if not alreay done
        D("Updating Base for iterative request");
        D("Param index size: " << this->param_indices.size());
        D("Iterative param fields size: " << this->param_fields.size());
        D("Param size: " << this->params.size());
        if (progress.size() == 0) {
            for (const auto& dep_field :this->param_fields) {
                this->progress.push_back(0);
            }
            for (int i = 0; i < this->param_indices.size(); i++) {
                this->params[this->param_indices[i]] = this->param_fields[i][0];
            }
            return true;
        }
        
        // update the progress of the iterative request and fill in new parameter to the base class of which the sender has a reference
        bool continu;
        for (int i = this->progress.size()-1; i >= 0; i--) {
            continu = false;
            if (this->progress[i] == this->param_fields[i].size()-1) {
                this->progress[i] = 0;
                continu = true;
            } else {
                this->progress[i]++;
            }
            this->params[this->param_indices[i]] = this->param_fields[i][this->progress[i]];
            if (!continu) {
                return true;
            }
        }

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
        D("Inserting new child");
        this->length++;
        if (!this->beginning) {
            D("Inserting first node");
            this->beginning = insert_node(this->beginning, to_insert);

            this->end = this->beginning;
            this->current_position = this->beginning;
            return;
        }
        if (this->current_position == this->beginning) {
            D("Insert new node, clinkedlist was a singleton");
            this->current_position = insert_node(this->beginning, to_insert);
            this->beginning = this->current_position->previous;
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
        D("removing next request");
        this->length--;
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
            this->current_position = this->end;
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
        D("Constructing copy to send");
        D("(Game|Endpoint|EndpointMethod): " << this->_game << this->_endpoint << this->_endpoint_method);
        for (auto& par : this->params) {
            D(par.param);
        }
        request new_req = {this->_game, this->_endpoint, this->_endpoint_method, this->params, this->optional_names, this->optional_inputs};
        return new_req;
    }

    request::request(const int game, const int endpoint, const int endpoint_method) {
        this->_game = game;
        this->_endpoint = endpoint;
        this->_endpoint_method = endpoint_method;
    }

    request BatchRequest::FINISHED = request(-1, -1, -1);

    std::shared_ptr<RequestNode> CLinkedList::get_next() {
        D("Current CLinkedList size: " << this->length);
        this->current_position = this->current_position->next;
        return this->current_position->node;
    }

    std::unique_ptr<LinkedRequest> RequestNode::send_request() {
        std::unique_ptr<LinkedRequest> to_send = std::move(this->_node->unsent_request);
        this->_node = std::make_unique<ReqNode>(); // this will instantiate an empty vector
        this->sent = true;
        return to_send;
    };

    request BatchRequest::get_current() {
        return this->current_request->get_base_copy();
    }

    request BatchRequest::get_next() {
        // the final request will remove itself from CLinkedList upon finishing
        D("Retrieving next request, curent ready request size: " << this->parent_requests->size());
        if (this->parent_requests->finished()) {
            return BatchRequest::FINISHED;
        }
        this->current_results = this->parent_requests->get_next();
        this->current_request = this->current_results->send_request();
        return this->current_request->get_base_copy();
    }

    bool BatchRequest::insert_result(json_ptr result) {
        D("Batch Request Receiving result");
        D("Json vector length" << this->current_results->_node->request_results.size());
        if (this->current_request->children.size() != 0) {
            result->insert(result->end(), simdjson::SIMDJSON_PADDING, '\0');
            // only save the response if required for dependent requests
            this->current_results->_node->request_results.push_back(std::move(result));
        }
        // if the request has finished we should insert all ready child requests
        if (!this->current_request->update_base()) {
            for (auto& child : this->current_request->children) {
                D("Checking child");
                if (child->sent) {
                    throw std::logic_error("Child request somehow executed before parent completion");
                }
                if (child->_node->unsent_request->ready()) {
                    D("Inserting Child: (game: " << child->_node->unsent_request->_game << ", endpoint: " << child->_node->unsent_request->_endpoint << ", endpoint_method: "  << child->_node->unsent_request->_endpoint_method << ")");
                    try {
                        child->_node->unsent_request->fill_request();
                        child->_node->unsent_request->update_base();
                        this->parent_requests->insert(child);
                    } catch (simdjson::simdjson_error& err) {
                        std::cerr << "Json parsing error" << err.what() << std::endl;
                    }
                }
            }
            D("Removing executed request and inserting all ready dependent requests");
            this->parent_requests->remove();
            return false;
        }
        return true;
    }

    BatchRequest::BatchRequest(const std::vector<std::shared_ptr<RequestNode>>& requests) {
        for (const auto& req : requests) {
            this->parent_requests->insert(req);
        }
    }

    json_access_info iter_access_info::get_base() const {
        return json_access_info(this->keys);
    }
}
