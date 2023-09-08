#include "../RequestQueue.h"
#include "../RestfulEzreal.h"
#include <array>
#include <sstream>
#include <utility>

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

#define FORMAT(name, ...) {static const std::string req_name = name; static const std::vector<std::string> p_names = {__VA_ARGS__}; this->update_display_func(req_name, p_names, params);}

namespace restfulEz {

    std::shared_ptr<LinkedInterface> LinkedInterface::make_linked_VAL(const int game, const int end_name, const int endpoint_method) {
        
        switch (end_name) {
            case CONTENT:
                switch (endpoint_method) {
                    case 0:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case MATCH:
                switch (endpoint_method) {
                    case 0:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case 1:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                    case 2:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case RANKED:
                switch (endpoint_method) {
                    case 0:
                        return std::make_shared<LinkedForm<2>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            case STATUS:
                switch (endpoint_method) {
                    case 0:
                        return std::make_shared<LinkedForm<1>>(BaseForm::make_form(game, end_name, endpoint_method)); break;
                } break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index");
        }
        throw std::invalid_argument("Invalid Endpoint Index");
    }

    BaseForm BaseForm::make_form_VAL(const int end_name, const int endpoint_method) {
        
        switch (end_name) {
            case CONTENT:
                switch (endpoint_method) {
                    case 1:
                        return BaseForm(1, 4, 0, 1, "Valorant", "Content", "Content", {"Routing"}, {_NO_FLAG}, true, {"locale"}, {""}, {_NO_FLAG}); break;
                } break;
            case MATCH:
                switch (endpoint_method) {
                    case 0:
                        return BaseForm(2, 4, 1, 0, "Valorant", "Match", "By Match", {"Routing", "Match ID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case 1:
                        return BaseForm(2, 4, 1, 1, "Valorant", "Match", "By Puuid", {"Routing", "PUUID"}, {_NO_FLAG, _NO_FLAG}); break;
                    case 2:
                        return BaseForm(2, 4, 1, 2, "Valorant", "Match", "By Queue", {"Routing", "Queue"}, {_NO_FLAG, _NO_FLAG}); break;
                } break;
            case RANKED:
                switch (endpoint_method) {
                    case 0:
                        return BaseForm(2, 4, 2, 0, "Valorant", "Ranked", "By Act", {"Routing", "Act ID"}, {_NO_FLAG, _NO_FLAG}, true, {"locale"}, {""}, {_NO_FLAG}); break;
                } break;
            case STATUS:
                switch (endpoint_method) {
                    case 0:
                        return BaseForm(1, 4, 3, 0, "Valorant", "Status", "Status", {"Routing"}, {_NO_FLAG}); break;
                } break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index");
        }
        throw std::invalid_argument("Invalid Endpoint Index");
    }

    using json_ptr = std::unique_ptr<std::vector<char>>;

    json_ptr RequestSender::Send_VAL(request& task) {
        json_ptr response;

        std::vector<PARAM_CONT>& params = task.params;
        std::vector<P_NAME>& opt_names = task.optional_names;
        std::vector<PARAM_CONT>& opt_inputs = task.optional_inputs;
        const int endpoint = task._endpoint;
        const int endpoint_method = task._endpoint_method;

        switch (endpoint) {
            case CONTENT:
                FORMAT("Valorant | Content", "Routing")
                response = this->underlying_client->Val_Content.content(params.at(0), std::pair<std::string, std::string>(opt_names.at(0), opt_inputs.at(0)));break;
            case MATCH:
                switch (endpoint_method) {
                    case BY_MATCH:
                        FORMAT("Valorant | Match | By Match ID", "Routing", "Match ID")
                        response = this->underlying_client->Val_Match.by_match(params.at(0), params.at(1));break;
                    case BY_PUUID:
                        FORMAT("Valorant | Match | By Puuid", "Routing", "PUUID")
                        response = this->underlying_client->Val_Match.by_puuid(params.at(0), params.at(1));break;
                    case BY_QUEUE:
                        FORMAT("Valorant | Match | By Match ID", "Routing", "Match ID")
                        response = this->underlying_client->Val_Match.by_queue(params.at(0), params.at(1));break;
                    default:
                        throw std::invalid_argument("Invalid Endpoint Method Index Given");
                } break;
            case RANKED:
                FORMAT("Valorant | Ranked | By Act", "Routing", "Act")
                response = this->underlying_client->Val_Ranked.by_act(params.at(0), params.at(1), std::pair<std::string, std::string>(opt_names.at(0), opt_inputs.at(0)));break;
            case STATUS:
                FORMAT("Valorant | Status | v1", "Routing")
                response = this->underlying_client->Val_Status.platform_data(params.at(0));break;
            default:
                throw std::invalid_argument("Invalid Endpoint Index Given");
        }
        return std::move(response);
    }
}
