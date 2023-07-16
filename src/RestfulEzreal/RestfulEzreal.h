#pragma once
#include "client/client.h"
#include "json/json.h"
#include "Walnut/Application.h"
#include "QueryForm.h"
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
