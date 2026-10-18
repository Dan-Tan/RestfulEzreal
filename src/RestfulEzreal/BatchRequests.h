#pragma once

#include <list>
#include <cstring>
#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <deque>
#include <unordered_set>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <iostream>

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
            if (str) {
                strncpy(name, str, P_NAME_LENGTH - 1);
                name[P_NAME_LENGTH - 1] = '\0';
            }
        };
    } P_NAME;
    
    // container for inputs params
    typedef struct PARAM_CONT {
        char param[P_INPUT_LENGTH] = { 0 };
        operator char*() { return param; };
        operator const char*() const { return param; };
        operator std::string() const { return std::string(param); };
        PARAM_CONT() { param[0] = '\0'; };
        PARAM_CONT(const char* str) {
            if (str) {
                strncpy(param, str, P_INPUT_LENGTH - 1);
                param[P_INPUT_LENGTH - 1] = '\0';
            }
        };
        PARAM_CONT(const std::string& str) : PARAM_CONT(str.c_str()) {}
        void operator=(const char* str) {
            if (str) {
                strncpy(param, str, P_INPUT_LENGTH - 1);
                param[P_INPUT_LENGTH - 1] = '\0';
            } else {
                param[0] = '\0';
            }
        }
        bool operator==(const PARAM_CONT& other) const {
            return std::strncmp(param, other.param, P_INPUT_LENGTH) == 0;
        }
    } PARAM_CONT;
    
    // container for json keys
    typedef struct KEY_CONT {
        char key[KEY_LENGTH] = { 0 };

        operator char*() {return key;};
        operator const char*() const {return key;};
        operator std::string() const {return std::string(key);};
        KEY_CONT() { key[0] = '\0'; };
        KEY_CONT(const char* str) {
            if (str) {
                strncpy(key, str, KEY_LENGTH - 1);
                key[KEY_LENGTH - 1] = '\0';
            }
        };

        void operator=(const char* str) {
            if (str) {
                strncpy(key, str, KEY_LENGTH - 1);
                key[KEY_LENGTH - 1] = '\0';
            } else {
                key[0] = '\0';
            }
        };
    } KEY_CONT;

    // =========================================================================
    // EXECUTION TRACKER & LIFECYCLE MODEL
    // =========================================================================

    enum class NodeExecState {
        IDLE,                // Unconfigured or invalid parameters
        READY,               // Configured and ready to execute
        WAITING_FOR_PARENTS, // Batch submitted; waiting for parent dependencies
        QUEUED = WAITING_FOR_PARENTS, // Alias for dependency waiting
        EXECUTING,           // Currently dispatching requests
        COMPLETED,           // Finished all requests successfully
        FAILED               // Encountered HTTP or extraction error
    };

    struct NodeExecutionTracker {
        std::atomic<NodeExecState> state{NodeExecState::IDLE};
        std::atomic<int> processed_count{0};
        std::atomic<int> items_dispatched{0};
        std::atomic<int> total_count{0};
        std::atomic<int> items_total{0};
        std::string error_message;

        NodeExecutionTracker() = default;

        void reset(NodeExecState initial_state = NodeExecState::IDLE) {
            state.store(initial_state);
            processed_count.store(0);
            items_dispatched.store(0);
            total_count.store(0);
            items_total.store(0);
            error_message.clear();
        }

        void increment_dispatched() {
            processed_count++;
            items_dispatched++;
        }

        void set_total(int total) {
            total_count.store(total);
            items_total.store(total);
        }
    };

    // =========================================================================
    // VALUE POOL MODEL
    // =========================================================================

    enum class PoolBindMode {
        CROSS, // Cartesian product across parameter pools (default)
        ZIP    // Lockstep positional pairing across multi-value pools (1-element pools broadcast)
    };

    class ValuePool {
    private:
        std::vector<PARAM_CONT> values_;
        std::unordered_set<std::string> seen_;
        bool deduplicate_ = false;
        std::size_t max_capacity_ = 0; // 0 = unlimited
        std::function<bool(const std::string&)> filter_predicate_ = nullptr;

    public:
        ValuePool() = default;
        explicit ValuePool(bool deduplicate, std::size_t max_capacity = 0, std::function<bool(const std::string&)> filter = nullptr)
            : deduplicate_(deduplicate), max_capacity_(max_capacity), filter_predicate_(std::move(filter)) {}
        explicit ValuePool(const std::vector<PARAM_CONT>& initial_values, bool deduplicate = false)
            : deduplicate_(deduplicate) {
            push_many(initial_values);
        }

        void set_deduplicate(bool dedup) {
            deduplicate_ = dedup;
            if (deduplicate_ && seen_.empty() && !values_.empty()) {
                std::vector<PARAM_CONT> unique_vals;
                for (const auto& v : values_) {
                    std::string s(v.param);
                    if (seen_.insert(s).second) {
                        unique_vals.push_back(v);
                    }
                }
                values_ = std::move(unique_vals);
            }
        }
        bool is_deduplicated() const { return deduplicate_; }

        void set_max_capacity(std::size_t cap) {
            max_capacity_ = cap;
            if (max_capacity_ > 0 && values_.size() > max_capacity_) {
                values_.resize(max_capacity_);
            }
        }
        std::size_t max_capacity() const { return max_capacity_; }

        void set_filter(std::function<bool(const std::string&)> predicate) {
            filter_predicate_ = std::move(predicate);
        }

        // Built-in DuckDB pre-filters to avoid redundant HTTP calls
        static std::function<bool(const std::string&)> SkipExistingPlayersInDB();
        static std::function<bool(const std::string&)> SkipExistingMatchesInDB();
        static std::function<bool(const std::string&)> SkipExistingTimelinesInDB();

        // Construct a pool from a delimiter-separated string (e.g. "Faker, Chovy, ShowMaker")
        static std::shared_ptr<ValuePool> FromDelimitedString(const std::string& input, char delimiter = ',', bool deduplicate = true);

        bool push(const PARAM_CONT& val, bool allow_empty = false) {
            std::string s(val.param);
            if (!allow_empty && s.empty()) return false;
            if (max_capacity_ > 0 && values_.size() >= max_capacity_) return false;
            if (deduplicate_) {
                if (seen_.find(s) != seen_.end()) return false;
            }
            if (filter_predicate_ && !s.empty()) {
                if (!filter_predicate_(s)) return false;
            }
            if (deduplicate_) {
                seen_.insert(s);
            }
            values_.push_back(val);
            return true;
        }

        bool push(const std::string& val, bool allow_empty = false) {
            return push(PARAM_CONT(val.c_str()), allow_empty);
        }

        bool push(const char* val, bool allow_empty = false) {
            return push(PARAM_CONT(val ? val : ""), allow_empty);
        }

        void push_many(const std::vector<PARAM_CONT>& vals) {
            for (const auto& v : vals) {
                push(v);
            }
        }

        void clearValues() {
            values_.clear();
            seen_.clear();
        }

        std::size_t size() const { return values_.size(); }
        bool empty() const { return values_.empty(); }
        const PARAM_CONT& at(std::size_t index) const { return values_.at(index); }
        const PARAM_CONT& operator[](std::size_t index) const { return values_[index]; }
        const std::vector<PARAM_CONT>& values() const { return values_; }
    };

    // MOST PRIMITIVE REQUEST STRUCT, ENOUGH INFORMATION TO SEND A SINGLE REQUEST
        
    typedef struct request {
        int _game = -1;
        int _endpoint = -1;
        int _endpoint_method = -1;

        std::vector<PARAM_CONT> params;
        std::vector<P_NAME>     optional_names;
        std::vector<PARAM_CONT> optional_inputs;
        
        int* success_ptr = nullptr;

        bool same_endpoint(const request& other) const {
            bool same = true;
            same &= this->_game            == other._game;
            same &= this->_endpoint        == other._endpoint;
            same &= this->_endpoint_method == other._endpoint_method;
            return same;
        }
        
        request() {};
        request(const int game, const int endpoint, const int endpoint_method);
        request(const int game, const int endpoint, const int endpoint_method, const std::vector<PARAM_CONT>& pars, const std::vector<P_NAME>& opt_names, const std::vector<PARAM_CONT>& opt_inputs, int* succ_ptr = nullptr);
    } request;

    struct LinkedRequest;
    struct RequestNode;

    using raw_json = std::vector<char>;
    using json_ptr = std::unique_ptr<std::vector<char>>;

    // JSON ACCESS / SELECTOR INFO
        
    typedef struct json_access_info {
        std::vector<KEY_CONT> keys {};
        PARAM_CONT get_param(const raw_json& response) const;
        void extract_into_pool(const raw_json& response, ValuePool& pool, std::size_t per_response_limit = 1) const;
        void extract_into_pool(const std::vector<json_ptr>& responses, ValuePool& pool, std::size_t per_response_limit = 1) const;
    } json_access_info;
    
    typedef struct iter_access_info : json_access_info {
        std::size_t iter_limit = 0; // 0 - iter over all
        json_access_info access_after_iter;
        iter_access_info() : json_access_info(), access_after_iter() {}
        
        json_access_info get_base() const;
        std::vector<KEY_CONT> combined_keys() const;
        std::vector<PARAM_CONT> get_params(const raw_json& response) const; 
        std::vector<PARAM_CONT> get_params(const std::vector<json_ptr>& responses) const;
        void extract_into_pool(const raw_json& response, ValuePool& pool) const;
        void extract_into_pool(const std::vector<json_ptr>& responses, ValuePool& pool) const;
    } iter_access_info;


    // LINK BETWEEN REQUESTS & POOLS
    
    typedef struct base_link {
        std::shared_ptr<RequestNode> parent;
        std::vector<std::size_t> param_indices;
        bool filled = false;
        std::size_t consumed_results = 0;

        virtual ~base_link() = default;
        virtual bool get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) = 0;
        virtual bool get_dependencies(std::vector<PARAM_CONT>& to_fill) = 0;
        virtual bool populate_pools(std::vector<std::shared_ptr<ValuePool>>& pools, const std::vector<std::size_t>& fallback_indices, std::size_t& fallback_cursor) = 0;
    } base_link;
    
    // describes the relationship between parent and child request (owned by child)
    typedef struct request_link : public base_link {
        std::vector<json_access_info> link_info;
        bool get_dependencies(std::vector<PARAM_CONT>& to_fill) override;
        bool get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fill) override;
        bool populate_pools(std::vector<std::shared_ptr<ValuePool>>& pools, const std::vector<std::size_t>& fallback_indices, std::size_t& fallback_cursor) override;
    } request_link;
    
    // describes the relationship between parent and child request when the child extracts multiple items into a pool
    typedef struct iter_request_link : public base_link {
        std::vector<iter_access_info> iter_link_info;
        bool get_dependencies(std::vector<std::vector<PARAM_CONT>>& to_fills) override;
        bool get_dependencies(std::vector<PARAM_CONT>& to_fill) override {throw std::logic_error("Should not be called from iter_request link");};
        bool populate_pools(std::vector<std::shared_ptr<ValuePool>>& pools, const std::vector<std::size_t>& fallback_indices, std::size_t& fallback_cursor) override;
    } iter_request_link;


    // UNIFIED POOL-BACKED LINKED REQUEST STRUCTURE 

    typedef struct LinkedRequest : request {
        virtual ~LinkedRequest() = default;
        std::vector<request_link> dependencies;
        std::vector<iter_request_link> iter_dependencies;
        std::vector<std::shared_ptr<RequestNode>> children;

        // First-class ValuePool per parameter slot
        std::vector<std::shared_ptr<ValuePool>> param_pools;
        PoolBindMode bind_mode = PoolBindMode::CROSS;

        // Execution status tracker
        std::shared_ptr<NodeExecutionTracker> tracker = nullptr;

        // Legacy compatibility fields synced from param_pools
        std::vector<std::vector<PARAM_CONT>> param_fields;
        std::vector<std::size_t> param_indices;
        std::vector<std::size_t> progress;
        std::size_t zip_cursor = 0;
        bool pools_initialized = false;

        // Pool configuration helpers
        std::shared_ptr<ValuePool> get_or_create_pool(std::size_t param_idx);
        void set_param_pool(std::size_t param_idx, std::shared_ptr<ValuePool> pool);
        void set_bind_mode(PoolBindMode mode) { bind_mode = mode; }

        virtual bool fill_request();
        bool append_from_parents();
        bool all_parents_completed() const;
        bool can_stream_from_parents() const;
        // Advances to the next combination in param_pools; returns false when all combinations in the pool have been dispatched
        virtual bool update_base();
        virtual bool ready();

        request get_base_copy();

    protected:
        bool fill_req_priv();
        void ensure_pools_sized();
        bool apply_first_combination();
        void update_tracker_total();
    } LinkedRequest;

    // IterativeRequest is now unified with LinkedRequest (all requests are pool-backed)
    typedef struct IterativeRequest : LinkedRequest {
        IterativeRequest() = default;
        ~IterativeRequest() override = default;
    } IterativeRequest;


    struct ReqNode {
        std::unique_ptr<LinkedRequest> unsent_request = nullptr;
        std::vector<json_ptr> request_results;
        ReqNode(std::unique_ptr<LinkedRequest> unsent) : unsent_request(std::move(unsent)), request_results() {}
        ReqNode() : unsent_request(nullptr), request_results() {}
        ~ReqNode() = default;
    };
    
    // A node in the Batch request graph representing both an unsent or sent request
    typedef struct RequestNode {
        bool sent = false;
        bool in_progress = false;
        bool completed = false;
        std::unique_ptr<ReqNode> _node;
        std::shared_ptr<NodeExecutionTracker> tracker = nullptr;

        RequestNode(std::unique_ptr<LinkedRequest> unsent) : _node(std::make_unique<ReqNode>(std::move(unsent))) {}

        std::unique_ptr<LinkedRequest> send_request();
        ~RequestNode() = default;
    } RequestNode;


    // CONTAINER STRUCTURE FOR REQUEST EXECUTION  

    class CLinkedList {
        private:
            std::deque<std::shared_ptr<RequestNode>> queue_;
            std::shared_ptr<RequestNode> current_ = nullptr;
         
        public:
            CLinkedList() = default;
            ~CLinkedList() = default;
            void insert(std::shared_ptr<RequestNode> to_insert) {
                if (to_insert) queue_.push_back(to_insert);
            }
            void remove() {
                if (!queue_.empty()) queue_.pop_front();
                current_ = nullptr;
            }
            inline bool finished() const { return queue_.empty(); }
            std::shared_ptr<RequestNode> get_next() {
                if (queue_.empty()) return nullptr;
                current_ = queue_.front();
                return current_;
            }

            std::size_t size() const { return queue_.size(); }
    };

    // CLASS ENCODING ALL REQUESTS THAT MAKE UP A BATCH

    class BatchRequest {
        public:
            struct ActiveNodeSlot {
                std::shared_ptr<RequestNode> node_handle = nullptr;
                std::unique_ptr<LinkedRequest> req = nullptr;
                bool current_dispatched = false;
            };

        private:
            std::unique_ptr<CLinkedList> parent_requests = std::make_unique<CLinkedList>(); 
            std::vector<ActiveNodeSlot> active_slots_;
            std::size_t current_slot_idx_ = 0;
            std::atomic<bool> completed_{false};

            void promote_ready_nodes();
            void finalize_completed_slots();

        public:
            BatchRequest(const std::vector<std::shared_ptr<RequestNode>>& requests);
            ~BatchRequest() = default;

            request get_next();
            request get_current();
            request select_next_ready(const std::function<int(const request&)>& wait_time_fn);
            using json_ptr = std::unique_ptr<std::vector<char>>;
            bool insert_result(json_ptr result);
            bool is_completed() const { return completed_.load(); }
            std::size_t active_node_count() const { return active_slots_.size() + parent_requests->size(); }
            // request returned when the batch request is finished
            static request FINISHED;

        private:
            bool reformat_parents();
    };
}
