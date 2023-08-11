#pragma once

#include <list>
#include <cstring>
#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <json/json.h>

#define P_NAME_LENGTH 32
#define KEY_LENGTH 64
#define P_INPUT_LENGTH 256

namespace restfulEz {
    
    // Param name container
    typedef struct P_NAME {
        char name[P_NAME_LENGTH] = { 0 };
        operator char* () { return name; }
        P_NAME() {};
        P_NAME(const char* str) {
            strncpy(name, str, P_NAME_LENGTH);
        };
    }P_NAME;
    
    // container for inputs params
    typedef struct PARAM_CONT {
        char param[P_INPUT_LENGTH] = { 0 };
        operator char*() { return param; };
        operator std::string() { return std::string(param); };
        PARAM_CONT() {};
        PARAM_CONT(const char* str) {
            strncpy(param, str, P_INPUT_LENGTH);
        };
        void  operator=(const char* str) {
            strncpy(param, str, P_INPUT_LENGTH);
        }
    }PARAM_CONT;
    
    // container for json keys
    typedef struct KEY_CONT {

        char key[KEY_LENGTH] = { 0 };

        operator char*() {return key;};
        operator std::string() {return std::string(key);};
        KEY_CONT() {};

        void operator=(const char* str) {
            strncpy(key, str, KEY_LENGTH);
        };
    } KEY_CONT;
    

    // most primitive request struct, enough information to send a single request
    typedef struct request {
        int _game;
        int _endpoint;
        int _endpoint_method;

        std::vector<PARAM_CONT> params;
        std::vector<P_NAME> optional_names;
        std::vector<PARAM_CONT> optional_inputs;

        std::shared_ptr<Json::Value> response;
        
        request() {};
        request(const request& copy) { // copy constructor
            this->_game = copy._game;
            this->_endpoint = copy._endpoint;
            this->_endpoint_method = copy._endpoint_method;

            this->params = copy.params;
            this->optional_names = copy.optional_names;
            this->optional_inputs = copy.optional_inputs;

            this->response = copy.response;
        };
    } request;

    struct LinkedRequest;
    union RequestNode;

    // defines how to get param information from parent request
    typedef struct json_access_info {
        std::vector<KEY_CONT> keys;
        const Json::Value* index_json(const Json::Value* to_index) const;

        PARAM_CONT get_param(std::shared_ptr<Json::Value> response) const;
    } json_access_info;
    
    typedef struct iter_access_info : json_access_info {
        std::size_t iter_limit = 0; // 0 - iter over all
        json_access_info access_after_iter;

        std::vector<PARAM_CONT> get_params(std::shared_ptr<Json::Value> response) const; 
        std::vector<PARAM_CONT> get_params(std::vector<std::shared_ptr<Json::Value>> responses) const; 
    } iter_access_info;

    typedef struct base_link {
        std::shared_ptr<RequestNode> parent;
        std::vector<std::size_t> param_indices;
        bool filled = false;

        virtual bool get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) = 0;
        virtual bool get_dependencies(std::vector<PARAM_CONT>& to_fill) = 0;
    } base_link;
    
    // describes the relationship between parent and child request (owned by child)
    typedef struct request_link : public base_link{
        std::vector<json_access_info> link_info;
        bool get_dependencies(std::vector<PARAM_CONT>& to_fill) override;
    } request_link;
    
    // describes the relation ship between parent and child request when the child needs to fill a vector of dependencies
    typedef struct iter_request_link : public base_link {
        std::vector<iter_access_info> iter_link_info;
        bool get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fills) override;
    } iter_request_link;

    typedef struct response {
        std::vector<std::shared_ptr<Json::Value>> responses;
    } response;

    typedef struct LinkedRequest : request {
        std::vector<request_link> dependencies;

        bool fill_request();
        virtual bool ready(); // check each link if completed
    } LinkedRequest;

    typedef struct IterativeRequest : LinkedRequest {
        std::vector<iter_request_link> iter_dependencies;
        std::vector<std::vector<PARAM_CONT>>  param_fields;
        std::vector<std::size_t> param_indices;

        bool iter_fill_request();
        bool ready() override;
    } IterativeRequest;
    
    // A node in the Batch request tree representing both an unsent or sent request;
    typedef union RequestNode {
        std::unique_ptr<LinkedRequest> unsent_request;
        std::vector<std::shared_ptr<Json::Value>> request_results;
        RequestNode(std::unique_ptr<LinkedRequest> unsent) {
            this->unsent_request = std::move(unsent);
        };
        std::unique_ptr<LinkedRequest> send_request() { // when the request is send remove the unique ptr to the request
            std::unique_ptr<LinkedRequest> to_send = std::move(this->unsent_request);
            this->request_results = std::vector<std::shared_ptr<Json::Value>>();
            return to_send;
        };
    } RequestNode;

    typedef struct ListNode {
        std::shared_ptr<ListNode> previous = nullptr;
        std::shared_ptr<RequestNode> node = nullptr;
        std::shared_ptr<ListNode> next = nullptr;
    } ListNode;

    class CLinkedList {
        private:
            std::shared_ptr<ListNode> beginning = nullptr;
            std::shared_ptr<ListNode> current_position = nullptr;
            std::shared_ptr<ListNode> end = nullptr;

            std::size_t length = 0;
         
        public:
            void insert(std::shared_ptr<RequestNode> to_insert);
            void remove();
            bool finished();
            std::shared_ptr<RequestNode> get_next();

            std::size_t size() const {return this->length;};
    };

    class BatchRequest {

        private:
            // only the Batch Request should have access to requests
            std::unique_ptr<CLinkedList> parent_requests; 
            std::shared_ptr<RequestNode> current_results = nullptr;
            std::unique_ptr<LinkedRequest> current_request = nullptr;

        public:
            BatchRequest();
            ~BatchRequest();

            request& get_next();
            void insert_result(std::shared_ptr<Json::Value> result);
            // request returned when the batch request is finished
            static request FINISHED;

        private:
            bool reformat_parents();

    };

}
