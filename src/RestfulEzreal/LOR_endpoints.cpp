#include "RestfulEzreal.h"
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

namespace restfulEz { // TODO: change to static factory method
    static std::array<QUERY_FORM, 2> Lor_Match = {
        QUERY_FORM(2, LOR, MATCH, M_BY_PUUID, "Legends of Runeterra", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, LOR, MATCH, M_BY_MATCH, "Legends of Runeterra", "Match", "By Match", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 1> Lor_Ranked = {
        QUERY_FORM(1, LOR, RANKED, LEADERBOARD, "Legends of Runeterra", "Ranked", "Leaderboards", {"Routing"}, {_NO_FLAG})
    };
    static std::array<QUERY_FORM, 1> Lor_Status = {
        QUERY_FORM(1, LOR, STATUS, S_STATUS, "Legends of Runeterra", "Status", "Status", {"Routing"}, {_NO_FLAG})
    };

    void RestfulEzreal::NewQueryFormLOR(int end_name, int endpoint_method) {

        try {
            switch (end_name) {
                case MATCH:
                    this->pushNewForm(Lor_Match.at(endpoint_method));break;
                case RANKED:
                    this->pushNewForm(Lor_Ranked.at(endpoint_method));break;
                case STATUS:
                    this->pushNewForm(Lor_Status.at(endpoint_method));break;
            }
        } 
        catch (std::out_of_range const* ex) {
            std::ostringstream ss;
            ss << "Invalid Endpoint and Method arguement. (Endpoint, Method): (" << end_name << ", " << endpoint_method << ")";
            throw std::invalid_argument(ss.str());
        }
    }

    Json::Value RestfulEzreal::Send_LOR() {
        Json::Value response;

        std::vector<PARAM_CONT>& params = this->_next_request->_req_in_form.params;
        const int endpoint = this->_next_request->_req_in_form._endpoint;
        const int endpoint_method = this->_next_request->_req_in_form._endpoint_method;
        
        try {
            switch (endpoint) {
                case MATCH:
                    switch (endpoint_method) {
                        case M_BY_PUUID:
                            response = this->_underlying_client->Lor_Match.by_puuid(params.at(0), params.at(1));break;
                        case M_BY_MATCH:
                            response = this->_underlying_client->Lor_Match.by_match(params.at(0), params.at(1));break;
                        default:
                            throw std::invalid_argument("Invalid Endpoint Method for LOR_MATCH, greater then 1 (strictly)");
                    }break;
                case RANKED:
                    response = this->_underlying_client->Lor_Ranked.leaderboards(params.at(0));break;
                case STATUS:
                    response = this->_underlying_client->Lor_Status.v1(params.at(0));break;
                default:
                    throw std::invalid_argument("Invalid Endpoint given, greater then 2 (strictly)");
            }
        }
        catch (std::out_of_range const& ex) {
            throw std::invalid_argument("Query Form param index out of range given, (too many arguements)");
        }
        return response;
    }
}
