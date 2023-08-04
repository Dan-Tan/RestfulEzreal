#pragma once

#include <cstring>
#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <json/json.h>

namespace restfulEz {

    typedef struct P_NAME {
        char name[32] = { 0 };
        operator char* () { return name; }
        P_NAME() {};
        P_NAME(const char* str) {
            strncpy(name, str, 32);
        };
    }P_NAME;

    typedef struct PARAM_CONT {
        char param[256] = {0};
        operator char*() { return param; };
        operator std::string() { return std::string(param); };
        PARAM_CONT() {};
        PARAM_CONT(const char* str) {
            strncpy(param, str, 256);
        };
        void  operator=(const char* str) {
            strncpy(param, str, 256);
        }
    }PARAM_CONT;

    typedef struct request {
        int _game;
        int _endpoint;
        int _endpoint_method;

        std::vector<PARAM_CONT> params;
        std::vector<P_NAME> optional_names;
        std::vector<PARAM_CONT> optional_inputs;

        Json::Value response;
        
        request() {};
        request(const request& copy) {*this = copy;};
    }request;

    typedef struct param_dependence_info {
        // does this link describe an iterative dependence
        bool iterative = false;
        // describes how to obtain the parameter from the Json object
        std::vector<PARAM_CONT> json_keys;
        // describes where to insert the obtained parameter
        std::size_t param_index;
        // describes which json keys precede the array to iterate over and json keys after the iterator
        std::size_t iter_index;
        // how many iterations, 0 == all
        std::size_t iter_limit = 0;
    } param_dependence_info;

    struct Linked_Request;

    typedef struct request_link {
        // unique pointer to child/parent node
        std::shared_ptr<Linked_Request> request;
        // pointer to dependence info
        std::vector<param_dependence_info> dependence_information;
    } request_link;

    typedef struct Linked_Request : public request {

        Linked_Request(const std::vector<std::size_t>& required_params);
        Linked_Request(const request& base, const std::vector<std::size_t>& required_params);
        ~Linked_Request() = default;
        
        std::vector<request_link> child_links;
        const std::vector<std::size_t> required_dependencies; // {0, 1, 0, 0} requires '2nd' param
        std::vector<std::size_t> completed_dependencies;
        // pass parameters from Json to 
        void fill_dependencies();
        // check if this request is ready to send
        bool ready();
    } Linked_Request;

    typedef struct Iterative_Request : public Linked_Request {

        Iterative_Request(const std::vector<std::size_t>& required_params) : Linked_Request(required_params) {};
        Iterative_Request(const request& base, const std::vector<std::size_t>& required_params) : Linked_Request(base, required_params) {};
        ~Iterative_Request() = default;
        // vector of params to write into the request sub class (avoid having multiple requests objects)
        std::vector<std::vector<PARAM_CONT>> params_from_parent;
        // where to place the params
        std::vector<std::size_t> param_indices;
    } Iterative_Request;
    
    // Batch requests are circular linked lists
    typedef struct Batch_Request {
        std::shared_ptr<Batch_Request> previous;
        std::shared_ptr<Linked_Request> request_node;
        std::shared_ptr<Batch_Request> next;
    } Batch_Request;
    
    // insert before current_node
    void insert_request(std::shared_ptr<Batch_Request> current_node, std::shared_ptr<Linked_Request> child_node);
    // remove current node and return next inline
    std::shared_ptr<Batch_Request> remove_request(std::shared_ptr<Batch_Request> node);
}
