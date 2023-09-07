#include "Walnut/Application.h"
#include <iostream>
#include <thread>
#include <regex>
#include <tuple>
#include <array>
#include <json/json.h>
#include "RestfulEzreal.h"
#include "simdjson.h"

#ifndef CONFIG_FILE_PATH
#define CONFIG_FILE_PATH "./EzrealRunes.json"
#else
#endif

namespace restfulEz {

    static void set_colours() {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.27f, 0.27f, 0.27f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.09f, 0.11f, 0.12f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.20f, 0.26f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.33f, 0.36f, 0.40f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.93f, 0.53f, 0.71f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.38f, 0.40f, 0.42f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(1.00f, 1.00f, 1.00f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.86f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.36f, 0.36f, 0.36f, 0.80f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.13f, 0.13f, 0.13f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.55f, 0.55f, 0.55f, 0.35f);
    }

    RestfulEzreal::RestfulEzreal(current_page* on_display) {
        this->_on_display = on_display;
        this->batch_group = std::make_shared<BatchForm>();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        set_colours();
    };

    void RestfulEzreal::OnUIRender() {
        // Main render function for the user interface
#if DEBUG_MODE
        ImGui::ShowMetricsWindow();
#endif

        static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
        static bool open = true;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        ImGui::Begin("RestfulEzreal", &open, window_flags);

        // We want to check which page is selected on the nav bar and render the selected page
        if (!this->_underlying_client) { this->config_check(); };

        if (*this->_on_display == current_page::HOME) { 
            this->render_home(); 
        } else if (*this->_on_display == current_page::ADVANCED_REQUESTS) { 
            this->render_advanced_requests();
        }

        ImGui::End();

        // In a seperate window render the status of the client.
        this->render_client_status();
    }

    bool RestfulEzreal::validate_key() {
        // check if the api-key given is valid.
        bool valid_key = false;
        bool tested = false;

        static const std::regex reg_stat("status");
        static std::smatch not_used;

        if (!valid_key && !tested) {
            json_ptr response = this->_underlying_client->Lol_Status.v4("OC1");
            std::string raw_rp = response->data();
            if (response->empty()) {
                tested = true;
                valid_key = false;
            }
            else if (!std::regex_search(raw_rp, not_used, reg_stat)) {
                valid_key = true;
                tested = true;
            }
            else {
                static const std::regex code("(401|403)");
                static const std::regex stat_code("status_code");
                if (std::regex_search(raw_rp, not_used, code) || std::regex_search(raw_rp, not_used, stat_code)) {
                    tested = true;
                    valid_key = false;
                }
            }
        }
        if (!valid_key && !tested) {
            json_ptr response = this->_underlying_client->Lol_Status.v4("KR");
            std::string raw_rp = response->data();
            static const std::regex stat("status");
            if (!response->empty() || !std::regex_search(raw_rp, not_used, stat)) {
                valid_key = true;
            }
            else {
                static const std::regex code("(401|403)");
                static const std::regex stat_code("status_code");
                if (std::regex_search(raw_rp, not_used, code) || std::regex_search(raw_rp, not_used, stat_code)) {
                    tested = true;
                    valid_key = false;
                }
            }
            tested = true;
        }

        return valid_key;
    }

    void RestfulEzreal::render_client_status() {

        static bool valid_key = false;

        static bool open = true;
        static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
        ImGui::Begin("Client State", &open, window_flags);

        if (!this->_underlying_client) {
            ImGui::Text("Client not intialised");
            ImGui::End();
            return;
        }

        if (!this->client_tested) {
            valid_key = this->validate_key();
            this->client_tested = true;
        }


        if (valid_key) {
            ImGui::Text("Client Initialised with valid key");
        }
        else {
            ImGui::Text("API-KEY given was not valid or unable to be validated, please check your connection or RIOT developer website");
        }

        ImGui::End();


    };

    void RestfulEzreal::render_home() {

        ImGui::Text("Home");

        int counter = 0;
        int to_remove = -1;
        for (QUERY_FORM& form : this->_current_forms) {
            if (form.check_remove()) {
                to_remove = counter;
            }
            counter += 1;
        }
        if (to_remove != -1) {
            this->_current_forms.erase(this->_current_forms.begin() + to_remove);
        }

        for (QUERY_FORM& form : this->_current_forms) {
            form.render_form();
        }

        this->NewFormButton();
    }

    void RestfulEzreal::pushNewForm(QUERY_FORM new_form) {
        this->_current_id += 1;
        this->_current_forms.push_back(new_form);
        this->_current_forms.back().set_id(this->_next_form_id);
        this->_current_forms.back().set_sender(request_sender);
        this->_next_form_id += 1;
    }

    inline bool file_exists(const std::string& name) {
        if (FILE* file = fopen(name.c_str(), "r")) {
            fclose(file);
            return true;
        }
        else {
            return false;
        }
    }

    static void write_config_file(const char api_key[256], const char path_to_log[64], const char path_to_output[64], const bool verbosity, const logging::LEVEL level_) {
        // format config file as json

        std::ofstream config_file(CONFIG_FILE_PATH, std::ios::trunc);

        config_file << "{\n";
        config_file << "\t\"api-key\" : \"" << api_key << "\",\n";
        config_file << "\t\"log-path\" : \"" << path_to_log << "\",\n";
        config_file << "\t\"log-level\" : " << static_cast<int>(level_) << ",\n";
        config_file << "\t\"output-path\" : \"" << path_to_output << "\",\n";
        config_file << "\t\"verbosity\" : " << (verbosity ? "true" : "false") << "\n";
        config_file << "}";

        config_file.close();
    }

    void RestfulEzreal::config_check() {
        if (file_exists(CONFIG_FILE_PATH)) {
            simdjson::ondemand::parser parser;
            simdjson::padded_string contents = simdjson::padded_string::load(CONFIG_FILE_PATH);
            simdjson::ondemand::document doc = parser.iterate(contents);

            logging::LEVEL report_level = static_cast<logging::LEVEL>(doc["log-level"].get_int64().value());
            this->_path_to_output = std::string(doc["output-path"].get_string().value());

            std::string log_path;
            log_path = doc["log-path"].get_string().value();

            this->_underlying_client = std::make_shared<client::RiotApiClient>(CONFIG_FILE_PATH, log_path, report_level, doc["verbosity"].get_bool().value());
            this->request_sender = std::make_shared<RequestSender>(this->_underlying_client, this->_path_to_output);
            this->batch_group->set_sender(this->request_sender);
            return;
        }
        ImGui::OpenPopup("Configure Client");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Configure Client", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char api_key[256] = "";
            static char path_to_log[64] = "";
            static char path_to_output[64] = "";
            static bool verbosity = false;
            static bool selected[5] = {};

            static bool submit = false;

            static logging::LEVEL level_ = logging::LEVEL::INFO;

            ImGui::InputText("API-KEY", api_key, 256);
            ImGui::InputText("OUTPUT DIRECTORY", path_to_output, 64);
            ImGui::InputText("PATH TO LOGFILE", path_to_log, 64);

            if (ImGui::BeginTable("Logging Level", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {

                ImGui::TableNextColumn();
                if (ImGui::Selectable("DEBUG", level_ == logging::LEVEL::DEBUG)) {
                    level_ = logging::LEVEL::DEBUG;
                }; // FIXME-TABLE: Selection overlap
                ImGui::TableNextColumn();
                if (ImGui::Selectable("INFO", level_ == logging::LEVEL::INFO)) {
                    level_ = logging::LEVEL::INFO;
                }; // FIXME-TABLE: Selection overlap
                ImGui::TableNextColumn();
                if (ImGui::Selectable("WARNING", level_ == logging::LEVEL::WARNING)) {
                    level_ = logging::LEVEL::WARNING;
                }; // FIXME-TABLE: Selection overlap
                ImGui::TableNextColumn();
                if (ImGui::Selectable("ERRORS", level_ == logging::LEVEL::ERRORS)) {
                    level_ = logging::LEVEL::ERRORS;
                }; // FIXME-TABLE: Selection overlap
                ImGui::TableNextColumn();
                if (ImGui::Selectable("CRITICAL", level_ == logging::LEVEL::CRITICAL)) {
                    level_ = logging::LEVEL::CRITICAL;
                }; // FIXME-TABLE: Selection overlap

                ImGui::EndTable();
            }
            ImGui::Checkbox("Verbose Logging", &verbosity);

            if (ImGui::Button("Submit")) { 
                write_config_file(api_key, path_to_log, path_to_output, verbosity, level_);
                this->_underlying_client = std::make_shared<client::RiotApiClient>(CONFIG_FILE_PATH, path_to_log, level_, verbosity);
                this->request_sender = std::make_shared<RequestSender>(this->_underlying_client, this->_path_to_output);
                this->batch_group->set_sender(this->request_sender);
                this->_path_to_output = path_to_output;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void RestfulEzreal::render_advanced_requests() {

        this->batch_group->render_form();

    }
}
