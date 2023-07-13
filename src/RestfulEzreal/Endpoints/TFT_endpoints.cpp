#include "../RestfulEzreal.h"
#include <array>
#include <sstream>
#include <stdexcept>

#define TFT 2

// ##################
#define LEAGUE 0

#define CHALLENGER 0
#define GRANDMASTER 1
#define MASTER 2
#define L_BY_SUMMONER_ID 3
#define L_BY_LEAGUE_ID 4
#define QUEUE_TOP 5
#define L_BY_TIER_DIVISION 6

// ##################
#define MATCH 1

#define M_BY_PUUID 0
#define M_BY_MATCH_ID 1
// ##################
#define STATUS 2

#define S_STATUS 0
// ##################
#define SUMMONER 3

#define S_BY_ACCOUNT_ID 0
#define S_BY_NAME 1
#define S_BY_PUUID 2
#define S_BY_SUMMONER_ID 3

namespace restfulEz {
    static std::array<QUERY_FORM, 7> Tft_League = {
        QUERY_FORM(1, TFT, LEAGUE, CHALLENGER, "Teamfight Tactics", "League", "Challenger", {"Routing"}, {_NO_FLAG}),
        QUERY_FORM(1, TFT, LEAGUE, GRANDMASTER, "Teamfight Tactics", "League", "Grandmaster", {"Routing"}, {_NO_FLAG}),
        QUERY_FORM(1, TFT, LEAGUE, MASTER, "Teamfight Tactics", "League", "Master", {"Routing"}, {_NO_FLAG}),
        QUERY_FORM(2, TFT, LEAGUE, L_BY_SUMMONER_ID, "Teamfight Tactics", "League", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, TFT, LEAGUE, L_BY_LEAGUE_ID, "Teamfight Tactics", "League", "By League ID", {"Routing", "League ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, TFT, LEAGUE, QUEUE_TOP, "Teamfight Tactics", "League", "Queue Top", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(3, TFT, LEAGUE, L_BY_TIER_DIVISION, "Teamfight Tactics", "League", "By Tier Division", {"Routing", "Tier", "Divisionk"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 2> Tft_Match = {
        QUERY_FORM(2, TFT, MATCH, M_BY_PUUID, "Teamfight Tactics", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, TFT, MATCH, M_BY_MATCH_ID, "Teamfight Tactics", "Match", "By Match ID", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 1> Tft_Status = {
        QUERY_FORM(1, TFT, STATUS, S_STATUS, "Teamfight Tactics", "Status", "Status", {"Routing"}, {_NO_FLAG})
    };
    static std::array<QUERY_FORM, 4> Tft_Summoner = {
        QUERY_FORM(2, TFT, SUMMONER, S_BY_ACCOUNT_ID, "Teamfight Tactics", "Summoner", "By Account ID", {"Routing", "Account ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, TFT, SUMMONER, S_BY_NAME, "Teamfight Tactics", "Summoner", "By Name", {"Routing", "Name"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, TFT, SUMMONER, S_BY_PUUID, "Teamfight Tactics", "Summoner", "By PUUID", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, TFT, SUMMONER, S_BY_SUMMONER_ID, "Teamfight Tactics", "Summoner", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG})
    };

    void RestfulEzreal::NewQueryFormTFT(int end_name, int endpoint_method) {
        switch (end_name) {
            case LEAGUE:
                this->pushNewForm(Tft_League.at(endpoint_method)); break;
            case MATCH:
                this->pushNewForm(Tft_Match.at(endpoint_method)); break;
            case STATUS:
                this->pushNewForm(Tft_Status.at(endpoint_method)); break;
            case SUMMONER:
                this->pushNewForm(Tft_Summoner.at(endpoint_method)); break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index");
        }
    }

    Json::Value RestfulEzreal::Send_TFT() {
        Json::Value response;

        std::vector<PARAM_CONT>& params = this->_next_request->_req_in_form.params;
        const int endpoint = this->_next_request->_req_in_form._endpoint;
        const int endpoint_method = this->_next_request->_req_in_form._endpoint_method;
        try {
            switch (endpoint) {
                case LEAGUE:
                    switch (endpoint_method) {
                        case CHALLENGER:
                            response = this->_underlying_client->Tft_League.challenger(params.at(0));break;
                        case GRANDMASTER:
                            response = this->_underlying_client->Tft_League.grandmaster(params.at(0));break;
                        case MASTER:
                            response = this->_underlying_client->Tft_League.master(params.at(0));break;
                        case L_BY_SUMMONER_ID:
                            response = this->_underlying_client->Tft_League.by_summoner_id(params.at(0), params.at(1));break;
                        case L_BY_LEAGUE_ID:
                            response = this->_underlying_client->Tft_League.by_league_id(params.at(0), params.at(1));break;
                        case QUEUE_TOP:
                            response = this->_underlying_client->Tft_League.queue_top(params.at(0), params.at(1));break;
                        case L_BY_TIER_DIVISION:
                            response = this->_underlying_client->Tft_League.by_tier_division(params.at(0), params.at(1), params.at(2));break;
                        default:
                            throw std::invalid_argument("Invalid Endpoint Method Index Given for Tft_League");
                    } break;
                case MATCH:
                    switch (endpoint_method) {
                        case M_BY_PUUID:
                            response = this->_underlying_client->Tft_Match.by_puuid(params.at(0), params.at(1));break;
                        case M_BY_MATCH_ID:
                            response = this->_underlying_client->Tft_Match.by_match(params.at(0), params.at(1));break;
                    } break;
                case STATUS:
                    response = this->_underlying_client->Tft_Status.v1(params.at(0));break;
                case SUMMONER:
                    switch (endpoint_method) {
                        case S_BY_ACCOUNT_ID:
                            response = this->_underlying_client->Tft_Summoner.by_account(params.at(0), params.at(1));break;
                        case S_BY_NAME:
                            response = this->_underlying_client->Tft_Summoner.by_name(params.at(0), params.at(1));break;
                        case S_BY_PUUID:
                            response = this->_underlying_client->Tft_Summoner.by_puuid(params.at(0), params.at(1));break;
                        case S_BY_SUMMONER_ID:
                            response = this->_underlying_client->Tft_Summoner.by_summoner_id(params.at(0), params.at(1));break;
                    } break;
                default:
                    throw std::invalid_argument("Invaid Endpoint Index given");
            }
        }
        catch (std::out_of_range const& ex) {
            std::ostringstream ss;
            ss << ex.what() << " Tried to index too many parameters";
            throw std::out_of_range(ss.str());
        }

        return response;
    }
}
