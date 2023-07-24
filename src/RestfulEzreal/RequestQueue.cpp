#include "RequestQueue.h"
#include "BatchRequests.h"

namespace restfulEz {

    void RequestSender::worker() {
        while (true) {
            request task;

            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);

                this->condition.wait(lock, [this]() {
                        return this->stop_execution || !this->simple_requests.empty() || !this->linked_requests.empty();
                        });

                if (this->stop && ( this->simple_requests.empty() || this->linked_requests.empty()) {
                    return;
                }

                if (!this->simple_requests.empty()) {
                    task = std::move(this->simple_requests.front());
                    this->simple_requests.pop();
                } else if (!this->linked_requests.empty()) {
                    task = std::move(this->simple_requests.front());
                    this->linked_requests.pop();
                } else {
                    throw std::logic_error("Request thread continued with no tasks");
                }
            };
            this->Send_Request(task);
        }
    }

    void RequestSender::Send_Request(request& task) {
        switch (task._game) {
            case 0: // RIOT
                this->Send_Riot(task); break;
            case 1: // LEAGUE OF LEGENDS
                this->Send_LOL(task); break;
            case 2: // TEAMFIGHT TACTICS
                this->Send_TFT(task); break;
            case 3: // VALORANT
                this->Send_VAL(task); break;
            case 4: // LEGENDS OF RUNETERR
                this->Send_LOR(task); break;
            default:
                throw std::invalid_argument("Invalid Game Index");
        }
        this->write_response_file(task);
    }

    void RequestSender::Send_Riot(request& task) {
        std::vector<PARAM_CONT> & params = task.params;

        switch (task._endpoint_method) {
            case 0:
                task.response = this->underlying_client->Account.by_puuid(params.at(0), params.at(1)); break;
            case 1:
                task.response = this->underlying_client->Account.by_riot_id(params.at(0), params.at(1), params.at(2)); break;
            case 2:
                task.response = this->underlying_client->Account.by_game(params.at(0), params.at(1), params.at(2)); break;
        }
    }

    void RequestSender::write_response_file(const request& task) {
        // write more informative file names
        const int game = task._game;
        const int endpoint = task._endpoint;
        const int endpoint_method = task._endpoint_method;

        const Json::Value& response = task.response;

        static int counter = 0; // change as well

        std::string file_output = this->output_directory + std::string("req") + std::to_string(game) + std::to_string(endpoint)+ std::to_string(endpoint_method) + std::to_string(counter);
        file_output.append(".json");

        Json::StreamWriterBuilder builder;
        std::ofstream output;

        output.open(file_output);
        output << Json::writeString(builder, response);
        output.close();

        counter++;
    }

    void RequestSender::Send_Batch_Request(std::shared_ptr<Batch_Request> batch) {

        // assume all tasks in the linked are ready to execute
        bool finished = false;

        while(batch) {
            this->Send_Request(*(batch->request_node));
            batch->request_node->fill_dependencies();
            for (auto& link : batch->request_node->child_links) {
                if (link.request->ready()) {
                    insert_request(batch, link.request);
                }
            }
            batch = remove_request(batch);
        }
    }
}
