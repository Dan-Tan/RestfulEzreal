#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <filesystem>
#include <condition_variable>
#include "BatchRequests.h"
#include "client/client.h"

namespace restfulEz {

    class RequestSender {
        /* 
         * RequestSender is intended to be the class handling the queue of request submitted by the user. 
         * Most API keys have strict rate limits that make sending requests using multi-threaded nearly pointless
         * The current implementation should be easy to implement with multithreading by inheriting from this classes
        */ 
        private: // fields
            // worker thread, mutex and condition
            std::thread worker_thread;
            std::mutex queue_mutex;
            std::condition_variable condition;

            // Stop execution
            bool stop_execution = false;

            // output information
            std::string output_directory;

            // Underlying client used to send requests
            std::shared_ptr<client::RiotApiClient> underlying_client = nullptr;

            // Simple requests that do not provide nor have dependecies
            std::queue<request> simple_requests = {}; 

            // Requests with a chain of dependecies
            std::queue<std::shared_ptr<BatchRequest>> linked_requests; 
            
            // generic work
            std::queue<std::function<void()>> work;

            // test display lock vars
            bool test_in_progress                      = false;
            std::array<int,    16> region_avail = { 0 };
            static constexpr std::array<char[5], 16> regions      = {"BR1" ,"EUN1" ,"EUW1" ,"JP1" ,"KR" ,"LA1" ,"LA2" ,"NA1" ,"OC1" ,"TR1" ,"RU" ,"PH2" ,"SG2" ,"TH2" ,"TW2" ,"VN2"};

        public:
            RequestSender();
            ~RequestSender();

            void stop() {this->stop_execution = true;}
            void start() {this->stop_execution = false;}

            void set_client(std::shared_ptr<client::RiotApiClient> client) {this->underlying_client = client;};
            void set_output_directory(const std::string& output) { this->output_directory = output; std::filesystem::create_directories(this->output_directory);}

            void add_request(request task);
            void add_batch_request(std::shared_ptr<BatchRequest> batch_task);
            void add_generic(std::function<void()> generic_work);

            int get_generic_size() {return this->work.size();};
            int get_simple_size() {return this->simple_requests.size();};
            int get_batch_size() {return this->linked_requests.size();};
            
            // for start up display
            int region_test_display();
            void test_regions();

            std::string recent_request = "No Requests Sent.";
            std::vector<std::string> recent_params = {};

        private: // methods
            void worker();

            using raw_json = std::vector<char>;
            void write_response_file(const request& task, const raw_json& result);

            using json_ptr = std::unique_ptr<std::vector<char>>;

            json_ptr Send_Request(request& task);
            void Send_Batch_Request(std::shared_ptr<BatchRequest> task);

            json_ptr Send_Riot(request& task);
            json_ptr Send_LOL(request& task);
            json_ptr Send_VAL(request& task);
            json_ptr Send_TFT(request& task);
            json_ptr Send_LOR(request& task);
    };
}
