#include "RequestQueue.h"
#include "BatchRequests.h"
#include <stdexcept>
#include <filesystem>

#ifdef DEBUG_MODE
#include <iostream>
#define D(x) std::cerr << x <<'\n'
#else
#define D(x)  
#endif

#include <iostream>

namespace restfulEz {

    RequestSender::RequestSender(std::shared_ptr<client::RiotApiClient> client, std::string& output_dir)
        : output_directory(output_dir) {
        this->worker_thread = std::thread([this] {this->worker();});
        this->underlying_client = client;
        std::filesystem::create_directories(this->output_directory);
    }

    RequestSender::~RequestSender() {

        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->stop_execution = true;
        }

        condition.notify_all();
        this->worker_thread.join();
    }

    void RequestSender::worker() {
        while (true) {
            request task;
            std::shared_ptr<BatchRequest> b_task;
            bool iterative = false;

            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);

                this->condition.wait(lock, [this]() {
                        return this->stop_execution || !this->simple_requests.empty() || !this->linked_requests.empty();
                        });

                if (this->stop_execution && ( this->simple_requests.empty() || this->linked_requests.empty())) {
                    return;
                }

                if (!this->simple_requests.empty()) {
                    task = std::move(this->simple_requests.front());
                    this->simple_requests.pop();
                    iterative = false;
                } else if (!this->linked_requests.empty()) {
                    b_task = std::move(this->linked_requests.front());
                    this->linked_requests.pop();
                    iterative = true;
                } else {
                    throw std::logic_error("Request thread continued with no tasks");
                }
            };
            if (iterative) {
                this->Send_Batch_Request(b_task);
            } else {
                this->Send_Request(task);
            }
        }
    }

    static const char* GAMES_[] = { "Riot", "League of Legends", "TeamfightTactics", "Legends of Runeterra", "Valorant"};

    static const char GAME_ENDPOINTS_[5][9][18] = {
        { "Account" },
        { "Champion Mastery", "Champion Rotation", "Clash", "League", "Challenges", "Status", "Match", "Summoner", "Spectator" },
        { "League", "Match", "Status", "Summoner" },
        { "Match", "Ranked", "Status" },
        { "Content", "Match", "Ranked", "Status" }
    };


    static const char ENDPOINT_METHODS_[5][9][7][24] = {
    {
        {"By Puuid", "By Riot ID", "By Game"}
    },
    {
        { "By Summoner ID", "By Summoner By Champion", "Top By Summoner", "Scores By Summoner" },
        { "Rotation" },
        {"By Summoner ID", "By Team", "Tourament By Team", "By Tournament"},
        {"Challenger", "Grandmaster", "Master", "By Summoner ID", "By League ID", "Specific League", "Experimental"},
        {"Configuration", "Percentiles", "Challenge Configuration", "Challenge Leaderboard", "Challenge Percentiles", "By Puuid"},
        {"v4 (recommended)", "v3"},
        {"By Match ID", "Timeline", "By Puuid"},
        {"By RSO Puuid", "By Account ID", "By Name", "By Puuid", "By Summoner ID"},
        {"By Summoner ID", "Featured Games"}
    },
    {
        { "Challenger", "Grandmaster", "Master", "By Summoner ID", "By League ID", "Queue Top", "By Tier Division"},
        { "By Puuid", "By Match ID"},
        {"v1"},
        {"By Account", "By Name", "By Puuid", "By Summoner ID"}
    },
    {
        { "By Puuid", "By Match ID"},
        { "Leaderboards"},
        {"v1"}
    },
    {
        { "Content"},
        { "By Match ID", "By Puuid", "By  Queue"},
        {"By Act"},
        {"Status"}
    }
    };

    using json_ptr = std::unique_ptr<std::vector<char>>;

    json_ptr RequestSender::Send_Request(request& task) {

        this->recent_request = std::string(GAMES_[task._game]) + " | " + std::string(GAME_ENDPOINTS_[task._game][task._endpoint]) + " | " + std::string(ENDPOINT_METHODS_[task._game][task._endpoint][task._endpoint_method]);
        if (this->recent_params.size() != 0) {
            this->recent_params.clear();
        }
        for (PARAM_CONT& param : task.params) {
            this->recent_params.emplace_back(param.param);
        }

        json_ptr response;
        switch (task._game) {
            case 0: // RIOT
                response = this->Send_Riot(task); break;
            case 1: // LEAGUE OF LEGENDS
                response = this->Send_LOL(task); break;
            case 2: // TEAMFIGHT TACTICS
                response = this->Send_TFT(task); break;
            case 3: // VALORANT
                response = this->Send_VAL(task); break;
            case 4: // LEGENDS OF RUNETERR
                response = this->Send_LOR(task); break;
            default:
                throw std::invalid_argument("Invalid Game Index");
        }
        this->write_response_file(task, *response);
        return std::move(response);
    }

    json_ptr RequestSender::Send_Riot(request& task) {
        std::vector<PARAM_CONT> & params = task.params;

        switch (task._endpoint_method) {
            case 0:
                return std::move(this->underlying_client->Account.by_puuid(params.at(0), params.at(1))); break;
            case 1:
                return std::move(this->underlying_client->Account.by_riot_id(params.at(0), params.at(1), params.at(2))); break;
            case 2:
                return std::move(this->underlying_client->Account.by_game(params.at(0), params.at(1), params.at(2))); break;
            default:
                throw std::move("Given ivnalid endpoint method index for Riot Accout endpoint");
        }
    }

    using raw_json = std::vector<char>;

    void RequestSender::write_response_file(const request& task, const raw_json& result) {
        // write more informative file names
        const int game = task._game;
        const int endpoint = task._endpoint;
        const int endpoint_method = task._endpoint_method;

        static int counter = 0; // change as well

        std::string file_output = this->output_directory + std::string("req") + std::to_string(game) + std::to_string(endpoint)+ std::to_string(endpoint_method) + std::to_string(counter);
        file_output.append(".json");

        std::ofstream output;

        output.open(file_output);
        output << result.data();
        output.close();

        counter++;
    }

    void RequestSender::Send_Batch_Request(std::shared_ptr<BatchRequest> batch) {

        // assume all tasks in the linked are ready to execute
        bool not_finished = true;
        json_ptr result;

        request next = batch->get_next();
        while (!next.same_endpoint(BatchRequest::FINISHED)) {
            D("Sending Request (game: " << next._game << ",endpoint: " << next._endpoint << ", method: " << next._endpoint_method << ")");
            result = this->Send_Request(next);
            while (batch->insert_result(std::move(result))) {
                next = batch->get_current();
                this->Send_Request(next);
            }
            next = batch->get_next();
        }

        D("Batch Request finished executing");
    }
}
