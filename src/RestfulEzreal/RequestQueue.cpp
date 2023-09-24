#include "RequestQueue.h"
#include "BatchRequests.h"
#include "imgui.h"
#include "utils/utils.h"
#include <stdexcept>
#include <filesystem>

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
            if (iterative) {
                this->Send_Batch_Request(b_task);
            } else if (singular) {
                this->Send_Request(task);
            } else {
                generic_work();
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
        try {
            bool failure = response[2] == 's'
                        && response[3] == 't'
                        && response[4] == 'a'
                        && response[5] == 't'
                        && response[6] == 'u'
                        && response[7] == 's';
            return failure;
        }
        catch (std::out_of_range& ex) {
            return true; // request failed
        }

    }

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
        bool failure = response->size() == 0 || check_failure_response(*response);
        if (task.success_ptr) {
            *task.success_ptr = failure ? 2 : 1;
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
