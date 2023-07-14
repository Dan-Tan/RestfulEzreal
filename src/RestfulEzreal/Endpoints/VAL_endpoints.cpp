#include "../RestfulEzreal.h"
#include <array>
#include <sstream>

#define VAL 4

// ###############
#define CONTENT 0

// ###############
#define MATCH 1

#define BY_MATCH 0
#define BY_PUUID 1
#define BY_QUEUE 2

// ###############
#define RANKED 2

// ###############
#define STATUS 3

namespace restfulEz {
    static std::array<QUERY_FORM, 1> Val_Content = {
        QUERY_FORM(1, 4, 0, 1, "Valorant", "Content", "Content", {"Routing"}, {_NO_FLAG}, true, {"locale"}, {""}, {_NO_FLAG})
    };
    static std::array<QUERY_FORM, 3> Val_Match = {
        QUERY_FORM(2, 4, 1, 0, "Valorant", "Match", "By Match", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, 4, 1, 1, "Valorant", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}),
        QUERY_FORM(2, 4, 1, 2, "Valorant", "Match", "By Queue", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG})
    };
    static std::array<QUERY_FORM, 1> Val_Ranked = {
        QUERY_FORM(2, 4, 2, 0, "Valorant", "Ranked", "By Act", {"Routing", "Act ID"}, {_NO_FLAG, _NO_FLAG}, true, {"locale"}, {""}, {_NO_FLAG})
    };
    static std::array<QUERY_FORM, 1> Val_Status = {
        QUERY_FORM(1, 4, 3, 0, "Valorant", "Status", "Status", {"Routing"}, {_NO_FLAG})
    };

    void RestfulEzreal::NewQueryFormVAL(int end_name, int endpoint_method) {
        
        switch (end_name) {
            case CONTENT:
                this->pushNewForm(Val_Content.at(endpoint_method)); break;
            case MATCH:
                this->pushNewForm(Val_Match.at(endpoint_method)); break;
            case RANKED:
                this->pushNewForm(Val_Ranked.at(endpoint_method)); break;
            case STATUS:
                this->pushNewForm(Val_Status.at(endpoint_method)); break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index");
        }
    }

    Json::Value RestfulEzreal::Send_VAL() {
        Json::Value response;

        std::vector<PARAM_CONT>& params = this->_next_request->_req_in_form.params;
        const int endpoint = this->_next_request->_req_in_form._endpoint;
        const int endpoint_method = this->_next_request->_req_in_form._endpoint_method;

        switch (endpoint) {
            case CONTENT:
                response = this->_underlying_client->Val_Content.content(params.at(0));break;
            case MATCH:
                switch (endpoint_method) {
                    case BY_MATCH:
                        response = this->_underlying_client->Val_Match.by_match(params.at(0), params.at(1));break;
                    case BY_PUUID:
                        response = this->_underlying_client->Val_Match.by_puuid(params.at(0), params.at(1));break;
                    case BY_QUEUE:
                        response = this->_underlying_client->Val_Match.by_queue(params.at(0), params.at(1));break;
                    default:
                        throw std::invalid_argument("Invalid Endpoint Method Index Given");
                } break;
            case RANKED:
                response = this->_underlying_client->Val_Ranked.by_act(params.at(0), params.at(1));break;
            case STATUS:
                response = this->_underlying_client->Val_Status.platform_data(params.at(0));break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index Given");
        }
        return response;
    }
}
