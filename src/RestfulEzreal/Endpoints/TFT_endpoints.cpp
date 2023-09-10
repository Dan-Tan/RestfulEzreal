#include "../RequestQueue.h"
#include "../RestfulEzreal.h"
#include <array>
#include <sstream>
#include <stdexcept>
#include <utility>

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

    std::shared_ptr<LinkedInterface> LinkedInterface::make_linked_TFT(const int game, const int end_name, const int endpoint_method) {
        switch (end_name) {
            case LEAGUE:
                switch (endpoint_method) {
                    case CHALLENGER:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case GRANDMASTER:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case MASTER:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case L_BY_SUMMONER_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case L_BY_LEAGUE_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case QUEUE_TOP:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case L_BY_TIER_DIVISION:
                        return std::make_shared<LinkedForm<3>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case MATCH:
                switch (endpoint_method) {
                    case M_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case M_BY_MATCH_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case STATUS:
                switch (endpoint_method) {
                    case S_STATUS:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case SUMMONER:
                switch (endpoint_method) {
                    case S_BY_ACCOUNT_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case S_BY_NAME:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case S_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case S_BY_SUMMONER_ID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index");
        }
        throw std::invalid_argument("Invalid Endpoint Index");
    }

    BaseForm BaseForm::make_form_TFT(const int end_name, const int endpoint_method) {
        switch (end_name) {
            case LEAGUE:
                switch (endpoint_method) {
                    case CHALLENGER:
                        return BaseForm(1, TFT, LEAGUE, CHALLENGER, "Teamfight Tactics", "League", "Challenger", {"Routing"}, {_NO_FLAG}); break;
                    case GRANDMASTER:
                        return BaseForm(1, TFT, LEAGUE, GRANDMASTER, "Teamfight Tactics", "League", "Grandmaster", {"Routing"}, {_NO_FLAG}); break;
                    case MASTER:
                        return BaseForm(1, TFT, LEAGUE, MASTER, "Teamfight Tactics", "League", "Master", {"Routing"}, {_NO_FLAG}); break;
                    case L_BY_SUMMONER_ID:
                        return BaseForm(2, TFT, LEAGUE, L_BY_SUMMONER_ID, "Teamfight Tactics", "League", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case L_BY_LEAGUE_ID:
                        return BaseForm(2, TFT, LEAGUE, L_BY_LEAGUE_ID, "Teamfight Tactics", "League", "By League ID", {"Routing", "League ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case QUEUE_TOP:
                        return BaseForm(2, TFT, LEAGUE, QUEUE_TOP, "Teamfight Tactics", "League", "Queue Top", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}); break;
                    case L_BY_TIER_DIVISION:
                        return BaseForm(3, TFT, LEAGUE, L_BY_TIER_DIVISION, "Teamfight Tactics", "League", "By Tier Division", {"Routing", "Tier", "Divisionk"}, {_NO_FLAG, _NO_FLAG, _NO_FLAG}, true, {"count"}, {"1"}, {_DEC_FLAG}); break;
                } break;
            case MATCH:
                switch (endpoint_method) {
                    case M_BY_PUUID:
                        return BaseForm(2, TFT, MATCH, M_BY_PUUID, "Teamfight Tactics", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}, true, {"start", "endTime", "startTime", "count"}, {"0", "", "", "20"}, {_DEC_FLAG, _DEC_FLAG, _DEC_FLAG, _DEC_FLAG}); break;
                    case M_BY_MATCH_ID:
                        return BaseForm(2, TFT, MATCH, M_BY_MATCH_ID, "Teamfight Tactics", "Match", "By Match ID", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}); break;
                } break;
            case STATUS:
                switch (endpoint_method) {
                    case S_STATUS:
                        return BaseForm(1, TFT, STATUS, S_STATUS, "Teamfight Tactics", "Status", "Status", {"Routing"}, {_NO_FLAG}); break;
                } break;
            case SUMMONER:
                switch (endpoint_method) {
                    case S_BY_ACCOUNT_ID:
                        return BaseForm(2, TFT, SUMMONER, S_BY_ACCOUNT_ID, "Teamfight Tactics", "Summoner", "By Account ID", {"Routing", "Account ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case S_BY_NAME:
                        return BaseForm(2, TFT, SUMMONER, S_BY_NAME, "Teamfight Tactics", "Summoner", "By Name", {"Routing", "Name"}, {_NO_FLAG, _NO_FLAG}); break;
                    case S_BY_PUUID:
                        return BaseForm(2, TFT, SUMMONER, S_BY_PUUID, "Teamfight Tactics", "Summoner", "By PUUID", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case S_BY_SUMMONER_ID:
                        return BaseForm(2, TFT, SUMMONER, S_BY_SUMMONER_ID, "Teamfight Tactics", "Summoner", "By Summoner ID", {"Routing", "Summoner ID"}, {_NO_FLAG, _NO_FLAG}); break;
                } break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index");
        }
        throw std::invalid_argument("Invalid Endpoint Index");
    }

    using json_ptr = std::unique_ptr<std::vector<char>>;

    json_ptr RequestSender::Send_TFT(request& task) {
        json_ptr response;

        std::vector<PARAM_CONT>& params = task.params;
        std::vector<P_NAME>& opt_names = task.optional_names;
        std::vector<PARAM_CONT>& opt_inputs = task.optional_inputs;
        const int endpoint = task._endpoint;
        const int endpoint_method = task._endpoint_method;
        try {
            switch (endpoint) {
                case LEAGUE:
                    switch (endpoint_method) {
                        case CHALLENGER:
                            response = this->underlying_client->Tft_League.challenger(params.at(0));break;
                        case GRANDMASTER:
                            response = this->underlying_client->Tft_League.grandmaster(params.at(0));break;
                        case MASTER:
                            response = this->underlying_client->Tft_League.master(params.at(0));break;
                        case L_BY_SUMMONER_ID:
                            response = this->underlying_client->Tft_League.by_summoner_id(params.at(0), params.at(1));break;
                        case L_BY_LEAGUE_ID:
                            response = this->underlying_client->Tft_League.by_league_id(params.at(0), params.at(1));break;
                        case QUEUE_TOP:
                            response = this->underlying_client->Tft_League.queue_top(params.at(0), params.at(1));break;
                        case L_BY_TIER_DIVISION:
                            response = this->underlying_client->Tft_League.by_tier_division(params.at(0), params.at(1), params.at(2), std::pair<std::string, int>(opt_names.at(0), atoi(opt_inputs.at(0))));break;
                        default:
                            throw std::invalid_argument("Invalid Endpoint Method Index Given for Tft_League");
                    } break;
                case MATCH:
                    switch (endpoint_method) {
                        case M_BY_PUUID:
                            response = this->underlying_client->Tft_Match.by_puuid(params.at(0), params.at(1), 
                                    std::pair<std::string, int>(opt_names.at(0), atoi(opt_inputs.at(0))),
                                    std::pair<std::string, int>(opt_names.at(1), atoi(opt_inputs.at(1))),
                                    std::pair<std::string, int>(opt_names.at(2), atoi(opt_inputs.at(2))),
                                    std::pair<std::string, int>(opt_names.at(3), atoi(opt_inputs.at(3)))
                                        );break;
                        case M_BY_MATCH_ID:
                            response = this->underlying_client->Tft_Match.by_match(params.at(0), params.at(1));break;
                    } break;
                case STATUS:
                    response = this->underlying_client->Tft_Status.v1(params.at(0));break;
                case SUMMONER:
                    switch (endpoint_method) {
                        case S_BY_ACCOUNT_ID:
                            response = this->underlying_client->Tft_Summoner.by_account(params.at(0), params.at(1));break;
                        case S_BY_NAME:
                            response = this->underlying_client->Tft_Summoner.by_name(params.at(0), params.at(1));break;
                        case S_BY_PUUID:
                            response = this->underlying_client->Tft_Summoner.by_puuid(params.at(0), params.at(1));break;
                        case S_BY_SUMMONER_ID:
                            response = this->underlying_client->Tft_Summoner.by_summoner_id(params.at(0), params.at(1));break;
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
        return std::move(response);
    }
}
