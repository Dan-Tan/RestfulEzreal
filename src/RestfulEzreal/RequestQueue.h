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
            std::queue<request> simple_requests; 

            // Requests with a chain of dependecies
            std::queue<std::shared_ptr<Batch_Request>> linked_requests; 

        public:
            RequestSender();
            ~RequestSender();

        private: // methods
            void worker();
            void write_response_file(const request& task);

            void Send_Request(request& task);
            void Send_Batch_Request(std::shared_ptr<Batch_Request> task);

            void Send_Riot(request& task);
            void Send_LOL(request& task);
            void Send_VAL(request& task);
            void Send_TFT(request& task);
            void Send_LOR(request& task);
    };
}
