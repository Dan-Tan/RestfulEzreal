#pragma once

#include <thread>
#include <mutex>
#include <queue>
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

        public:
            RequestSender(std::shared_ptr<client::RiotApiClient> client, std::string& output_directory);
            ~RequestSender();

            void set_client(std::shared_ptr<client::RiotApiClient> client) {this->underlying_client = client;};

            void add_request(request task) {
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    this->simple_requests.push(task);
                }
                condition.notify_one();
            };

            void add_batch_request(std::shared_ptr<BatchRequest> batch_task) {
                {
                    std::unique_lock<std::mutex>  lock(queue_mutex);
                    this->linked_requests.push(batch_task);
                }
                condition.notify_one();
            };

        private: // methods
            void worker();
            void write_response_file(const request& task, std::shared_ptr<Json::Value> result);

            std::shared_ptr<Json::Value> Send_Request(request& task);
            void Send_Batch_Request(std::shared_ptr<BatchRequest> task);

            std::shared_ptr<Json::Value> Send_Riot(request& task);
            std::shared_ptr<Json::Value> Send_LOL(request& task);
            std::shared_ptr<Json::Value> Send_VAL(request& task);
            std::shared_ptr<Json::Value> Send_TFT(request& task);
            std::shared_ptr<Json::Value> Send_LOR(request& task);
    };
}
