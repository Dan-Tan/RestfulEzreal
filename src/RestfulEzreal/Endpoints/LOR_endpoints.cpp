#include "../RequestQueue.h"
#include "../RestfulEzreal.h"
#include <array>
#include <sstream>
#include <stdexcept>

#define LOR 3

// ##################
#define MATCH 0

#define M_BY_PUUID 0
#define M_BY_MATCH 1

// ##################
#define RANKED 1

#define LEADERBOARD 0

// ##################
#define STATUS 2

#define S_STATUS 0


namespace restfulEz { 

    std::shared_ptr<LinkedInterface> LinkedInterface::make_linked_LOR(const int game, const int end_name, const int endpoint_method) {

        try {
            switch (end_name) {
                case MATCH:
                    switch (endpoint_method) {
                    case M_BY_PUUID:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case M_BY_MATCH:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    } break;
                case RANKED:
                    switch (endpoint_method) {
                    case LEADERBOARD:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    } break;
                case STATUS:
                    switch (endpoint_method) {
                    case S_STATUS:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    } break;
            }
        } 
        catch (std::out_of_range const* ex) {
            std::ostringstream ss;
            ss << "Invalid Endpoint and Method arguement. (Endpoint, Method): (" << end_name << ", " << endpoint_method << ")";
            throw std::invalid_argument(ss.str());
        }
        throw std::invalid_argument("Invalid endpoint given");
    }

    BaseForm BaseForm::make_form_LOR(const int end_name, const int endpoint_method) {

        try {
            switch (end_name) {
                case MATCH:
                    switch (endpoint_method) {
                    case M_BY_PUUID:
                        return BaseForm(2, LOR, MATCH, M_BY_PUUID, "Legends of Runeterra", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case M_BY_MATCH:
                        return BaseForm(2, LOR, MATCH, M_BY_MATCH, "Legends of Runeterra", "Match", "By Match", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    } break;
                case RANKED:
                    switch (endpoint_method) {
                    case LEADERBOARD:
                        return BaseForm(1, LOR, RANKED, LEADERBOARD, "Legends of Runeterra", "Ranked", "Leaderboards", {"Routing"}, {_NO_FLAG}); break;
                    } break;
                case STATUS:
                    switch (endpoint_method) {
                    case S_STATUS:
                        return BaseForm(1, LOR, STATUS, S_STATUS, "Legends of Runeterra", "Status", "Status", {"Routing"}, {_NO_FLAG}); break;
                    } break;
            }
        } 
        catch (std::out_of_range const* ex) {
            std::ostringstream ss;
            ss << "Invalid Endpoint and Method arguement. (Endpoint, Method): (" << end_name << ", " << endpoint_method << ")";
            throw std::invalid_argument(ss.str());
        }
        throw std::invalid_argument("Invalid endpoint given");
    }

    using json_ptr = std::unique_ptr<std::vector<char>>;

    json_ptr RequestSender::Send_LOR(request& task) {
        json_ptr response;

        std::vector<PARAM_CONT>& params = task.params;
        const int endpoint = task._endpoint;
        const int endpoint_method = task._endpoint_method;
        
        try {
            switch (endpoint) {
                case MATCH:
                    switch (endpoint_method) {
                        case M_BY_PUUID:
                            response = this->underlying_client->Lor_Match.by_puuid(params.at(0), params.at(1));break;
                        case M_BY_MATCH:
                            response = this->underlying_client->Lor_Match.by_match(params.at(0), params.at(1));break;
                        default:
                            throw std::invalid_argument("Invalid Endpoint Method for LOR_MATCH, greater then 1 (strictly)");
                    }break;
                case RANKED:
                    response = this->underlying_client->Lor_Ranked.leaderboards(params.at(0));break;
                case STATUS:
                    response = this->underlying_client->Lor_Status.v1(params.at(0));break;
                default:
                    throw std::invalid_argument("Invalid Endpoint given, greater then 2 (strictly)");
            }
        }
        catch (std::out_of_range const& ex) {
            throw std::invalid_argument("Query Form param index out of range given, (too many arguements)");
        }
        return std::move(response);
    }
}
