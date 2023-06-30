#include "Walnut/Application.h"
#include <vector>
#include <array>
#include <memory>
#include <stdlib.h>
#include "../include/RestfulEzreal.h"

#define _NO_FLAG ImGuiInputTextFlags_None
#define _DEC_FLAG ImGuiInputTextFlags_CharsDecimal

namespace restfulEz {
	
	static std::array<QUERY_FORM, 3> Account = {
		QUERY_FORM(2, 0, 0, 0, "Riot", "Account", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(3, 0, 0, 1, "Riot", "Account", "By Riot ID", {"Routing", "Game Name", "Tagline"}, {_NO_FLAG,_NO_FLAG,_NO_FLAG}),
		QUERY_FORM(3, 0, 0, 2, "Riot", "Account", "By Game By Puuid", {"Routing", "Game", "PUUID"}, {_NO_FLAG,_NO_FLAG,_NO_FLAG})
	};
	static std::array<QUERY_FORM, 4> Champion_Mastery = {
		QUERY_FORM(2, 1, 0, 0, "League of Legends", "Champion Mastery", "By Summoner Id", {"Routing","Summoner ID"},{_NO_FLAG,_NO_FLAG}),
		QUERY_FORM(3, 1, 0, 1, "League of Legends", "Champion Mastery", "By Summoner By Champion", {"Routing", "Summoner ID", "Champion ID"},{_NO_FLAG,_NO_FLAG,_DEC_FLAG}),
		QUERY_FORM(3, 1, 0, 2, "League of Legends", "Champion Mastery", "Top By Summoner", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 0, 3, "League of Legends", "Champion Mastery", "Score By Summoner", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 1> Champion_Rotation = {
		QUERY_FORM(1, 1, 1, 0, "League of Legends", "Champion_Rotation", "Rotation", {"Routing"}, {_NO_FLAG})
	};
	static std::array<QUERY_FORM, 4> Clash = {
		QUERY_FORM(2, 1, 2, 0, "League of Legends", "Clash", "By Summoner Id", {"Routing", "Summoner Id"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 2, 1, "League of Legends", "Clash", "By Team", {"Routing", "Team"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 2, 2, "League of Legends", "Clash", "Tournament By Team", {"Routing", "Team"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 2, 3, "League of Legends", "Clash", "By Tourament", {"Routing", "Tourament"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 7> League = {
		QUERY_FORM(2, 1, 3, 0, "League of Legends", "League", "Challenger", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 3, 1, "League of Legends", "League", "Grandmaster", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 3, 2, "League of Legends", "League", "Master", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 3, 3, "League of Legends", "League", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 3, 4, "League of Legends", "League", "By League ID", {"Routing", "League ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(4, 1, 3, 5, "League of Legends", "League", "Specific League", {"Routing", "Queue", "Tier", "Division"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG, _NO_FLAG}),
		QUERY_FORM(4, 1, 3, 6, "League of Legends", "League", "Experimental", {"Routing", "Queue", "Tier", "Division"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 6> Challenges = {
		QUERY_FORM(1, 1, 4, 0, "League of Legends", "Challenges", "Configuration", {"Routing"}, {_NO_FLAG}),
		QUERY_FORM(1, 1, 4, 1, "League of Legends", "Challenges", "Percentiles", {"Routing"}, {_NO_FLAG}),
		QUERY_FORM(2, 1, 4, 2, "League of Legends", "Challenges", "Challenge Configuration", {"Routing", "Challenge ID"}, {_NO_FLAG, _DEC_FLAG}),
		QUERY_FORM(3, 1, 4, 3, "League of Legends", "Challenges", "Challenge Leaderboards", {"Routing", "Challenge ID", "Level"}, {_NO_FLAG, _DEC_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 4, 4, "League of Legends", "Challenges", "Challenge Percentiles", {"Routing", "Challenge ID"}, {_NO_FLAG, _DEC_FLAG}),
		QUERY_FORM(2, 1, 4, 5, "League of Legends", "Challenges", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 2> Lol_Status = {
		QUERY_FORM(1, 1, 5, 0, "League of Legends", "Status", "Default (v4)", {"Routing"}, {_NO_FLAG}),
		QUERY_FORM(1, 1, 5, 1, "League of Legends", "Status", "v3", {"Routing"}, {_NO_FLAG})
	};
	static std::array<QUERY_FORM, 2> Lor_Match = {
		QUERY_FORM(2, 3, 0, 0, "Legends of Runeterra", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 3, 0, 1, "Legends of Runeterra", "Match", "By Match", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 1> Lor_Ranked = {
		QUERY_FORM(1, 3, 1, 0, "Legends of Runeterra", "Ranked", "Leaderboards", {"Routing"}, {_NO_FLAG})
	};
	static std::array<QUERY_FORM, 1> Lor_Status = {
		QUERY_FORM(1, 3, 2, 0, "Legends of Runeterra", "Status", "Status", {"Routing"}, {_NO_FLAG})
	};
	static std::array<QUERY_FORM, 3> Match = {
		QUERY_FORM(2, 1, 6, 0, "League of Legends", "Match", "By Match ID", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG }),
		QUERY_FORM(2, 1, 6, 1, "League of Legends", "Match", "Timeline", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 6, 2, "League of Legends", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 5> Summoner = {
		QUERY_FORM(2, 1, 7, 0, "League of Legends", "Summoner", "By RSO Puuid", {"Routing", "RSO PUUID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 7, 1, "League of Legends", "Summoner", "By Account ID", {"Routing", "Account ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 7, 2, "League of Legends", "Summoner", "By Name", {"Routing", "Name"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 7, 3, "League of Legends", "Summoner", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 1, 7, 4, "League of Legends", "Summoner", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG })
	};
	static std::array<QUERY_FORM, 2> Spectator = {
		QUERY_FORM(2, 1, 8, 0, "League of Legends", "Spectator", "By Summoner Id", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG }),
		QUERY_FORM(1, 1, 8, 1, "League of Legends", "Spectator", "Featured Games", {"Routing"}, {_NO_FLAG})
	};
	static std::array<QUERY_FORM, 7> Tft_League = {
		QUERY_FORM(1, 2, 0, 0, "Teamfight Tactics", "League", "Challenger", {"Routing"}, {_NO_FLAG}),
		QUERY_FORM(1, 2, 0, 1, "Teamfight Tactics", "League", "Grandmaster", {"Routing"}, {_NO_FLAG}),
		QUERY_FORM(1, 2, 0, 2, "Teamfight Tactics", "League", "Master", {"Routing"}, {_NO_FLAG}),
		QUERY_FORM(2, 2, 0, 3, "Teamfight Tactics", "League", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 2, 0, 4, "Teamfight Tactics", "League", "By League ID", {"Routing", "League ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 2, 0, 5, "Teamfight Tactics", "League", "Queue Top", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(3, 2, 0, 6, "Teamfight Tactics", "League", "By Tier Division", {"Routing", "Tier", "Divisionk"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 2> Tft_Match = {
		QUERY_FORM(2, 2, 1, 0, "Teamfight Tactics", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 2, 1, 1, "Teamfight Tactics", "Match", "By Match ID", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 1> Tft_Status = {
		QUERY_FORM(1, 2, 2, 0, "Teamfight Tactics", "Status", "Status", {"Routing"}, {_NO_FLAG})
	};
	static std::array<QUERY_FORM, 4> Tft_Summoner = {
		QUERY_FORM(2, 2, 3, 0, "Teamfight Tactics", "Summoner", "By Account ID", {"Routing", "Account ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 2, 3, 1, "Teamfight Tactics", "Summoner", "By Name", {"Routing", "Name"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 2, 3, 2, "Teamfight Tactics", "Summoner", "By PUUID", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 2, 3, 3, "Teamfight Tactics", "Summoner", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 1> Val_Content = {
		QUERY_FORM(1, 4, 0, 1, "Valorant", "Content", "Content", {"Routing"}, {_NO_FLAG})
	};
	static std::array<QUERY_FORM, 3> Val_Match = {
		QUERY_FORM(2, 4, 1, 0, "Valorant", "Match", "By Match", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 4, 1, 1, "Valorant", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
		QUERY_FORM(2, 4, 1, 2, "Valorant", "Match", "By Queue", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 1> Val_Ranked = {
		QUERY_FORM(2, 4, 2, 0, "Valorant", "Ranked", "By Act", {"Routing", "Act ID"}, {_NO_FLAG, _NO_FLAG})
	};
	static std::array<QUERY_FORM, 1> Val_Status = {
		QUERY_FORM(1, 4, 3, 0, "Valorant", "Status", "Status", {"Routing"}, {_NO_FLAG})
	};
//
	void RestfulEzreal::NewQueryForm(int gameName, int end_name, int endpoint_method) {
		switch (gameName) {
		case 0: // RIOT
			this->pushNewForm(Account[endpoint_method]); break;
		case 1: // LEAGUE OF LEGENDS
			switch (end_name) {
			case 0:
				this->pushNewForm(Champion_Mastery[endpoint_method]); break;
			case 1:
				this->pushNewForm(Champion_Rotation[endpoint_method]); break;
			case 2:
				this->pushNewForm(Clash[endpoint_method]); break;
			case 3:
				this->pushNewForm(League[endpoint_method]); break;
			case 4:
				this->pushNewForm(Challenges[endpoint_method]); break;
			case 5:
				this->pushNewForm(Lol_Status[endpoint_method]); break;
			case 6:
				this->pushNewForm(Match[endpoint_method]); break;
			case 7:
				this->pushNewForm(Summoner[endpoint_method]); break;
			case 8:
				this->pushNewForm(Spectator[endpoint_method]); break;
			}break;
		case 2: // TEAMFIGHT TACTICS
			switch (end_name) {
			case 0: // LEAGUE
				this->pushNewForm(Tft_League[endpoint_method]);break;
			case 1: // MATCH
				this->pushNewForm(Tft_Match[endpoint_method]);break;
			case 2: // STATUS
				this->pushNewForm(Tft_Status[endpoint_method]);break;
			case 3: // SUMMONER
				this->pushNewForm(Tft_Summoner[endpoint_method]);break;
			}
		case 3: // VALORANT
			switch (end_name) {
			case 0: // CONTENT
				this->pushNewForm(Val_Content[endpoint_method]); break;
			case 1: // MATCH
				this->pushNewForm(Val_Match[endpoint_method]); break;
			case 2: // STATUS
				this->pushNewForm(Val_Ranked[endpoint_method]); break;
			case 3: // RANKED
				this->pushNewForm(Val_Status[endpoint_method]); break;
			} break;
		case 4: // LEGENDS OF RUNETERR
			switch (end_name) {
			case 0: // MATCH
				this->pushNewForm(Lor_Match[endpoint_method]); break;
			case 1: // RANKED
				this->pushNewForm(Lor_Ranked[endpoint_method]); break;
			case 2: // STATUS
				this->pushNewForm(Lor_Status[endpoint_method]); break;
			}break;
		default:
			throw std::invalid_argument("Invalid Game Name");
		}
			
	}

	// Render Endpoint Submission Forms
	void QUERY_FORM::render_form() {
		float height_l = ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild(this->_ID.data(), ImVec2(ImGui::GetContentRegionAvail().x, (height_l * (this->_n_params + 2) + 3 * ImGui::GetStyle().ItemSpacing.y)), true, ImGuiWindowFlags_ChildWindow);
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
		}
		else {
			ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
			for (int i = 0; i < this->_n_params; i++) {
				ImGui::InputText(this->_param_names[i], this->_params_in_form[i], 256, ImGuiInputTextFlags_ReadOnly);
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
			}
		}
		else {
			if (this->request_result.size() != 0) {
				ImGui::Text(this->request_result.data());
			}
		}
		ImGui::EndChild();
	};	

	Json::Value RestfulEzreal::Send_LOL() {
		Json::Value response;

		std::vector<PARAM_CONT>& params = this->_next_request->_req_in_form.params;
		const int endpoint = this->_next_request->_req_in_form._endpoint;
		const int endpoint_method = this->_next_request->_req_in_form._endpoint_method;

		switch (endpoint) {
		case 0: // Champion Mastery
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Champion_Mastery.by_summoner_id(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->Champion_Mastery.by_summoner_by_champion(params.at(0), params.at(1), atoi(params.at(2))); break;
			case 2:
				response = this->_underlying_client->Champion_Mastery.by_summoner_id(params.at(0), params.at(1)); break;
			case 3:
				response = this->_underlying_client->Champion_Mastery.by_summoner_id(params.at(0), params.at(1)); break;
			} break;
		case 1: // Champion Rotation
			response = this->_underlying_client->Champion.rotations(params.at(0));break;
		case 2: // Clash
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Clash.by_summoner_id(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->Clash.by_team(params.at(0), params.at(1)); break;
			case 2:
				response = this->_underlying_client->Clash.tournament_by_team(params.at(0), params.at(1)); break;
			case 3:
				response = this->_underlying_client->Clash.by_tournament(params.at(0), params.at(1)); break;
			}break;
		case 3: // League
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->League.challenger(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->League.grandmaster(params.at(0), params.at(1)); break;
			case 2:
				response = this->_underlying_client->League.master(params.at(0), params.at(1)); break;
			case 3:
				response = this->_underlying_client->League.by_summoner_id(params.at(0), params.at(1)); break;
			case 4:
				response = this->_underlying_client->League.by_league_id(params.at(0), params.at(1)); break;
			case 5:
				response = this->_underlying_client->League.specific_league(params.at(0), params.at(1), params.at(2), params.at(3));;; break;
			case 6:
				response = this->_underlying_client->League_Exp.entries(params.at(0), params.at(1), params.at(2), params.at(3)); break;
			}break;
		case 4: // Challenges
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Lol_Challenges.config(params.at(0)); break;
			case 1:
				response = this->_underlying_client->Lol_Challenges.percentiles(params.at(0)); break;
			case 2:
				response = this->_underlying_client->Lol_Challenges.challenge_config(params.at(0), atoi(params.at(1))); break;
			case 3:
				response = this->_underlying_client->Lol_Challenges.challenge_leaderboard(params.at(0), atoi(params.at(2)), params.at(2)); break;
			case 4:
				response = this->_underlying_client->Lol_Challenges.challenge_percentiles(params.at(0), atoi(params.at(1))); break;
			case 5:
				response = this->_underlying_client->Lol_Challenges.by_puuid(params.at(0), params.at(1)); break;
			}break;
		case 5: // Lol_Status
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Lol_Status.v4(params.at(0)); break;
			case 1:
				response = this->_underlying_client->Lol_Status.v3(params.at(0)); break;
			}break;
		case 6: // Match
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Match.by_match_id(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->Match.timeline(params.at(0), params.at(1)); break;
			case 2:
				response = this->_underlying_client->Match.by_puuid(params.at(0), params.at(1), { "", "" }); break;
			}break;
		case 7: // Summoner
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Summoner.by_rso_puuid(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->Summoner.by_account_id(params.at(0), params.at(1)); break;
			case 2:
				response = this->_underlying_client->Summoner.by_name(params.at(0), params.at(1)); break;
			case 3:
				response = this->_underlying_client->Summoner.by_puuid(params.at(0), params.at(1)); break;
			case 4:
				response = this->_underlying_client->Summoner.by_summoner_id(params.at(0), params.at(1)); break;
			}break;
		case 8: // Spectator
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Spectator.by_summoner_id(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->Spectator.featured_games(params.at(0)); break;
			}break;
		default: // Catch Exception
			throw std::invalid_argument("Invalid Endpoint");
		};
		return response;
	}

	Json::Value RestfulEzreal::Send_TFT() {
		Json::Value response;

		std::vector<PARAM_CONT>& params = this->_next_request->_req_in_form.params;
		const int endpoint = this->_next_request->_req_in_form._endpoint;
		const int endpoint_method = this->_next_request->_req_in_form._endpoint_method;

		switch (endpoint) {
		case 0:
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Tft_League.challenger(params.at(0)); break;
			case 1:
				response = this->_underlying_client->Tft_League.grandmaster(params.at(0)); break;
			case 2:
				response = this->_underlying_client->Tft_League.master(params.at(0)); break;
			case 3:
				response = this->_underlying_client->Tft_League.by_summoner_id(params.at(0), params.at(1)); break;
			case 4:
				response = this->_underlying_client->Tft_League.by_league_id(params.at(0), params.at(1)); break;
			case 5:
				response = this->_underlying_client->Tft_League.queue_top(params.at(0), params.at(1)); break;
			case 6:
				response = this->_underlying_client->Tft_League.by_tier_division(params.at(0), params.at(1), params.at(2)); break;
			}
			break;
		case 1:
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Tft_Match.by_puuid(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->Tft_Match.by_match(params.at(0), params.at(1)); break;
			}
			break;
		case 2:
			response = this->_underlying_client->Tft_Status.v1(params.at(0)); break;
			break;
		case 3:
			switch (endpoint_method) {
			case 0:
				response = this->_underlying_client->Tft_Summoner.by_account(params.at(0), params.at(1)); break;
			case 1:
				response = this->_underlying_client->Tft_Summoner.by_account(params.at(0), params.at(1)); break;
			case 2:
				response = this->_underlying_client->Tft_Summoner.by_puuid(params.at(0), params.at(1)); break;
			case 3:
				response = this->_underlying_client->Tft_Summoner.by_summoner_id(params.at(0), params.at(1)); break;
			}
			break;
		}

		return response;
	}

	Json::Value RestfulEzreal::Send_VAL() {
		Json::Value response;
		return response;
	}

	Json::Value RestfulEzreal::Send_LOR() {
		Json::Value response;
		return response;
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

		static const int Game_endpoint_length[5] = {1, 9, 4, 4, 3};

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
