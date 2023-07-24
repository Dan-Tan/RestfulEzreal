#pragma once
#include "Walnut/Application.h"
#include "BatchRequests.h"
#include <vector>
#include <string>
#include <cstring>

namespace restfulEz {


    class QUERY_FORM {
        private:
            std::string _game_name;
            std::string _endpoint;
            std::string _endpoint_method;

            std::vector<P_NAME> _param_names;
            std::vector<PARAM_CONT> _params_in_form = {};
            std::vector<ImGuiInputTextFlags> _type_ordering;

            bool _accepts_optional;
            std::vector<P_NAME> _optional_names;
            std::vector<PARAM_CONT> _optional_inputs;
            std::vector<ImGuiInputTextFlags> _optional_types;
            std::vector<int> _optionals_to_send; 

            int _n_used_optional_p1 = 0;

            std::string _ID; // Distinguish Form from other Query Forms

            std::size_t _n_params;
            QUERY_FORM** _client_req;

            bool* _execute; // pointer to client telling object to send request, not a good idea but fix later
            bool form_execute = false;
            bool remove_form = false;


        public:
            QUERY_FORM(const std::size_t n_params, const int game_ind, const int endpoint_ind, 
                    const int endpoint_method_ind, const char* game_name, 
                    const char* endpoint, const char* endpoint_method, 
                    std::vector<P_NAME> param_names, std::vector<ImGuiInputTextFlags> type_ordering,
                    bool accepts_optional = false, std::vector<P_NAME> optional_names = {}, 
                    std::vector<PARAM_CONT> optional_inputs = {}, std::vector<ImGuiInputTextFlags> optional_types = {});

            ~QUERY_FORM() = default;

            request _req_in_form;
            std::string request_result = "";

            void render_form();

            void set_execute(bool* execute, QUERY_FORM** client_req, int ID) { 
                this->_execute = execute; 
                this->_client_req = client_req;
                this->_ID = std::string("##form") + std::to_string(ID);
            };
            bool check_remove() { return this->remove_form; };

        private:
            void render_optionals(bool already_sent);
            void render_input_fields();
    };

}
