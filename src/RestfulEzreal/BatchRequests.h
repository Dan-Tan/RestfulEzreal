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

    // DIFFERENT CHAR[] containers for use in IMGUI::INPUTTEXT() to avoid strings
    
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
        KEY_CONT(const char* str) {
            strncpy(key, str, KEY_LENGTH);
        };

        void operator=(const char* str) {
            strncpy(key, str, KEY_LENGTH);
        };
    } KEY_CONT;



    
    // MOST PRIMITIVE REQUEST STRUCT, ENOUGH INFORMATION TO SEND A SINGLE REQUEST
        
    typedef struct request {
        int _game;
        int _endpoint;
        int _endpoint_method;

        std::vector<PARAM_CONT> params;
        std::vector<P_NAME> optional_names;
        std::vector<PARAM_CONT> optional_inputs;

        Json::Value response;

        bool same_endpoint(const request& other) {
            bool same = true;
            same &= this->_game == other._game;
            same &= this->_endpoint == other._endpoint;
            same &= this->_endpoint_method == other._endpoint_method;
            return same;
        }
        
        request() {};
        request(const int game, const int endpoint, const int endpoint_method);
    } request;

    struct LinkedRequest;
    struct RequestNode;

    // JSON ACCESS INFO
        
    typedef struct json_access_info {
        std::vector<KEY_CONT> keys;
        const Json::Value* index_json(const Json::Value* to_index) const;

        PARAM_CONT get_param(std::shared_ptr<Json::Value> response) const;
    } json_access_info;
    
    typedef struct iter_access_info : json_access_info {
        std::size_t iter_limit = 0; // 0 - iter over all
        json_access_info access_after_iter;
        
        json_access_info get_base() const;
        std::vector<PARAM_CONT> get_params(std::shared_ptr<Json::Value> response) const; 
        std::vector<PARAM_CONT> get_params(std::vector<std::shared_ptr<Json::Value>> responses) const; 
    } iter_access_info;




    // LINK BETWEEN REQUESTS
    
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
        bool get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) override {throw std::logic_error("Should not be callws from request link class");};
    } request_link;
    
    // describes the relationship between parent and child request when the child needs to fill a vector of dependencies
    typedef struct iter_request_link : public base_link {
        std::vector<iter_access_info> iter_link_info;
        bool get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fills) override;
        bool get_dependencies(std::vector<PARAM_CONT>& to_fill) override {throw std::logic_error("Should not be called from iter_request link");};
    } iter_request_link;



    // TOP LEVEL INDIVIDUAL LINKED REQUEST STRUCTURE 

    typedef struct LinkedRequest : request {
        std::vector<request_link> dependencies;
        std::vector<std::shared_ptr<RequestNode>> children;

        bool fill_request();
        // Non-iterative requests only send one request so when asked to update they will respond with completed
        virtual bool update_base() {return false;}
        virtual bool ready();
    } LinkedRequest;

    typedef struct IterativeRequest : LinkedRequest {
        std::vector<iter_request_link> iter_dependencies;
        std::vector<std::vector<PARAM_CONT>>  param_fields;
        std::vector<std::size_t> param_indices;
        
        // response false when the Iterative request has been completed
        bool update_base() override;
        bool ready() override;
        bool iter_fill_request();

        std::vector<std::size_t> progress;
    } IterativeRequest;

    typedef union ReqNode {
        std::unique_ptr<LinkedRequest> unsent_request;
        std::vector<std::shared_ptr<Json::Value>> request_results;
        ReqNode(std::unique_ptr<LinkedRequest> unsent) : unsent_request(std::move(unsent)) {};
        ~ReqNode() {};
    } ReqNode;
    
    // A node in the Batch request tree representing both an unsent or sent request;
    typedef struct RequestNode {
        bool sent = false;
        std::unique_ptr<ReqNode> _node;
        RequestNode(std::unique_ptr<LinkedRequest> unsent) : _node(std::make_unique<ReqNode>(std::move(unsent))) {};

        std::unique_ptr<LinkedRequest> send_request() { // when the request is send remove the unique ptr to the request
            std::unique_ptr<LinkedRequest> to_send = std::move(this->_node->unsent_request);
//            this->_node->request_results = {};
            this->sent = true;
            return std::move(to_send);
        };
        ~RequestNode() {
            // manually deallocate if the request was never sent
            if (!this->sent) {
                this->_node->unsent_request.reset();
            }
        }
    } RequestNode;


    // LINKED LIST REQUEST STRUCTURE FOR REQUEST EXECUTION  
    
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
            CLinkedList() = default;
            ~CLinkedList() = default;
            void insert(std::shared_ptr<RequestNode> to_insert);
            void remove();
            inline bool finished() {return this->length == 0;};
            std::shared_ptr<RequestNode> get_next();

            std::size_t size() const {return this->length;};
    };

    // CLASS ENCODING ALL REQUESTS THAT MAKE UP A BATCH

    class BatchRequest {

        private:
            // only the Batch Request should have access to requests
            std::unique_ptr<CLinkedList> parent_requests = std::make_unique<CLinkedList>(); 
            std::shared_ptr<RequestNode> current_results = nullptr;
            std::unique_ptr<LinkedRequest> current_request = nullptr;

        public:
            BatchRequest(const std::vector<std::shared_ptr<RequestNode>>& requests);
            ~BatchRequest() = default;

            request& get_next();
            bool insert_result(const Json::Value& result);
            // request returned when the batch request is finished
            static request FINISHED;

        private:
            bool reformat_parents();

    };
}
