#include "../RequestQueue.h"
#include "../RestfulEzreal.h"
#include <array>
#include <utility>
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

    std::shared_ptr<LinkedInterface> LinkedInterface::make_linked_LOL(const int game, const int end_name, const int endpoint_method) {
        // purely for readability
        switch (end_name) {
            case CHAMPION_MASTERY:
                switch (endpoint_method) {
                    case CM_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CM_BY_RIOT_ID:
                        return std::make_shared<LinkedForm<3>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CM_BY_GAME_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CM_SCORE_BY_SUMMONER:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case CHAMPION_ROTATION:
                switch (endpoint_method) {
                    case CR_ROTATION:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case CLASH:
                switch (endpoint_method) {
                    case CL_BY_SUMMONER_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CL_BY_TEAM:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CL_TOURNAMENT_BY_TEAM:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CL_BY_TOURNAMENT:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case LEAGUE:
                switch (endpoint_method) {
                    case CHALLENGER:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case GRANDMASTER:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case MASTER:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case L_BY_SUMMONER_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case L_BY_LEAGUE_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case L_SPECIFIC_LEAGUE:
                        return std::make_shared<LinkedForm<4>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case L_EXPERIMENTAL:
                        return std::make_shared<LinkedForm<4>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case CHALLENGES:
                switch (endpoint_method) {
                    case CH_CONFIGURATION:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CH_PERCENTILES:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CH_CH_CONFIGURATION:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CH_CH_LEADERBOARD:
                        return std::make_shared<LinkedForm<3>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CH_CH_PERCENTILES:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case CH_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case STATUS:
                switch (endpoint_method) {
                    case V4:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case V3:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case MATCH:
                switch (endpoint_method) {
                    case M_BY_MATCH_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case M_TIMELINE:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case M_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case SUMMONER:
                switch (endpoint_method) {
                    case S_BY_RSO_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case S_BY_ACCOUNT_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case S_BY_NAME:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case S_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case S_BY_SUMMONER_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case SPECTATOR:
                switch (endpoint_method) {
                    case SP_BY_SUMMONER_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case SP_FEATURED:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
        };
        throw std::invalid_argument("Invalid Endpoint Index");
    };

    BaseForm BaseForm::make_form_LOL(const int end_name, const int endpoint_method) {
        // purely for readability
        switch (end_name) {
            case CHAMPION_MASTERY:
                switch (endpoint_method) {
                    case CM_BY_PUUID:
                        return BaseForm(2, LOL, CHAMPION_MASTERY, CM_BY_PUUID, "League of Legends", "Champion Mastery", "By Summoner Id", {"Routing","Summoner ID"},{_NO_FLAG,_NO_FLAG}); break;
                    case CM_BY_RIOT_ID:
                        return BaseForm(3, LOL, CHAMPION_MASTERY, CM_BY_RIOT_ID, "League of Legends", "Champion Mastery", "By Summoner By Champion", {"Routing", "Summoner ID", "Champion ID"},{_NO_FLAG,_NO_FLAG,_DEC_FLAG}); break;
                    case CM_BY_GAME_BY_PUUID:
                        return BaseForm(2, LOL, CHAMPION_MASTERY, CM_BY_GAME_BY_PUUID, "League of Legends", "Champion Mastery", "Top By Summoner", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}, true, {"count"}, {"3"}, {_DEC_FLAG}); break;
                    case CM_SCORE_BY_SUMMONER:
                        return BaseForm(2, LOL, CHAMPION_MASTERY, CM_SCORE_BY_SUMMONER, "League of Legends", "Champion Mastery", "Score By Summoner", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}); break;
                } break;
            case CHAMPION_ROTATION:
                switch (endpoint_method) {
                    case CR_ROTATION:
                        return BaseForm(1, LOL, CHAMPION_ROTATION, CR_ROTATION, "League of Legends", "Champion_Rotation", "Rotation", {"Routing"}, {_NO_FLAG}); break;
                } break;
            case CLASH:
                switch (endpoint_method) {
                    case CL_BY_SUMMONER_ID:
                        return BaseForm(2, LOL, CLASH, CL_BY_SUMMONER_ID, "League of Legends", "Clash", "By Summoner Id", {"Routing", "Summoner Id"}, {_NO_FLAG, _NO_FLAG}); break;
                    case CL_BY_TEAM:
                        return BaseForm(2, LOL, CLASH, CL_BY_TEAM, "League of Legends", "Clash", "By Team", {"Routing", "Team"}, {_NO_FLAG, _NO_FLAG}); break;
                    case CL_TOURNAMENT_BY_TEAM:
                        return BaseForm(2, LOL, CLASH, CL_TOURNAMENT_BY_TEAM, "League of Legends", "Clash", "Tournament By Team", {"Routing", "Team"}, {_NO_FLAG, _NO_FLAG}); break;
                    case CL_BY_TOURNAMENT:
                        return BaseForm(2, LOL, CLASH, CL_BY_TOURNAMENT, "League of Legends", "Clash", "By Tourament", {"Routing", "Tourament"}, {_NO_FLAG, _NO_FLAG}); break;
                } break;
            case LEAGUE:
                switch (endpoint_method) {
                    case CHALLENGER:
                        return BaseForm(2, LOL, LEAGUE, CHALLENGER, "League of Legends", "League", "Challenger", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}); break;
                    case GRANDMASTER:
                        return BaseForm(2, LOL, LEAGUE, GRANDMASTER, "League of Legends", "League", "Grandmaster", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}); break;
                    case MASTER:
                        return BaseForm(2, LOL, LEAGUE, MASTER, "League of Legends", "League", "Master", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}); break;
                    case L_BY_SUMMONER_ID:
                        return BaseForm(2, LOL, LEAGUE, L_BY_SUMMONER_ID, "League of Legends", "League", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case L_BY_LEAGUE_ID:
                        return BaseForm(2, LOL, LEAGUE, L_BY_LEAGUE_ID, "League of Legends", "League", "By League ID", {"Routing", "League ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case L_SPECIFIC_LEAGUE:
                        return BaseForm(4, LOL, LEAGUE, L_SPECIFIC_LEAGUE, "League of Legends", "League", "Specific League", {"Routing", "Queue", "Tier", "Division"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG, _NO_FLAG}, true, {"page"}, {"1"}, {_DEC_FLAG}); break;
                    case L_EXPERIMENTAL:
                        return BaseForm(4, LOL, LEAGUE, L_EXPERIMENTAL, "League of Legends", "League", "Experimental", {"Routing", "Queue", "Tier", "Division"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG, _NO_FLAG}, true, {"page"}, {"1"}, {_DEC_FLAG}); break;
                } break;
            case CHALLENGES:
                switch (endpoint_method) {
                    case CH_CONFIGURATION:
                        return BaseForm(1, LOL, CHALLENGES, CH_CONFIGURATION, "League of Legends", "Challenges", "Configuration", {"Routing"}, {_NO_FLAG}); break;
                    case CH_PERCENTILES:
                        return BaseForm(1, LOL, CHALLENGES, CH_PERCENTILES, "League of Legends", "Challenges", "Percentiles", {"Routing"}, {_NO_FLAG}); break;
                    case CH_CH_CONFIGURATION:
                        return BaseForm(2, LOL, CHALLENGES, CH_CH_CONFIGURATION, "League of Legends", "Challenges", "Challenge Configuration", {"Routing", "Challenge ID"}, {_NO_FLAG, _DEC_FLAG}); break;
                    case CH_CH_LEADERBOARD:
                        return BaseForm(3, LOL, CHALLENGES, CH_CH_LEADERBOARD, "League of Legends", "Challenges", "Challenge Leaderboards", {"Routing", "Challenge ID", "Level"}, {_NO_FLAG, _DEC_FLAG, _NO_FLAG}, true, {"limit"}, {""}, {_DEC_FLAG}); break;
                    case CH_CH_PERCENTILES:
                        return BaseForm(2, LOL, CHALLENGES, CH_CH_PERCENTILES, "League of Legends", "Challenges", "Challenge Percentiles", {"Routing", "Challenge ID"}, {_NO_FLAG, _DEC_FLAG}); break;
                    case CH_BY_PUUID:
                        return BaseForm(2, LOL, CHALLENGES, CH_BY_PUUID, "League of Legends", "Challenges", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}); break;
                } break;
            case STATUS:
                switch (endpoint_method) {
                    case V4:
                        return BaseForm(1, LOL, STATUS, V4, "League of Legends", "Status", "Default (v4)", {"Routing"}, {_NO_FLAG}); break;
                    case V3:
                        return BaseForm(1, LOL, STATUS, V3, "League of Legends", "Status", "v3", {"Routing"}, {_NO_FLAG}); break;
                } break;
            case MATCH:
                switch (endpoint_method) {
                    case M_BY_MATCH_ID:
                        return BaseForm(2, LOL, MATCH, M_BY_MATCH_ID, "League of Legends", "Match", "By Match ID", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG }); break;
                    case M_TIMELINE:
                        return BaseForm(2, LOL, MATCH, M_TIMELINE, "League of Legends", "Match", "Timeline", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case M_BY_PUUID:
                        return BaseForm(2, LOL, MATCH, M_BY_PUUID, "League of Legends", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}, true, {"startTime", "endTime", "queue", "type", "start", "count"}, {"", "", "", "", "0", "20"}, {_DEC_FLAG, _DEC_FLAG, _DEC_FLAG, _NO_FLAG, _DEC_FLAG, _DEC_FLAG}); break;
                } break;
            case SUMMONER:
                switch (endpoint_method) {
                    case S_BY_RSO_PUUID:
                        return BaseForm(2, LOL, SUMMONER, S_BY_RSO_PUUID, "League of Legends", "Summoner", "By RSO Puuid", {"Routing", "RSO PUUID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case S_BY_ACCOUNT_ID:
                        return BaseForm(2, LOL, SUMMONER, S_BY_ACCOUNT_ID, "League of Legends", "Summoner", "By Account ID", {"Routing", "Account ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case S_BY_NAME:
                        return BaseForm(2, LOL, SUMMONER, S_BY_NAME, "League of Legends", "Summoner", "By Name", {"Routing", "Name"}, {_NO_FLAG, _NO_FLAG}); break;
                    case S_BY_PUUID:
                        return BaseForm(2, LOL, SUMMONER, S_BY_PUUID, "League of Legends", "Summoner", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case S_BY_SUMMONER_ID:
                        return BaseForm(2, LOL, SUMMONER, S_BY_SUMMONER_ID, "League of Legends", "Summoner", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG }); break;
                } break;
            case SPECTATOR:
                switch (endpoint_method) {
                    case SP_BY_SUMMONER_ID:
                        return BaseForm(2, LOL, SPECTATOR, SP_BY_SUMMONER_ID, "League of Legends", "Spectator", "By Summoner Id", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG }); break;
                    case SP_FEATURED:
                        return BaseForm(1, LOL, SPECTATOR, SP_FEATURED, "League of Legends", "Spectator", "Featured Games", {"Routing"}, {_NO_FLAG}); break;
                } break;
        };
        throw std::invalid_argument("Invalid Endpoint Index");

    };

    using json_ptr = std::unique_ptr<std::vector<char>>;

    json_ptr RequestSender::Send_LOL(request& task) {
        json_ptr response;

        std::vector<PARAM_CONT>& params = task.params;
        std::vector<P_NAME>& opt_names = task.optional_names; // WARNING MIGHT NOT BE INITIALISED/EMPTY
        std::vector<PARAM_CONT>& opt_inputs = task.optional_inputs; // WARNING MIGHT NOT BE INITIALISED/EMPTY
        const int endpoint = task._endpoint;
        const int endpoint_method = task._endpoint_method;

        switch (endpoint) {
            case CHAMPION_MASTERY: // Champion Mastery
                switch (endpoint_method) {
                    case CM_BY_PUUID:
                        response = this->underlying_client->Champion_Mastery.by_summoner_id(params.at(0), params.at(1)); break;
                    case CM_BY_RIOT_ID:
                        response = this->underlying_client->Champion_Mastery.by_summoner_by_champion(params.at(0), params.at(1), atoi(params.at(2))); break;
                    case CM_BY_GAME_BY_PUUID:
                        response = this->underlying_client->Champion_Mastery.by_summoner_top(params.at(0), params.at(1), std::pair<std::string, int>(opt_names.at(0), atoi(opt_inputs.at(0)))); break;
                    case CM_SCORE_BY_SUMMONER:
                        response = this->underlying_client->Champion_Mastery.scores_by_summoner(params.at(0), params.at(1)); break;
                } break;
            case CHAMPION_ROTATION: // Champion Rotation
                response = this->underlying_client->Champion.rotations(params.at(0));break;
            case CLASH: // Clash
                switch (endpoint_method) {
                    case CL_BY_SUMMONER_ID:
                        response = this->underlying_client->Clash.by_summoner_id(params.at(0), params.at(1)); break;
                    case CL_BY_TEAM:
                        response = this->underlying_client->Clash.by_team(params.at(0), params.at(1)); break;
                    case CL_TOURNAMENT_BY_TEAM:
                        response = this->underlying_client->Clash.tournament_by_team(params.at(0), params.at(1)); break;
                    case CL_BY_TOURNAMENT:
                        response = this->underlying_client->Clash.by_tournament(params.at(0), params.at(1)); break;
                }break;
            case LEAGUE: // League
                switch (endpoint_method) {
                    case CHALLENGER:
                        response = this->underlying_client->League.challenger(params.at(0), params.at(1)); break;
                    case GRANDMASTER:
                        response = this->underlying_client->League.grandmaster(params.at(0), params.at(1)); break;
                    case MASTER:
                        response = this->underlying_client->League.master(params.at(0), params.at(1)); break;
                    case L_BY_SUMMONER_ID:
                        response = this->underlying_client->League.by_summoner_id(params.at(0), params.at(1)); break;
                    case L_BY_LEAGUE_ID:
                        response = this->underlying_client->League.by_league_id(params.at(0), params.at(1)); break;
                    case L_SPECIFIC_LEAGUE:
                        response = this->underlying_client->League.specific_league(params.at(0), params.at(1), params.at(2), params.at(3), std::pair<std::string, int>(opt_names.at(0), atoi(opt_inputs.at(0)))); break; 
                    case L_EXPERIMENTAL:
                        response = this->underlying_client->League_Exp.entries(params.at(0), params.at(1), params.at(2), params.at(3), std::pair<std::string, int>(opt_names.at(0), atoi(opt_inputs.at(0)))); break;
                }break;
            case CHALLENGES: // Challenges
                switch (endpoint_method) {
                    case CH_CONFIGURATION:
                        response = this->underlying_client->Lol_Challenges.config(params.at(0)); break;
                    case CH_PERCENTILES:
                        response = this->underlying_client->Lol_Challenges.percentiles(params.at(0)); break;
                    case CH_CH_CONFIGURATION:
                        response = this->underlying_client->Lol_Challenges.challenge_config(params.at(0), atoi(params.at(1))); break;
                    case CH_CH_LEADERBOARD:
                        response = this->underlying_client->Lol_Challenges.challenge_leaderboard(params.at(0), atoi(params.at(2)), params.at(2), std::pair<std::string, int>(opt_names.at(0), atoi(opt_inputs.at(0)))); break;
                    case CH_CH_PERCENTILES:
                        response = this->underlying_client->Lol_Challenges.challenge_percentiles(params.at(0), atoi(params.at(1))); break;
                    case CH_BY_PUUID:
                        response = this->underlying_client->Lol_Challenges.by_puuid(params.at(0), params.at(1)); break;
                }break;
            case STATUS: // Lol_Status
                switch (endpoint_method) {
                    case V4:
                        response = this->underlying_client->Lol_Status.v4(params.at(0)); break;
                    case V3:
                        response = this->underlying_client->Lol_Status.v3(params.at(0)); break;
                }break;
            case MATCH: // Match
                switch (endpoint_method) {
                    case M_BY_MATCH_ID:
                        response = this->underlying_client->Match.by_match_id(params.at(0), params.at(1)); break;
                    case M_TIMELINE:
                        response = this->underlying_client->Match.timeline(params.at(0), params.at(1)); break;
                    case M_BY_PUUID:
                        response = this->underlying_client->Match.by_puuid(params.at(0), params.at(1), 
                                std::pair<std::string, std::string>(opt_names.at(0), opt_inputs.at(0)),
                                std::pair<std::string, int>(opt_names.at(1), atoi(opt_inputs.at(1))),
                                std::pair<std::string, int>(opt_names.at(2), atoi(opt_inputs.at(2))),
                                std::pair<std::string, int>(opt_names.at(3), atoi(opt_inputs.at(3))),
                                std::pair<std::string, int>(opt_names.at(4), atoi(opt_inputs.at(4))),
                                std::pair<std::string, int>(opt_names.at(5), atoi(opt_inputs.at(5)))
                                ); break;
                }break;
            case SUMMONER: // Summoner
                switch (endpoint_method) {
                    case S_BY_RSO_PUUID:
                        response = this->underlying_client->Summoner.by_rso_puuid(params.at(0), params.at(1)); break;
                    case S_BY_ACCOUNT_ID:
                        response = this->underlying_client->Summoner.by_account_id(params.at(0), params.at(1)); break;
                    case S_BY_NAME:
                        response = this->underlying_client->Summoner.by_name(params.at(0), params.at(1)); break;
                    case S_BY_PUUID:
                        response = this->underlying_client->Summoner.by_puuid(params.at(0), params.at(1)); break;
                    case S_BY_SUMMONER_ID:
                        response = this->underlying_client->Summoner.by_summoner_id(params.at(0), params.at(1)); break;
                }break;
            case SPECTATOR: // Spectator
                switch (endpoint_method) {
                    case SP_BY_SUMMONER_ID:
                        response = this->underlying_client->Spectator.by_summoner_id(params.at(0), params.at(1)); break;
                    case SP_FEATURED:
                        response = this->underlying_client->Spectator.featured_games(params.at(0)); break;
                }break;
            default: // Catch Exception
                throw std::invalid_argument("Invalid Endpoint");
        };
        return std::move(response);
    }
}
