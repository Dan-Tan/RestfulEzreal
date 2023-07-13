#pragma once
#include "client/client.h"
#include "json/json.h"
#include "Walnut/Application.h"
#include <fstream>
#include <vector>
#include <array>
#include <string>

#define _NO_FLAG ImGuiInputTextFlags_None
#define _DEC_FLAG ImGuiInputTextFlags_CharsDecimal


enum current_page {
    HOME,
    GUIDE,
    ADVANCED_REQUESTS,
    CLIENT_CONFIG,
    REQUEST_HISTORY,
    REQUEST_STATUS
};

namespace restfulEz {

    typedef struct P_NAME {
        char name[32] = { 0 };
        operator char* () { return name; }
        P_NAME() {};
        P_NAME(const char* str) {
            strncpy(name, str, sizeof(str));
        };
    }P_NAME;

    typedef struct PARAM_CONT {
        char param[256] = {0};
        operator char*() { return param; };
        operator std::string() { return std::string(param); };
        PARAM_CONT() {};
        PARAM_CONT(const char* str) {
            strncpy(param, str, sizeof(str));
        };
    }PARAM_CONT;

    typedef struct request {
        int _game;
        int _endpoint;
        int _endpoint_method;

        std::vector<PARAM_CONT> params;
    }request;


    class QUERY_FORM {
        private:
            std::string _game_name;
            std::string _endpoint;
            std::string _endpoint_method;

            std::vector<P_NAME> _param_names;
            std::vector<PARAM_CONT> _params_in_form = {};
            std::vector<ImGuiInputTextFlags> _type_ordering;

            std::string _ID; // Distinguish Form from other Query Forms

            std::size_t _n_params;
            QUERY_FORM** _client_req;
            bool* _execute;
            bool form_execute = false;

            bool remove_form = false;


        public:
            QUERY_FORM(const std::size_t n_params, const int game_ind, const int endpoint_ind, const int endpoint_method_ind, const char* game_name, 
                    const char* endpoint, const char* endpoint_method, 
                    std::vector<P_NAME> param_names, std::vector<ImGuiInputTextFlags> type_ordering) 
                : _game_name(game_name), _endpoint(endpoint), _endpoint_method(endpoint_method)
            {
                this->_n_params = n_params;
                this->_param_names = param_names;

                this->_type_ordering = type_ordering;
                this->_params_in_form.resize(n_params);

                this->_req_in_form = { game_ind, endpoint_ind, endpoint_method_ind };
            };
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
    };

    class RestfulEzreal : public Walnut::Layer
    {
        private:
            std::shared_ptr<client::RiotApiClient> _underlying_client = nullptr;
            current_page* _on_display;
            std::string _path_to_output;

            std::vector<QUERY_FORM> _current_forms = {};
            bool send_next_request = false;
            bool client_tested = false;

        public:
            RestfulEzreal(current_page* on_display) {
                this->_on_display = on_display;
            };

            void OnUIRender() override;
            QUERY_FORM* _next_request = nullptr;

        protected:
            void Send_Request(); // will send the request currently in _next_request;

        private:

            void NewFormButton();
            bool validate_key();

            Json::Value Send_LOL();
            Json::Value Send_VAL();
            Json::Value Send_TFT();
            Json::Value Send_LOR();

            int _current_id = 1;

            void NewQueryForm(int gameName, int end_name, int endpoint_method); // purely for readability and maintenance
            void NewQueryFormLOL(int end_name, int endpoint_method);
            void NewQueryFormTFT(int end_name, int endpoint_method); 
            void NewQueryFormVAL(int end_name, int endpoint_method);
            void NewQueryFormLOR(int end_name, int endpoint_method);

            void inline pushNewForm(QUERY_FORM new_form) {
                new_form.set_execute(&this->send_next_request, &(this->_next_request), this->_current_id);
                this->_current_id += 1;
                this->_current_forms.push_back(new_form);
            }

            void set_output_directory() {};
            void set_debug_level() {};
            void set_verbosity() {};

            void render_home();
            void render_client_status();
            void render_guide() {};
            void render_advanced_requests() {};
            void render_client_config() {};
            void render_json() {};
            void render_rate_status() {};

            void config_check();
    };
}
