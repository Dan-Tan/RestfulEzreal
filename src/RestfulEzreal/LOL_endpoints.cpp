#include "Walnut/Application.h"
#include "RestfulEzreal.h"
#include <array>

// GAME
#define LOL 1

// ENDPOINTS/METHOD indices

// ###################
#define CHAMPION_MASTERY 0

#define CM_BY_PUUID 0
#define CM_BY_RIOT_ID 1
#define CM_BY_GAME_BY_PUUID 2
#define CM_SCORE_BY_SUMMONER 3

// ###################
#define CHAMPION_ROTATION 1

#define CR_ROTATION 0


// ###################
#define CLASH 2

#define CL_BY_SUMMONER_ID 0 
#define CL_BY_TEAM 1
#define CL_TOURNAMENT_BY_TEAM 2
#define CL_BY_TOURNAMENT 3

// ###################
#define LEAGUE 3

#define CHALLENGER 0
#define GRANDMASTER 1
#define MASTER 2
#define L_BY_SUMMONER_ID 3
#define L_BY_LEAGUE_ID 4
#define L_SPECIFIC_LEAGUE 5
#define L_EXPERIMENTAL 6

// ##################
#define CHALLENGES 4

#define CH_CONFIGURATION 0 
#define CH_PERCENTILES 1
#define CH_CH_CONFIGURATION 2
#define CH_CH_LEADERBOARD 3
#define CH_CH_PERCENTILES 4
#define CH_BY_PUUID 5

// ##################
#define STATUS 5

#define V4 0
#define V3 1

// ##################
#define MATCH 6

#define M_BY_MATCH_ID 0
#define M_TIMELINE 1
#define M_BY_PUUID 2

// ##################
#define SUMMONER 7

#define S_BY_RSO_PUUID 0
#define S_BY_ACCOUNT_ID 1
#define S_BY_NAME 2
#define S_BY_PUUID 3
#define S_BY_SUMMONER_ID 4

// ##################
#define SPECTATOR 8

#define SP_BY_SUMMONER_ID 0
#define SP_FEATURED 1

namespace restfulEz {

    static std::array<QUERY_FORM, 4> Champion_Mastery = {
        QUERY_FORM(2, LOL, CHAMPION_MASTERY, CM_BY_PUUID, "League of Legends", "Champion Mastery", "By Summoner Id", {"Routing","Summoner ID"},{_NO_FLAG,_NO_FLAG}),
        QUERY_FORM(3, LOL, CHAMPION_MASTERY, CM_BY_RIOT_ID, "League of Legends", "Champion Mastery", "By Summoner By Champion", {"Routing", "Summoner ID", "Champion ID"},{_NO_FLAG,_NO_FLAG,_DEC_FLAG}),
        QUERY_FORM(3, LOL, CHAMPION_MASTERY, CM_BY_GAME_BY_PUUID, "League of Legends", "Champion Mastery", "Top By Summoner", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, CHAMPION_MASTERY, CM_SCORE_BY_SUMMONER, "League of Legends", "Champion Mastery", "Score By Summoner", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 1> Champion_Rotation = {
        QUERY_FORM(1, LOL, CHAMPION_ROTATION, CR_ROTATION, "League of Legends", "Champion_Rotation", "Rotation", {"Routing"}, {_NO_FLAG})
    };
    static std::array<QUERY_FORM, 4> Clash = {
        QUERY_FORM(2, LOL, CLASH, CL_BY_SUMMONER_ID, "League of Legends", "Clash", "By Summoner Id", {"Routing", "Summoner Id"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, CLASH, CL_BY_TEAM, "League of Legends", "Clash", "By Team", {"Routing", "Team"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, CLASH, CL_TOURNAMENT_BY_TEAM, "League of Legends", "Clash", "Tournament By Team", {"Routing", "Team"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, CLASH, CL_BY_TOURNAMENT, "League of Legends", "Clash", "By Tourament", {"Routing", "Tourament"}, {_NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 7> League = {
        QUERY_FORM(2, LOL, LEAGUE, CHALLENGER, "League of Legends", "League", "Challenger", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, LEAGUE, GRANDMASTER, "League of Legends", "League", "Grandmaster", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, LEAGUE, MASTER, "League of Legends", "League", "Master", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, LEAGUE, L_BY_SUMMONER_ID, "League of Legends", "League", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, LEAGUE, L_BY_LEAGUE_ID, "League of Legends", "League", "By League ID", {"Routing", "League ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(4, LOL, LEAGUE, L_SPECIFIC_LEAGUE, "League of Legends", "League", "Specific League", {"Routing", "Queue", "Tier", "Division"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG, _NO_FLAG}),
        QUERY_FORM(4, LOL, LEAGUE, L_EXPERIMENTAL, "League of Legends", "League", "Experimental", {"Routing", "Queue", "Tier", "Division"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 6> Challenges = {
        QUERY_FORM(1, LOL, CHALLENGES, CH_CONFIGURATION, "League of Legends", "Challenges", "Configuration", {"Routing"}, {_NO_FLAG}),
        QUERY_FORM(1, LOL, CHALLENGES, CH_PERCENTILES, "League of Legends", "Challenges", "Percentiles", {"Routing"}, {_NO_FLAG}),
        QUERY_FORM(2, LOL, CHALLENGES, CH_CH_CONFIGURATION, "League of Legends", "Challenges", "Challenge Configuration", {"Routing", "Challenge ID"}, {_NO_FLAG, _DEC_FLAG}),
        QUERY_FORM(3, LOL, CHALLENGES, CH_CH_LEADERBOARD, "League of Legends", "Challenges", "Challenge Leaderboards", {"Routing", "Challenge ID", "Level"}, {_NO_FLAG, _DEC_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, CHALLENGES, CH_CH_PERCENTILES, "League of Legends", "Challenges", "Challenge Percentiles", {"Routing", "Challenge ID"}, {_NO_FLAG, _DEC_FLAG}),
        QUERY_FORM(2, LOL, CHALLENGES, CH_BY_PUUID, "League of Legends", "Challenges", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 2> Lol_Status = {
        QUERY_FORM(1, LOL, STATUS, V4, "League of Legends", "Status", "Default (v4)", {"Routing"}, {_NO_FLAG}),
        QUERY_FORM(1, LOL, STATUS, V3, "League of Legends", "Status", "v3", {"Routing"}, {_NO_FLAG})
    };
    static std::array<QUERY_FORM, 3> Match = {
        QUERY_FORM(2, LOL, MATCH, M_BY_MATCH_ID, "League of Legends", "Match", "By Match ID", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG }),
        QUERY_FORM(2, LOL, MATCH, M_TIMELINE, "League of Legends", "Match", "Timeline", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, MATCH, M_BY_PUUID, "League of Legends", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 5> Summoner = {
        QUERY_FORM(2, LOL, SUMMONER, S_BY_RSO_PUUID, "League of Legends", "Summoner", "By RSO Puuid", {"Routing", "RSO PUUID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, SUMMONER, S_BY_ACCOUNT_ID, "League of Legends", "Summoner", "By Account ID", {"Routing", "Account ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, SUMMONER, S_BY_NAME, "League of Legends", "Summoner", "By Name", {"Routing", "Name"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, SUMMONER, S_BY_PUUID, "League of Legends", "Summoner", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOL, SUMMONER, S_BY_SUMMONER_ID, "League of Legends", "Summoner", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG })
    };
    static std::array<QUERY_FORM, 2> Spectator = {
        QUERY_FORM(2, LOL, SPECTATOR, SP_BY_SUMMONER_ID, "League of Legends", "Spectator", "By Summoner Id", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG }),
        QUERY_FORM(1, LOL, SPECTATOR, SP_FEATURED, "League of Legends", "Spectator", "Featured Games", {"Routing"}, {_NO_FLAG})
    };

    void RestfulEzreal::NewQueryFormLOL(int end_name, int endpoint_method) {
        // purely for readability
        switch (end_name) {
            case CHAMPION_MASTERY:
                this->pushNewForm(Champion_Mastery[endpoint_method]); break;
            case CHAMPION_ROTATION:
                this->pushNewForm(Champion_Rotation[endpoint_method]); break;
            case CLASH:
                this->pushNewForm(Clash[endpoint_method]); break;
            case LEAGUE:
                this->pushNewForm(League[endpoint_method]); break;
            case CHALLENGES:
                this->pushNewForm(Challenges[endpoint_method]); break;
            case STATUS:
                this->pushNewForm(Lol_Status[endpoint_method]); break;
            case MATCH:
                this->pushNewForm(Match[endpoint_method]); break;
            case SUMMONER:
                this->pushNewForm(Summoner[endpoint_method]); break;
            case SPECTATOR:
                this->pushNewForm(Spectator[endpoint_method]); break;
        };

    };

    Json::Value RestfulEzreal::Send_LOL() {
        Json::Value response;

        std::vector<PARAM_CONT>& params = this->_next_request->_req_in_form.params;
        const int endpoint = this->_next_request->_req_in_form._endpoint;
        const int endpoint_method = this->_next_request->_req_in_form._endpoint_method;

        switch (endpoint) {
            case CHAMPION_MASTERY: // Champion Mastery
                switch (endpoint_method) {
                    case CM_BY_PUUID:
                        response = this->_underlying_client->Champion_Mastery.by_summoner_id(params.at(0), params.at(1)); break;
                    case CM_BY_RIOT_ID:
                        response = this->_underlying_client->Champion_Mastery.by_summoner_by_champion(params.at(0), params.at(1), atoi(params.at(2))); break;
                    case CM_BY_GAME_BY_PUUID:
                        response = this->_underlying_client->Champion_Mastery.by_summoner_id(params.at(0), params.at(1)); break;
                    case CM_SCORE_BY_SUMMONER:
                        response = this->_underlying_client->Champion_Mastery.by_summoner_id(params.at(0), params.at(1)); break;
                } break;
            case CHAMPION_ROTATION: // Champion Rotation
                response = this->_underlying_client->Champion.rotations(params.at(0));break;
            case CLASH: // Clash
                switch (endpoint_method) {
                    case CL_BY_SUMMONER_ID:
                        response = this->_underlying_client->Clash.by_summoner_id(params.at(0), params.at(1)); break;
                    case CL_BY_TEAM:
                        response = this->_underlying_client->Clash.by_team(params.at(0), params.at(1)); break;
                    case CL_TOURNAMENT_BY_TEAM:
                        response = this->_underlying_client->Clash.tournament_by_team(params.at(0), params.at(1)); break;
                    case CL_BY_TOURNAMENT:
                        response = this->_underlying_client->Clash.by_tournament(params.at(0), params.at(1)); break;
                }break;
            case LEAGUE: // League
                switch (endpoint_method) {
                    case CHALLENGER:
                        response = this->_underlying_client->League.challenger(params.at(0), params.at(1)); break;
                    case GRANDMASTER:
                        response = this->_underlying_client->League.grandmaster(params.at(0), params.at(1)); break;
                    case MASTER:
                        response = this->_underlying_client->League.master(params.at(0), params.at(1)); break;
                    case L_BY_SUMMONER_ID:
                        response = this->_underlying_client->League.by_summoner_id(params.at(0), params.at(1)); break;
                    case L_BY_LEAGUE_ID:
                        response = this->_underlying_client->League.by_league_id(params.at(0), params.at(1)); break;
                    case L_SPECIFIC_LEAGUE:
                        response = this->_underlying_client->League.specific_league(params.at(0), params.at(1), params.at(2), params.at(3));;; break;
                    case L_EXPERIMENTAL:
                        response = this->_underlying_client->League_Exp.entries(params.at(0), params.at(1), params.at(2), params.at(3)); break;
                }break;
            case CHALLENGES: // Challenges
                switch (endpoint_method) {
                    case CH_CONFIGURATION:
                        response = this->_underlying_client->Lol_Challenges.config(params.at(0)); break;
                    case CH_PERCENTILES:
                        response = this->_underlying_client->Lol_Challenges.percentiles(params.at(0)); break;
                    case CH_CH_CONFIGURATION:
                        response = this->_underlying_client->Lol_Challenges.challenge_config(params.at(0), atoi(params.at(1))); break;
                    case CH_CH_LEADERBOARD:
                        response = this->_underlying_client->Lol_Challenges.challenge_leaderboard(params.at(0), atoi(params.at(2)), params.at(2)); break;
                    case CH_CH_PERCENTILES:
                        response = this->_underlying_client->Lol_Challenges.challenge_percentiles(params.at(0), atoi(params.at(1))); break;
                    case CH_BY_PUUID:
                        response = this->_underlying_client->Lol_Challenges.by_puuid(params.at(0), params.at(1)); break;
                }break;
            case STATUS: // Lol_Status
                switch (endpoint_method) {
                    case V4:
                        response = this->_underlying_client->Lol_Status.v4(params.at(0)); break;
                    case V3:
                        response = this->_underlying_client->Lol_Status.v3(params.at(0)); break;
                }break;
            case MATCH: // Match
                switch (endpoint_method) {
                    case M_BY_MATCH_ID:
                        response = this->_underlying_client->Match.by_match_id(params.at(0), params.at(1)); break;
                    case M_TIMELINE:
                        response = this->_underlying_client->Match.timeline(params.at(0), params.at(1)); break;
                    case M_BY_PUUID:
                        response = this->_underlying_client->Match.by_puuid(params.at(0), params.at(1), { "", "" }); break;
                }break;
            case SUMMONER: // Summoner
                switch (endpoint_method) {
                    case S_BY_RSO_PUUID:
                        response = this->_underlying_client->Summoner.by_rso_puuid(params.at(0), params.at(1)); break;
                    case S_BY_ACCOUNT_ID:
                        response = this->_underlying_client->Summoner.by_account_id(params.at(0), params.at(1)); break;
                    case S_BY_NAME:
                        response = this->_underlying_client->Summoner.by_name(params.at(0), params.at(1)); break;
                    case S_BY_PUUID:
                        response = this->_underlying_client->Summoner.by_puuid(params.at(0), params.at(1)); break;
                    case S_BY_SUMMONER_ID:
                        response = this->_underlying_client->Summoner.by_summoner_id(params.at(0), params.at(1)); break;
                }break;
            case SPECTATOR: // Spectator
                switch (endpoint_method) {
                    case SP_BY_SUMMONER_ID:
                        response = this->_underlying_client->Spectator.by_summoner_id(params.at(0), params.at(1)); break;
                    case SP_FEATURED:
                        response = this->_underlying_client->Spectator.featured_games(params.at(0)); break;
                }break;
            default: // Catch Exception
                throw std::invalid_argument("Invalid Endpoint");
        };
        return response;
    }

}
