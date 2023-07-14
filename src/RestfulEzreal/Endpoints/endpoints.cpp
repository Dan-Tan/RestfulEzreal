#include "Walnut/Application.h"
#include <vector>
#include <array>
#include <memory>
#include <stdlib.h>
#include "../RestfulEzreal.h"
#include "imgui.h"

namespace restfulEz {

    static std::array<QUERY_FORM, 3> Account = {
        QUERY_FORM(2, 0, 0, 0, "Riot", "Account", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(3, 0, 0, 1, "Riot", "Account", "By Riot ID", {"Routing", "Game Name", "Tagline"}, {_NO_FLAG,_NO_FLAG,_NO_FLAG}),
        QUERY_FORM(3, 0, 0, 2, "Riot", "Account", "By Game By Puuid", {"Routing", "Game", "PUUID"}, {_NO_FLAG,_NO_FLAG,_NO_FLAG})
    };
    //
    void RestfulEzreal::NewQueryForm(int gameName, int end_name, int endpoint_method) {
        switch (gameName) {
            case 0: // RIOT
                this->pushNewForm(Account[endpoint_method]); break;
            case 1: // LEAGUE OF LEGENDS
                this->NewQueryFormLOL(end_name, endpoint_method); break;
            case 2: // TEAMFIGHT TACTICS
                this->NewQueryFormTFT(end_name, endpoint_method); break;
            case 3: // VALORANT
                this->NewQueryFormLOR(end_name, endpoint_method); break;
            case 4: // LEGENDS OF RUNETERR
                this->NewQueryFormVAL(end_name, endpoint_method); break;
            default:
                throw std::invalid_argument("Invalid Game Name");
        }

    }

    // Render Endpoint Submission Forms
    void QUERY_FORM::render_form() {
        float height_l = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild(this->_ID.data(), ImVec2(ImGui::GetContentRegionAvail().x, (height_l * (this->_n_params + this->_n_used_optional_p1 + 2) + 3 * ImGui::GetStyle().ItemSpacing.y)), true, ImGuiWindowFlags_ChildWindow);
        ImGui::Text((this->_game_name + " | " + this->_endpoint + " | " + this->_endpoint_method).data());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("My Button").x);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        if (ImGui::Button("Close")) {
            this->remove_form = true;
        };

        if (!this->form_execute) {
            for (int i = 0; i < this->_n_params; i++) {
                ImGui::InputText(this->_param_names[i], this->_params_in_form[i], 256, this->_type_ordering[i]);
            }
            if (this->_accepts_optional) {
                this->render_optionals(false);
            }
        }
        else {
            ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
            for (int i = 0; i < this->_n_params; i++) {
                ImGui::InputText(this->_param_names[i], this->_params_in_form[i], 256, ImGuiInputTextFlags_ReadOnly);
            }
            if (this->_accepts_optional) {
                this->render_optionals(true);
            }
            ImGui::PopStyleColor();
        }

        if (!this->form_execute) {
            if (ImGui::Button("Send")) {
                bool empty = false;
                for (int i = 0; i < this->_n_params; i++) {
                    empty |= strlen(this->_params_in_form[i].param) == 0;
                }
                if (!empty) {
                    for (int i = 0; i < this->_n_params; i++) {
                        this->_req_in_form.params.push_back(this->_params_in_form[i]);
                    }
                    *this->_execute = true;
                    this->form_execute = true;
                    *this->_client_req = this;
                }
                if (this->_accepts_optional) {
                    this->render_optionals(false);
                }
            }
        }
        else {
            if (this->request_result.size() != 0) {
                ImGui::Text(this->request_result.data());
            }
        }
        ImGui::EndChild();
    };	

    void QUERY_FORM::render_optionals(bool already_sent) {
        if (!this->_accepts_optional) {
            throw std::logic_error("QUERY_FORM::render_optional should never be called for endpoints with no optional arguements");
        }

        char id[4] = "x# ";

        if (already_sent) {
            ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
            for (int i = 0; i < this->_optionals_to_send.size(); i++) {
                if (this->_optionals_to_send[i] == 1) {
                    ImGui::InputText(this->_optional_names[i], this->_optional_inputs[i], 256, ImGuiInputTextFlags_ReadOnly);
                }
            }
            ImGui::PopStyleColor();
        } else {
            for (int i = 0; i < this->_optionals_to_send.size(); i++) {
                if (this->_optionals_to_send[i]) {
                    ImGui::InputText(this->_optional_names[i], this->_optional_inputs[i], 256, this->_optional_types[i]);
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("x").x);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
                    if (ImGui::Button(id)) {
                        this->_optionals_to_send[i] = 0;
                        this->_n_used_optional_p1 -= 1;
                    };
                }
            }
        }
        float drop_down_width = (0.3f * ImGui::GetContentRegionAvail().x);
        ImGui::SetNextItemWidth(drop_down_width);
        static int opt_index = 0;
        if (ImGui::BeginCombo("##OPTIONAL", this->_optional_names[opt_index], ImGuiComboFlags_None))
        {
            for (int n = 0; n < this->_optional_names.size(); n++)
            {
                if (this->_optionals_to_send[n] == 0) {
                    const bool is_selected = (opt_index == n);
                    if (ImGui::Selectable(this->_optional_names[n], is_selected)) {
                        opt_index = n;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }

        // Render add optional button besides dropdown
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("Add Optional").x);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
        if (ImGui::Button("Add Optional")) {
            if (this->_optionals_to_send[opt_index] == 0) {this->_n_used_optional_p1 += 1;};
            this->_optionals_to_send[opt_index] = 1;
        }

    }

    void RestfulEzreal::NewFormButton() {

            static const char* GAMES_[] = { "Riot", "League of Legends", "TeamfightTactics", "Legends of Runeterra", "Valorant"};

            static const char GAME_ENDPOINTS_[5][9][18] = {
            { "Account" },
            { "Champion Mastery", "Champion Rotation", "Clash", "League", "Challenges", "Status", "Match", "Summoner", "Spectator" },
            { "League", "Match", "Status", "Summoner" },
            { "Match", "Ranked", "Status" },
            { "Content", "Match", "Ranked", "Status" }
            };


            static const char ENDPOINT_METHODS_[5][9][7][24] = {
            {{"By Puuid", "By Riot ID", "By Game"}},
            {{ "By Summoner ID", "By Summoner By Champion", "Top By Summoner", "Scores By Summoner" },
            { "Rotation" },
            {"By Summoner ID", "By Team", "Tourament By Team", "By Tournament"},
            {"Challenger", "Grandmaster", "Master", "By Summoner ID", "By League ID", "Specific League", "Experimental"},
            {"Configuration", "Percentiles", "Challenge Configuration", "Challenge Leaderboard", "Challenge Percentiles", "By Puuid"},
            {"v4 (recommended)", "v3"},
            {"By Match ID", "Timeline", "By Puuid"},
            {"By RSO Puuid", "By Account ID", "By Name", "By Puuid", "By Summoner ID"},
            {"By Summoner ID", "Featured Games"}
            },
            {{ "Challenger", "Grandmaster", "Master", "By Summoner ID", "By League ID", "Queue Top", "By Tier Division"},
                { "By Puuid", "By Match ID"},
                {"v1"},
                {"By Account", "By Name", "By Puuid", "By Summoner ID"}
            },
            {{ "By Puuid", "By Match ID"},
                { "Leaderboards"},
                {"v1"}
            },
            {{ "Content"},
                { "By Match ID", "By Puuid", "By  Queue"},
                {"By Act"},
                {"Status"}
            }
            };

            static const int Game_endpoint_length[5] = {1, 9, 4, 3, 4};

            static const int endpoint_lengths[5][9] = {
                {3},
                {4, 1, 4, 7, 6, 2, 3, 5, 2},
                {7, 2, 1, 3},
                {2, 1, 1},
                {1, 3, 1, 1}
            };

            static int last_iter[] = {0, 0, 0};

            static int game = 0;
            static int endpoint = 0;
            static int endpoint_method = 0;

            if (game != last_iter[0]) {
                last_iter[0] = game;
                endpoint = 0;
                endpoint_method = 0;
                last_iter[1] = 0;
                last_iter[2] = 0;
            }
            else if (last_iter[1] != endpoint) {
                last_iter[1] = endpoint;
                endpoint_method = 0;
                last_iter[2] = 0;
            }

            float drop_down_width = (0.3f * ImGui::GetContentRegionAvail().x);
            ImGui::SetNextItemWidth(drop_down_width);
            if (ImGui::BeginCombo("##input1", GAMES_[game], ImGuiComboFlags_None))
            {
                for (int n = 0; n < IM_ARRAYSIZE(GAMES_); n++)
                {
                    const bool is_selected = (game == n);
                    if (ImGui::Selectable(GAMES_[n], is_selected))
                        game = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(drop_down_width);
            if (ImGui::BeginCombo("##input2", GAME_ENDPOINTS_[game][endpoint]))
            {
                for (int n = 0; n < Game_endpoint_length[game]; n++)
                {
                    const bool is_selected = (endpoint == n);
                    if (ImGui::Selectable(GAME_ENDPOINTS_[game][n], is_selected))
                        endpoint = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(drop_down_width);
            if (ImGui::BeginCombo("##input3", ENDPOINT_METHODS_[game][endpoint][endpoint_method]))
            {
                for (int n = 0; n < endpoint_lengths[game][endpoint]; n++)
                {
                    const bool is_selected = (endpoint_method == n);
                    if (ImGui::Selectable(ENDPOINT_METHODS_[game][endpoint][n], is_selected))
                        endpoint_method = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            if (ImGui::Button("Add")) {
                this->NewQueryForm(game, endpoint, endpoint_method);
                game = 0; endpoint = 0; endpoint_method = 0;
                last_iter[0] = 0; last_iter[1] = 0; last_iter[2] = 0;
            }
    }

    void RestfulEzreal::Send_Request() {
        std::vector<PARAM_CONT> & params = this->_next_request->_req_in_form.params;
        if (!this->send_next_request) {
            return;
        }
        this->send_next_request = false;
        Json::Value response;
        switch (this->_next_request->_req_in_form._game) {
            case 0: // RIOT
                switch (this->_next_request->_req_in_form._endpoint_method) {
                    case 0:
                        response = this->_underlying_client->Account.by_puuid(params.at(0), params.at(1)); break;
                    case 1:
                        response = this->_underlying_client->Account.by_riot_id(params.at(0), params.at(1), params.at(2)); break;
                    case 2:
                        response = this->_underlying_client->Account.by_game(params.at(0), params.at(1), params.at(2)); break;
                }break;
            case 1: // LEAGUE OF LEGENDS
                response = this->Send_LOL(); break;
            case 2: // TEAMFIGHT TACTICS
                response = this->Send_TFT(); break;
            case 3: // VALORANT
                response = this->Send_VAL(); break;
            case 4: // LEGENDS OF RUNETERR
                response = this->Send_LOR(); break;
            default:
                throw std::invalid_argument("Invalid Game Index");
        }
        const int game = this->_next_request ->_req_in_form._game;
        const int endpoint = this->_next_request->_req_in_form._endpoint;
        const int endpoint_method = this->_next_request->_req_in_form._endpoint_method;

        std::ofstream output;
        std::string file_output = this->_path_to_output + std::string("req") + std::to_string(game)+ std::to_string(endpoint)+ std::to_string(endpoint_method);
        file_output.append(".json");
        output.open(file_output);
        Json::StreamWriterBuilder builder;
        output << Json::writeString(builder, response);
        output.close();

        if (response.isMember("status")) {
            this->_next_request->request_result = response["status"]["message"].asString();
        }
        else {
            this->_next_request->request_result = "Successful";
        }
    }
};
