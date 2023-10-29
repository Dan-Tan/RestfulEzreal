#pragma once
#include "client/client.h"
#include "json/json.h"
#include "RequestQueue.h"
#include "QueryForm.h"
#include "simdjson.h"
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <vulkan/vulkan_core.h>
#include <imgui.h>

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

    class RestfulEzreal
    {
        private:
            std::shared_ptr<client::RiotApiClient> _underlying_client = nullptr;
            current_page* _on_display;
            std::string _path_to_output;

            std::vector<QUERY_FORM> _current_forms = {};
            std::shared_ptr<RequestSender> request_sender = std::make_shared<RequestSender>(); // default init create thread
            bool send_next_request = false;
            bool client_tested = false;

            std::shared_ptr<BatchForm> batch_group;

            int _next_form_id = 1;

            std::string custom_config_path;

        public:
            RestfulEzreal(current_page* on_display);
            ~RestfulEzreal() {
                this->request_sender->stop();
            }

            void OnUIRender();
            int render_welcome();
            int render_welcome_config();
            QUERY_FORM* _next_request = nullptr;

        private:

            void NewFormButton();

            int _current_id = 1;

            void NewQueryForm(int gameName, int end_name, int endpoint_method); // purely for readability and maintenance

            void pushNewForm(QUERY_FORM new_form);

            void set_debug_level() {};
            void set_verbosity() {};

            void render_home();
            void render_client_status();
            void render_guide() {};
            void render_advanced_requests();
            void render_client_config() {};
            void render_json() {};
            void render_rate_status() {};
            
            void configure_from_existing(bool custom_path = false, const std::string& file_path = std::string(""));
            bool configure_new_client(bool allow_close = false);
            template<bool in_popup>
            bool conf_form();
            void reconfigure_client();
            void config_check();

            bool start_up_from_existing(int* ret);
            void instantiate_client(int* int_out, int* parse_out, const std::string& file_path);
            int display_configuration_process(std::unique_ptr<std::string> file_cont, const float x_align);
            bool start_up_default(int* ret);
    };
}
