#pragma once
#include "client/client.h"
#include "json/json.h"
#include "Walnut/Application.h"
#include "RequestQueue.h"
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

    class FormGroup;

    class RestfulEzreal : public Walnut::Layer
    {
        friend class FormGroup;
        private:
            std::shared_ptr<client::RiotApiClient> _underlying_client = nullptr;
            current_page* _on_display;
            std::string _path_to_output;

            std::vector<QUERY_FORM> _current_forms = {};
            std::shared_ptr<RequestSender> request_sender;
            bool send_next_request = false;
            bool client_tested = false;

            std::shared_ptr<FormGroup> batch_group = nullptr;

            int _next_form_id = 1;

        public:
            RestfulEzreal(current_page* on_display) {
                this->_on_display = on_display;
                this->batch_group = std::make_shared<FormGroup>();
            };

            void OnUIRender() override;
            QUERY_FORM* _next_request = nullptr;

        private:

            void NewFormButton();
            bool validate_key();

            int _current_id = 1;

            void NewQueryForm(int gameName, int end_name, int endpoint_method); // purely for readability and maintenance
            void NewQueryFormLOL(int end_name, int endpoint_method);
            void NewQueryFormTFT(int end_name, int endpoint_method); 
            void NewQueryFormVAL(int end_name, int endpoint_method);
            void NewQueryFormLOR(int end_name, int endpoint_method);

            void pushNewForm(QUERY_FORM new_form);

            void set_output_directory() {};
            void set_debug_level() {};
            void set_verbosity() {};

            void render_home();
            void render_client_status();
            void render_guide() {};
            void render_advanced_requests();
            void render_client_config() {};
            void render_json() {};
            void render_rate_status() {};

            void config_check();
    };

    class FormGroup {

        private:
            std::vector<LinkedForm> forms;

        public:
            FormGroup() = default;
            ~FormGroup() = default;

            void render_group(RestfulEzreal& owner);
            void add_form(QUERY_FORM& form) {
                forms.push_back({form}); // trigger constructor
            }
    };
}
