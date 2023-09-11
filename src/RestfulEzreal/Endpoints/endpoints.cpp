#include <vector>
#include <array>
#include <memory>
#include <stdlib.h>
#include "../RestfulEzreal.h"
#include "imgui.h"

namespace restfulEz {

    std::shared_ptr<LinkedInterface> LinkedInterface::make_linked(const int gameName, const int end_name, const int endpoint_method) {
        switch (gameName) {
            case 0:
                switch (endpoint_method) {
                    case 0:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(gameName, end_name, endpoint_method)); break;
                    case 1:
                        return std::make_shared<LinkedForm<3>>(BaseForm::make_form(gameName, end_name, endpoint_method)); break;
                    case 2:
                        return std::make_shared<LinkedForm<3>>(BaseForm::make_form(gameName, end_name, endpoint_method)); break;
                } break;
            case 1: // LEAGUE OF LEGENDS
                return make_linked_LOL(gameName, end_name, endpoint_method); break;
            case 2: // TEAMFIGHT TACTICS
                return make_linked_TFT(gameName, end_name, endpoint_method); break;
            case 3: // VALORANT
                return make_linked_VAL(gameName, end_name, endpoint_method); break;
            case 4: // LEGENDS OF RUNETERR
                return make_linked_LOR(gameName, end_name, endpoint_method); break;
            default:
                throw std::invalid_argument("Invalid Game Name");
        }
        throw std::invalid_argument("Invalid Game Name");
    }

    BaseForm BaseForm::make_form(const int gameName, const int end_name, const int endpoint_method) {
        switch (gameName) {
            case 0:
                switch (endpoint_method) {
                    case 0:
                        return BaseForm(2, 0, 0, 0, "Riot", "Account", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case 1:
                        return BaseForm(3, 0, 0, 1, "Riot", "Account", "By Riot ID", {"Routing", "Game Name", "Tagline"}, {_NO_FLAG,_NO_FLAG,_NO_FLAG}); break;
                    case 2:
                        return BaseForm(3, 0, 0, 2, "Riot", "Account", "By Game By Puuid", {"Routing", "Game", "PUUID"}, {_NO_FLAG,_NO_FLAG,_NO_FLAG}); break;
                } break;
            case 1: // LEAGUE OF LEGENDS
                return make_form_LOL(end_name, endpoint_method); break;
            case 2: // TEAMFIGHT TACTICS
                return make_form_TFT(end_name, endpoint_method); break;
            case 3: // VALORANT
                return make_form_VAL(end_name, endpoint_method); break;
            case 4: // LEGENDS OF RUNETERR
                return make_form_LOR(end_name, endpoint_method); break;
            default:
                throw std::invalid_argument("Invalid Game Name");
        }
        throw std::invalid_argument("Invalid Game Name");
    }
    void RestfulEzreal::NewQueryForm(int gameName, int end_name, int endpoint_method) {
        this->pushNewForm(BaseForm::make_form(gameName, end_name, endpoint_method));
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
            }
    }

    static const inline std::string create_filename(const request& name_request) {
        std::string request_name = "placeholder for now";
        return request_name;
    }

};
