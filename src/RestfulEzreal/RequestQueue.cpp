#include "RequestQueue.h"
#include "BatchRequests.h"
#include "FileManager.h"
#include "db/DatabaseManager.h"
#include "imgui.h"
#include "utils/utils.h"
#include "simdjson.h"
#include <stdexcept>
#include <filesystem>
#include <chrono>
#include <fstream>

#define NUM_REGIONS 16

#ifdef DEBUG_MODE
#include <iostream>
#define D(x) std::cerr << x <<'\n'
#else
#define D(x)  
#endif

#include <iostream>

namespace restfulEz {

    RequestSender::RequestSender() {
        this->worker_thread = std::thread([this] {this->worker();});
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
            std::function<void()> generic_work;
            bool iterative = false;
            bool singular = false;

            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);

                this->condition.wait(lock, [this]() {
                        return this->stop_execution || !this->simple_requests.empty() || !this->linked_requests.empty() || !this->work.empty();
                });

                if (this->stop_execution && ( this->simple_requests.empty() || this->linked_requests.empty() || this->work.empty())) {
                    return;
                }

                if (!this->work.empty()) {
                    generic_work = std::move(this->work.front());
                    this->work.pop();
                    iterative = false; singular = false;
                } else if (!this->simple_requests.empty()) {
                    task = std::move(this->simple_requests.front());
                    this->simple_requests.pop();
                    iterative = false; singular = true;
                } else if (!this->linked_requests.empty()) {
                    b_task = std::move(this->linked_requests.front());
                    this->linked_requests.pop();
                    iterative = true; singular = false;
                } else {
                    throw std::logic_error("Request thread continued with no tasks");
                }
            };

            try {
                if (iterative) {
                    this->Send_Batch_Request(b_task);
                } else if (singular) {
                    this->Send_Request(task);
                } else if (generic_work) {
                    generic_work();
                }
            } catch (const std::exception& ex) {
                std::cerr << "[RequestSender] Exception in background worker: " << ex.what() << std::endl;
            } catch (...) {
                std::cerr << "[RequestSender] Unknown exception in background worker." << std::endl;
            }
        }
    }

    void RequestSender::add_request(request task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            this->simple_requests.push(task);
        }
        condition.notify_one();
    }

    void RequestSender::add_batch_request(std::shared_ptr<BatchRequest> batch_task) {
        {
            std::unique_lock<std::mutex>  lock(queue_mutex);
            this->linked_requests.push(batch_task);
        }
        condition.notify_one();
    }

    void RequestSender::add_generic(std::function<void()> generic_work) {
        {
            std::unique_lock<std::mutex>  lock(queue_mutex);
            this->work.push(generic_work);
        }
        condition.notify_one();
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

    static bool check_failure_response(const std::vector<char>& response) {
        if (response.size() < 8) return false;
        std::string_view sv(response.data(), std::min<size_t>(response.size(), 40));
        return (sv.find("\"status\"") != std::string_view::npos);
    }

    json_ptr RequestSender::Send_Request(request& task) {
        if (!this->underlying_client) {
            std::cerr << "[RequestSender] RiotApiClient is not initialized." << std::endl;
            if (task.success_ptr) *task.success_ptr = 2;
            return nullptr;
        }

        {
            std::lock_guard<std::mutex> lock(this->recent_mutex);
            try {
                this->recent_request = std::string(GAMES_[task._game]) + " | " + std::string(GAME_ENDPOINTS_[task._game][task._endpoint]) + " | " + std::string(ENDPOINT_METHODS_[task._game][task._endpoint][task._endpoint_method]);
            } catch (...) {
                this->recent_request = "Unknown Request";
            }

            this->recent_params.clear();
            for (PARAM_CONT& param : task.params) {
                this->recent_params.emplace_back(param.param);
            }
        }


        json_ptr response = nullptr;
        try {
            switch (task._game) {
                case 0: // RIOT
                    response = this->Send_Riot(task); break;
                case 1: // LEAGUE OF LEGENDS
                    response = this->Send_LOL(task); break;
                case 2: // TEAMFIGHT TACTICS
                    response = this->Send_TFT(task); break;
                case 3: // VALORANT
                    response = this->Send_VAL(task); break;
                case 4: // LEGENDS OF RUNETERRA
                    response = this->Send_LOR(task); break;
                default:
                    std::cerr << "[RequestSender] Invalid Game Index: " << task._game << std::endl;
                    if (task.success_ptr) *task.success_ptr = 2;
                    return nullptr;
            }
        } catch (const std::exception& ex) {
            std::cerr << "[RequestSender] Exception sending request: " << ex.what() << std::endl;
            if (task.success_ptr) *task.success_ptr = 2;
            return nullptr;
        }

        bool failure = (!response || response->empty() || check_failure_response(*response));
        if (task.success_ptr) {
            *task.success_ptr = failure ? 2 : 1;
        }
        if (!failure && response && !response->empty()) {
            this->write_response_file(task, *response);
        }
        return response;
    }


    json_ptr RequestSender::Send_Riot(request& task) {
        std::vector<PARAM_CONT>& params = task.params;
        if (!this->underlying_client) return nullptr;

        switch (task._endpoint_method) {
            case 0:
                if (params.size() < 2) throw std::invalid_argument("Account by_puuid requires 2 parameters (Routing, PUUID)");
                return this->underlying_client->Account.by_puuid(params[0].param, params[1].param);
            case 1:
                if (params.size() < 3) throw std::invalid_argument("Account by_riot_id requires 3 parameters (Routing, Game Name, Tagline)");
                return this->underlying_client->Account.by_riot_id(params[0].param, params[1].param, params[2].param);
            case 2:
                if (params.size() < 3) throw std::invalid_argument("Account by_game_by_puuid requires 3 parameters (Routing, Game, PUUID)");
                return this->underlying_client->Account.by_game_by_puuid(params[0].param, params[1].param, params[2].param);
            default:
                throw std::invalid_argument("Invalid endpoint method index for Riot Account endpoint");
        }
    }

    using raw_json = std::vector<char>;

    void RequestSender::write_response_file(const request& task, const raw_json& result) {
        if (result.size() == 0) return;

        // Strip trailing null characters (\0) which riot-cpp appends to response_content
        size_t valid_len = result.size();
        while (valid_len > 0 && result[valid_len - 1] == '\0') {
            valid_len--;
        }
        if (valid_len == 0) return;

        const int game = task._game;
        const int endpoint = task._endpoint;
        const int endpoint_method = task._endpoint_method;

        std::string identifier = "";
        if (game == 0 && task.params.size() >= 3) {
            identifier = std::string(task.params[1].param) + "_" + std::string(task.params[2].param);
        } else if (task.params.size() >= 2) {
            identifier = task.params[1].param;
        } else if (task.params.size() > 0) {
            identifier = task.params[0].param;
        }

        // 1. Write structured JSON file using FileManager (without trailing null byte)
        std::filesystem::path file_output = FileManager::GetInstance().GetRawOutputPath(game, endpoint, endpoint_method, identifier);
        try {
            if (file_output.has_parent_path()) {
                std::filesystem::create_directories(file_output.parent_path());
            }
            std::ofstream output(file_output, std::ios::trunc);
            if (output.is_open()) {
                output.write(result.data(), valid_len);
                output.close();
            }
        } catch (const std::exception& ex) {
            std::cerr << "[RequestSender] Error writing response file: " << ex.what() << std::endl;
        }

        // 2. Stream response into DuckDB database if initialized
        if (DatabaseManager::GetInstance().IsInitialized()) {
            std::string dev_prefix = FileManager::GetInstance().GetDevKeyPrefix();

            // Riot Account-v1 (Game: 0, Endpoint: 0)
            if (game == 0 && endpoint == 0) {
                try {
                    simdjson::padded_string padded(result.data(), valid_len);
                    simdjson::dom::parser parser;
                    simdjson::dom::element doc;
                    if (parser.parse(padded).get(doc) == simdjson::SUCCESS) {
                        std::string puuid = "", game_name = "", tag_line = "";
                        std::string_view sv;
                        if (doc["puuid"].get(sv) == simdjson::SUCCESS) {
                            puuid = std::string(sv);
                        }
                        if (doc["gameName"].get(sv) == simdjson::SUCCESS) {
                            game_name = std::string(sv);
                        }
                        if (doc["tagLine"].get(sv) == simdjson::SUCCESS) {
                            tag_line = std::string(sv);
                        }

                        if (!game_name.empty() && !tag_line.empty()) {
                            DatabaseManager::GetInstance().SaveAccount(dev_prefix, puuid, game_name, tag_line);
                            std::cout << "[DatabaseManager] Ingested account: " << game_name << "#" << tag_line << std::endl;
                        }
                    }
                } catch (const std::exception& ex) {
                    std::cerr << "[RequestSender] Account JSON parse error: " << ex.what() << std::endl;
                }
            }
            // League Match-v5 (Game: 1, Endpoint: 6, Method: 0 [Match] or 1 [Timeline])
            else if (game == 1 && endpoint == 6) {
                if (task.params.size() >= 2) {
                    std::string match_id = task.params[1].param;
                    std::string raw_str(result.data(), valid_len);
                    if (endpoint_method == 0) {
                        DatabaseManager::GetInstance().SaveMatch(match_id, raw_str);
                        std::cout << "[DatabaseManager] Ingested match: " << match_id << std::endl;
                    } else if (endpoint_method == 1) {
                        DatabaseManager::GetInstance().SaveTimeline(match_id, raw_str);
                        std::cout << "[DatabaseManager] Ingested timeline: " << match_id << std::endl;
                    }
                }
            }
        }
    }

    void RequestSender::Send_Batch_Request(std::shared_ptr<BatchRequest> batch) {
        if (!batch) return;

        std::cout << "[RequestSender] Send_Batch_Request: Starting pipeline execution..." << std::endl;
        try {
            request next = batch->get_next();
            while (!next.same_endpoint(BatchRequest::FINISHED)) {
                std::cout << "[RequestSender] Send_Batch_Request: Dispatching query game=" << next._game << " endpoint=" << next._endpoint << " method=" << next._endpoint_method << std::endl;
                json_ptr result = this->Send_Request(next);
                while (batch->insert_result(std::move(result))) {
                    next = batch->get_current();
                    if (next.same_endpoint(BatchRequest::FINISHED)) break;
                    std::cout << "[RequestSender] Send_Batch_Request: Dispatching iterative query game=" << next._game << " endpoint=" << next._endpoint << " method=" << next._endpoint_method << std::endl;
                    result = this->Send_Request(next);
                }
                next = batch->get_next();
            }
            std::cout << "[RequestSender] Send_Batch_Request: Pipeline completed successfully." << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "[RequestSender] Exception in Send_Batch_Request: " << ex.what() << std::endl;
        }
    }



    void RequestSender::test_regions() {
        /*
         * Simply insert status requests for each region into request queue then returns
         * LOL STATUS request has ID 1, 5, 0
         */

        static auto add_req = [this](const std::size_t ind) -> void 
        {
            this->add_request({1, 5, 0, {this->regions[ind]}, {}, {}, &this->region_avail[ind]});
        };

        re_utils::map_func(add_req, std::make_index_sequence<16>({}));
    }

    static int acceptable_server_avail(const std::array<int, 16>& availabilities) {

        int waiting_count = 0;
        int success_count = 0;
        int failure_count = 0;

        for (const int& status : availabilities) {
            if      (status == 0) { waiting_count++; }
            else if (status == 1) { success_count++; }
            else                  { failure_count++; }
        }
        // bit branchless programming for your enjoyment (preceded by branchful programming :) )
        return 0 + ((success_count > NUM_REGIONS/2) * 1) + ((failure_count > NUM_REGIONS/2) * 2);
    }

    static bool make_recomendation(const int rec, const float x_begin, const float x_end) {

        static ImGuiIO& io = ImGui::GetIO();

        static constexpr char waiting_msg[] = "Testing in progress";
        static constexpr char success_msg[] = "Key successfully validated";
        static constexpr char failure_msg[] = "Key validation failed";

        static constexpr char waiting_button[] = "Skip";
        static constexpr char success_button[] = "Continue";
        static constexpr char failure_button[] = "Continue (caution)";

        static const float waiting_width = ImGui::CalcTextSize(waiting_msg).x;
        static const float success_width = ImGui::CalcTextSize(success_msg).x;
        static const float failure_width = ImGui::CalcTextSize(failure_msg).x;
        
        const char* to_use = (char*)((long)&waiting_msg[0] * (rec == 0) + (long)&success_msg[0] * (rec == 1) + (long)&failure_msg[0] * (rec == 2));
        const char* button_msg = (char*)((long)&waiting_button[0] * (rec == 0) + (long)&success_button[0] * (rec == 1) + (long)&failure_button[0] * (rec == 2));

        ImGui::SetCursorPosX(io.DisplaySize.x * 0.5  - ImGui::CalcTextSize(to_use).x * 0.5);
        const ImVec4 col = (rec == 0) ? ImVec4(0.58f, 0.58f, 0.58f, 1.0f) : (rec == 1) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(to_use);
        ImGui::PopStyleColor();

        ImGui::NewLine();
        ImGui::SetCursorPosX(x_end - ImGui::CalcTextSize(button_msg).x - 2* ImGui::GetStyle().ButtonTextAlign.x);
        if (ImGui::Button(button_msg)) {
            return true;
        };
        
        return false;
    }

    int RequestSender::region_test_display() {
        
        static ImGuiIO& io = ImGui::GetIO();

        static bool first = true;
        static float title_size;
        if (first) {
            ImGui::PushFont(io.Fonts->Fonts[1]);
            title_size = ImGui::CalcTextSize("RESTfulEzreal;").x;
            ImGui::PopFont();
            first = false;
        }

        const float x_begin = io.DisplaySize.x * 0.5 - title_size * 0.5;
        const float x_end   = x_begin + title_size;

        static auto disp_singular_text = [this](const std::size_t ind, const float x_begin, const float x_end) -> void 
        {
            ImGui::SetCursorPosX(x_begin);
            ImGui::TextUnformatted("Testing ");
            ImGui::SameLine();
            ImGui::TextUnformatted(this->regions[ind]);
            ImGui::SameLine();
            re_utils::pending_text_right("waiting", "Success", "failure", this->region_avail[ind], x_end);
        };

        re_utils::map_func_mult(disp_singular_text, std::make_index_sequence<16>({}), x_begin, x_end);

        char continue_button[9] = "continue";
        const int recommendation = acceptable_server_avail(this->region_avail);
        ImGui::NewLine();

        int continue_indicator = 0;
        // only return at very end to prevent flicker
        if (make_recomendation(recommendation, x_begin, x_end)) {
            continue_indicator = 1;
        };
        ImGui::SameLine();
        ImGui::SetCursorPosX(x_begin);
        if (ImGui::Button("Cancel")) {
            continue_indicator = 2;
        };

        return continue_indicator;
    }
}
