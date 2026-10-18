#include <iostream>
#include <cstring>
#include <bits/stdc++.h>
#include <thread>
#include <nfd.h> // file_dialog
#include <regex>
#include <tuple>
#include <array>
#include <json/json.h>
#include <string_view>
#include <tuple>
#include "RestfulEzreal.h"
#include "FileManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "simdjson.h"
#include "utils/utils.h"

#define ROOT_DIR ../fonts

#ifndef CONFIG_FILE_PATH
#define CONFIG_FILE_PATH "./EzrealRunes.json"
#else
#endif

namespace restfulEz {

    static void set_colours() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameBorderSize = 1.0f;
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.TabBorderSize = 1.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);
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
        colors[ImGuiCol_CheckMark]              = ImVec4(0.92f, 0.68f, 0.01f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.92f, 0.68f, 0.01f, 0.80f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.92f, 0.68f, 0.01f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.92f, 0.68f, 0.01f, 0.35f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.92f, 0.68f, 0.01f, 0.65f);
        colors[ImGuiCol_Header]                 = ImVec4(0.92f, 0.68f, 0.01f, 0.35f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.92f, 0.68f, 0.01f, 0.65f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.92f, 0.68f, 0.01f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.92f, 0.68f, 0.01f, 0.35f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.92f, 0.68f, 0.01f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.92f, 0.68f, 0.01f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.92f, 0.68f, 0.01f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.92f, 0.68f, 0.01f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.92f, 0.68f, 0.01f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.86f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.92f, 0.68f, 0.01f, 0.60f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.92f, 0.68f, 0.01f, 0.80f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.13f, 0.13f, 0.13f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.92f, 0.68f, 0.01f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.92f, 0.68f, 0.01f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.96f, 0.76f, 0.15f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.92f, 0.68f, 0.01f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.92f, 0.68f, 0.01f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.92f, 0.68f, 0.01f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.55f, 0.55f, 0.55f, 0.35f);
    }

    RestfulEzreal::RestfulEzreal(current_page* on_display) {
        this->_on_display = on_display;
        this->_client_state_collapsed = true;
        this->batch_group = std::make_shared<BatchForm>();
        this->batch_group->set_sender(this->request_sender);
        set_colours();
    };


    static std::string config_file_dialog() {
        NFD_Init();
        
        std::string to_ret;
        nfdchar_t *outPath;
        nfdresult_t result = NFD_OpenDialog(&outPath, NULL, 0, NULL);
        if (result == NFD_OKAY) {
            to_ret = std::string(outPath);
            NFD_FreePathN(outPath);
        }
        else if (result == NFD_CANCEL)
        {
            to_ret = std::string("CANCEL");
        }
        else 
        {
            std::cerr << "ERROR WITH FILE DIALOG" << std::endl;
        }
        
        NFD_Quit();
        return to_ret;
    }

    void RestfulEzreal::SkipWelcomeAndInit(const std::string& workspace_dir) {
        this->show_welcome = false;
        this->welcome_state = 0;

        auto& fm = FileManager::GetInstance();
        if (!fm.LoadEnvKey(".env") || fm.GetApiKey().empty()) {
            fm.SetApiKey("RGAPI-00000000-0000-0000-0000-000000000000");
        }
        fm.SetWorkspace(workspace_dir);
        this->_path_to_output = fm.GetConfig().active_workspace_path;

        std::string log_file = (fm.GetWorkspacePaths().root_dir / "logs" / "ui.log").string();
        this->_underlying_client = std::make_shared<client::RiotApiClient>(
            fm.GetApiKey(), log_file, spdlog::level::info, false, true
        );
        this->request_sender->set_client(this->_underlying_client);
        this->request_sender->set_output_directory(this->_path_to_output);
        if (this->batch_group) {
            this->batch_group->set_sender(this->request_sender);
        }
        DatabaseManager::GetInstance().Init(fm.GetWorkspacePaths().db_file.string());
    }

    void RestfulEzreal::OnUIRender() {
        // Main render function for the user interface
        if (this->show_welcome) {
            switch (this->welcome_state) {
                case -1:
                    this->welcome_state = this->render_welcome(); break;
                case 1:
                    this->show_welcome = this->start_up_default(&this->welcome_state); break;
                case 2:
                    this->show_welcome = !this->start_up_from_existing(&this->welcome_state);
                    break;
                case 3:
                    this->welcome_state = this->render_welcome_config(); break;
                case 4:
                    break;
                case 5:
                    break;
                default:
                    std::cerr << "Invalid welcome page index" << std::endl;
            }
        } else {

        static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration 
                                             | ImGuiWindowFlags_NoMove 
                                             | ImGuiWindowFlags_NoResize 
                                             | ImGuiWindowFlags_NoTitleBar 
                                             | ImGuiWindowFlags_NoBringToFrontOnFocus;
        static ImGuiIO& io = ImGui::GetIO();
        const float sidebar_width = std::max(180.0f, io.DisplaySize.x * 0.14f);
        const float right_margin = this->_client_state_collapsed ? 0.0f : 300.0f;
        ImGui::SetNextWindowPos(ImVec2(sidebar_width, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - sidebar_width - right_margin, io.DisplaySize.y));

        ImGui::Begin("RestfulEzreal", NULL, window_flags);

        // We want to check which page is selected on the nav bar and render the selected page
        if (!this->_underlying_client) { this->config_check(); };

        switch (*this->_on_display) {
            case current_page::HOME:
                this->render_home();
                break;
            case current_page::GUIDE:
                this->render_guide();
                break;
            case current_page::ADVANCED_REQUESTS:
                this->render_advanced_requests();
                break;
            case current_page::CLIENT_CONFIG:
                this->render_client_config();
                break;
            case current_page::REQUEST_HISTORY:
                this->render_json();
                break;
            case current_page::REQUEST_STATUS:
                this->render_rate_status();
                break;
        }

        ImGui::End();

        // In a seperate window render the status of the client.
        this->render_client_status();
        }
    }

    static bool render_button_text(bool* hovered, const char* txt, const char* ID) {
        bool pressed = false;
        bool pop = false;
        static const ImVec4 active_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        if (*hovered) {
            pop = true;
            ImGui::PushStyleColor(ImGuiCol_Text, active_col);
        }
        ImGui::Text(txt);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::SameLine();
        ImGui::Text(" ~ ");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        pressed = ImGui::Button(ID);
        if (pop) {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) {
            *hovered = true;
        } else {
            *hovered = false;
        }
        return pressed;
    }

    int RestfulEzreal::render_welcome() {


        static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
        static ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("Welcome", NULL, window_flags);
        
        // DISPLAY TITLE RESTFULEZREAL
        ImGui::PushFont(io.Fonts->Fonts[1]);
        const ImVec2 sze = ImGui::CalcTextSize("RESTfulEzreal;");
        const ImVec2 half_sze = ImVec2(sze.x * 0.5, sze.y * 0.5);

        ImVec2 title_pos = ImVec2(io.DisplaySize.x * 0.5 - half_sze.x, io.DisplaySize.y * 0.25 - half_sze.y); 
        ImGui::SetCursorPos(title_pos);
        static re_utils::FBF_STRING_16 title = re_utils::create_fbf_string<16, re_utils::FBF_STRING_16>("RESTfulEzreal;", 2);
        re_utils::fbf_text(title);
        //ImGui::Text("RESTfulEzreal;");

        ImGui::PopFont();
        
        // DISPLAY SUBTITLE
        ImGui::PushFont(io.Fonts->Fonts[2]);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
        ImGui::SetCursorPos(ImVec2(title_pos.x, title_pos.y + 2 *  half_sze.y));
        ImGui::Text("A RESTFUL API CLIENT WITH A GRAPHICAL USER INTEFACE");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        
        int start_up_type = -1;
        
        ImGui::PushFont(io.Fonts->Fonts[0]);
        static bool hovered_button[5] = { false , false , false , false , false};
        static const float txt_size[7] = {ImGui::CalcTextSize("CONFIGURE    from existing").x, ImGui::CalcTextSize("from existing").x, ImGui::CalcTextSize("from custom").x, ImGui::CalcTextSize("new").x, ImGui::CalcTextSize("ABOUT").x, ImGui::CalcTextSize("SOURCE").x, ImGui::CalcTextSize(" ").x} ;
        const float last_ali_col = title_pos.x + ImGui::CalcTextSize("Configure    from existing ").x;

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
        static float lne_height = ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[0], title_pos.y + 2*  sze.y + 1.5 * lne_height));
        if (hovered_button[0] || hovered_button[1] || hovered_button[2] ) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        ImGui::Text("CONFIGURE ");
        ImGui::SameLine();
        if (hovered_button[0] || hovered_button[1] || hovered_button[2] ) {
            ImGui::PopStyleColor();
        }

        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[1], title_pos.y + 2*  sze.y + 1.5 * lne_height));
        if (render_button_text(&hovered_button[0], " from existing", ">##1")) { start_up_type = 1; };

        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[1], title_pos.y + 2*  sze.y + 3 * lne_height));
        if (render_button_text(&hovered_button[1], " from custom  ", ">##3")) { 
            start_up_type = 2; 
            this->custom_config_path = config_file_dialog();
        };

        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[1], title_pos.y + 2*  sze.y + 4.5 * lne_height));
        if (render_button_text(&hovered_button[2], " new          ", ">##4")) { start_up_type = 3; };

        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[0], title_pos.y + 2*  sze.y + 9 * lne_height));
        if (hovered_button[3]) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); }
        ImGui::Text("ABOUT");
        ImGui::SameLine();
        if (hovered_button[3]) { ImGui::PopStyleColor(); }

        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[6], title_pos.y + 2*  sze.y + 9 * lne_height));
        if (render_button_text(&hovered_button[3], "  ", ">##5")) { start_up_type = 4; };

        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[0], title_pos.y + 2*  sze.y + 7.5 * lne_height));
        if (hovered_button[4]) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); }
        ImGui::Text("SOURCE");
        ImGui::SameLine();
        if (hovered_button[4]) { ImGui::PopStyleColor(); }

        ImGui::SetCursorPos(ImVec2(last_ali_col - txt_size[6], title_pos.y + 2*  sze.y + 7.5 * lne_height));
        if (render_button_text(&hovered_button[4], "  ", ">##6")) { start_up_type = 5; };

        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::End();

        return start_up_type;
    }

    int RestfulEzreal::render_welcome_config() {
        static ImGuiIO& io = ImGui::GetIO();
        re_utils::full_display();
        static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
        int to_return = 3;
        ImGui::Begin("Welcome Config", NULL, window_flags);
        if (this->conf_form<false>()) {
            this->custom_config_path = CONFIG_FILE_PATH;
            to_return = 2; // client status dynamic
        };
        ImGui::SameLine();
        ImGui::PushFont(io.Fonts->Fonts[1]);
        ImGui::SetCursorPosX(io.DisplaySize.x * 0.5 - ImGui::CalcTextSize("RestfulEzreal;").x * 0.5);
        ImGui::PopFont();
        if (ImGui::Button("Cancel")) {
            to_return = -1; // back to start
        };
        ImGui::End();

        return to_return;
    }

    static void render_recent_request(const std::string& req_name, const std::vector<std::string>& param_descriptions) {
        ImGui::TextUnformatted("Most Recent Request");
        ImGui::TextUnformatted(req_name.c_str());
        for (const std::string& param_desc : param_descriptions) {
            ImGui::TextUnformatted(param_desc.c_str());
        }
    }


    void RestfulEzreal::render_client_status() {

        static bool open = true;
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 12.0f, 45.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(this->_client_state_collapsed ? 200.0f : 240.0f, 0.0f),
            ImVec2(this->_client_state_collapsed ? 245.0f : 280.0f, 450.0f)
        );
        static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Client State", &open, window_flags);
        
        if (!this->_underlying_client) {
            ImGui::Text("Client not initialised");
            ImGui::End();
            return;
        }

        if (this->_client_state_collapsed) {
            bool api_ok = this->_underlying_client != nullptr;
            bool db_ok = DatabaseManager::GetInstance().IsInitialized();
            ImGui::TextColored(api_ok ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) : ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
                               "[API: %s | DB: %s]", api_ok ? "Active" : "Inactive", db_ok ? "Connected" : "Off");
            ImGui::SameLine();
            if (ImGui::SmallButton("[+]##ExpandClientState")) {
                this->_client_state_collapsed = false;
            }
            ImGui::End();
            return;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Client State");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::SmallButton("[-]##CollapseClientState")) {
            this->_client_state_collapsed = true;
        }
        ImGui::Separator();

        static bool show_key_modal = false;
        static char quick_key[256] = "";

        if (ImGui::Button("Update API Key##QuickKeyBtn")) {
            show_key_modal = true;
            quick_key[0] = '\0';
        }
        ImGui::SameLine();
        static bool reconfiguring = false;
        if (ImGui::Button("Settings / Workspace")) {
            reconfiguring = true;
        }

        if (show_key_modal) {
            ImGui::OpenPopup("Update Riot API Key");
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("Update Riot API Key", &show_key_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Enter new 24h Riot Developer API Key:");
                ImGui::InputText("##NewKeyInput", quick_key, sizeof(quick_key), ImGuiInputTextFlags_Password);
                if (ImGui::Button("Save##QuickKeySave")) {
                    if (quick_key[0] != '\0') {
                        FileManager::GetInstance().SetApiKey(quick_key);
                        FileManager::GetInstance().SaveConfig(CONFIG_FILE_PATH);
                        if (this->_underlying_client) {
                            this->_underlying_client->update_api_key(quick_key);
                        }
                    }
                    show_key_modal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel##QuickKeyCancel")) {
                    show_key_modal = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (reconfiguring) {
            if (this->configure_new_client(true)) {
                reconfiguring = false;
            }
        }

        std::string current_key = FileManager::GetInstance().GetApiKey();
        if (!current_key.empty()) {
            std::string masked = "RGAPI-**** (" + std::to_string(current_key.length()) + " chars)";
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "API Key: %s", masked.c_str());
        }

        std::string ws_path = FileManager::GetInstance().GetWorkspacePaths().root_dir.string();
        ImGui::TextWrapped("Workspace: %s", ws_path.c_str());

        auto [recent_req_name, recent_req_params] = this->request_sender->get_recent_request_info();
        render_recent_request(recent_req_name, recent_req_params);


        ImGui::Separator();
        ImGui::Text("Database (DuckDB):");
        if (DatabaseManager::GetInstance().IsInitialized()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "  Status: Connected");
            ImGui::Text("  Players in DB: %ld", static_cast<long>(DatabaseManager::GetInstance().GetPlayerCount()));
            ImGui::Text("  Matches in DB: %ld", static_cast<long>(DatabaseManager::GetInstance().GetMatchCount()));
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "  Status: Not Initialized");
        }

        ImGui::End();
    };

    void RestfulEzreal::render_home() {

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Single API Requests");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| Interactive ad-hoc query forms for Riot Games APIs");
        ImGui::Separator();
        ImGui::Spacing();

        this->NewFormButton();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

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

        if (this->_current_forms.empty()) {
            ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 80.0f));
            ImGui::BeginChild("HomeQuickStartPanel", ImVec2(0, 220), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
            ImGui::TextUnformatted("Quick-Start Guide");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped("Select a Game, Endpoint category, and Endpoint Method above, then click 'Add' to instantiate a query card.");
            ImGui::Spacing();
            ImGui::BulletText("League of Legends: Account-v1 (By Riot ID / By PUUID), Match-v5 (Match List / Details / Timeline), Champion Mastery.");
            ImGui::BulletText("Teamfight Tactics: Match-v1 (By Match ID / By PUUID), League-v1.");
            ImGui::BulletText("Valorant & Legends of Runeterra: Content-v1, Match-v1, Ranked-v1.");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.58f, 0.58f, 0.58f, 1.0f), "Tip: For multi-stage linked pipelines with automatic JSON parameter extraction and DuckDB ingestion, switch to the 'Advanced Requests' page.");
            ImGui::EndChild();
        } else {
            for (QUERY_FORM& form : this->_current_forms) {
                form.render_form();
            }
        }
    }

    void RestfulEzreal::pushNewForm(QUERY_FORM new_form) {
        this->_current_id += 1;
        this->_current_forms.push_back(new_form);
        this->_current_forms.back().set_id(this->_next_form_id);
        this->_current_forms.back().set_sender(request_sender);
        this->_next_form_id += 1;
    }

    static int search_and_display(const std::string& path, const float& x_begin, const float& x_end) {
        static ImGuiIO& io = ImGui::GetIO();
        static bool first_pass = true;
        static bool valid_path = false;
        
        static std::string file_content;

        if (first_pass) { 
            std::ifstream config_file;
            config_file.open((path.c_str())); 
            if (config_file.is_open()) {
                config_file >> file_content;
                valid_path = true;
            }
        }
        
        ImGui::SetCursorPosX(x_begin);
        ImGui::PushFont(io.Fonts->Fonts[0]);
        if (valid_path) {
            ImGui::Text(file_content.data());
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("ERROR!");
            
            ImGui::SetCursorPosX(x_begin);
            ImGui::Text("INVALID FILE PATH!");
            ImGui::PopStyleColor();
        }
        ImGui::PopFont();

        bool use_file = false;
        

        return use_file;
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

    static void write_config_file(const char api_key[256], const char path_to_log[64], const char path_to_output[64], const bool verbosity, const spdlog::level::level_enum level_) {
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

    void RestfulEzreal::configure_from_existing(bool custom_path, const std::string& file_path) {
        std::string target_config = custom_path ? file_path : CONFIG_FILE_PATH;
        FileManager::GetInstance().LoadConfig(target_config);
        const auto& cfg = FileManager::GetInstance().GetConfig();

        this->_path_to_output = cfg.active_workspace_path;
        this->_underlying_client = std::make_shared<client::RiotApiClient>(cfg.api_key, cfg.log_path, cfg.log_level, cfg.verbosity, true);
        this->request_sender->set_client(this->_underlying_client);
        this->request_sender->set_output_directory(this->_path_to_output);
        this->batch_group->set_sender(this->request_sender);

        DatabaseManager::GetInstance().Init(FileManager::GetInstance().GetWorkspacePaths().db_file.string());
    }
    
    template<bool in_popup>
    static void render_field(const std::size_t write_size, char* to_write, const char* title, const char* ID, const ImGuiInputFlags input_flag, bool* active, const float x_align) {
        /*
         * write_size - size of text buffer for ImGui
         * to_write   - char to write to
         * title      - title of field
         * ID         - FORM ID
         * active     - pointer to bool whether to change col
         * x_align    - float to align all fields to
         */

        static ImGuiIO& io = ImGui::GetIO();

        ImGui::PushFont(io.Fonts->Fonts[0]);
        if constexpr (!in_popup) { ImGui::SetCursorPosX(x_align); }
        if (*active) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); }
        ImGui::Text(title);
        ImGui::PopFont();

        ImGui::PushFont(io.Fonts->Fonts[3]);
        if constexpr (!in_popup) { 
            ImGui::SetCursorPosX(x_align); 
            static bool frst = true;
            static float txt_width;
            if (frst) {
                ImGui::PushFont(io.Fonts->Fonts[1]);
                txt_width = ImGui::CalcTextSize("RESTfulEzreal;").x;
                ImGui::PopFont();
            }
            ImGui::SetNextItemWidth(txt_width);
        }
        ImGui::InputText(ID, to_write, write_size, input_flag);
        ImGui::PopFont();
        if (*active) { ImGui::PopStyleColor(); }
        if (ImGui::IsItemActive()) {
            *active = true;
        } else {
            *active = false;
        }
    }
    
    template<bool in_popup>
    bool RestfulEzreal::conf_form() {
        static char api_key[256] = "";
        static char path_to_log[64] = "";
        static char path_to_output[256] = "";
        static bool verbosity = false;
        static bool init_fields = true;

        if (init_fields && FileManager::GetInstance().IsLoaded()) {
            const auto& cfg = FileManager::GetInstance().GetConfig();
            strncpy(api_key, cfg.api_key.c_str(), sizeof(api_key) - 1);
            strncpy(path_to_log, cfg.log_path.c_str(), sizeof(path_to_log) - 1);
            strncpy(path_to_output, cfg.active_workspace_path.c_str(), sizeof(path_to_output) - 1);
            verbosity = cfg.verbosity;
            init_fields = false;
        }

        static bool submit = false;
        static ImGuiIO& io = ImGui::GetIO();
        
        // computing align value
        float x_ali;
        float y_pos;
        static ImVec2 sze;
        static float lne_height;
        if constexpr (!in_popup) {
            static bool frst = true;
            if (frst) {
                ImGui::PushFont(io.Fonts->Fonts[1]);
                sze = ImGui::CalcTextSize("RESTfulEzreal;");
                ImGui::PopFont();
                ImGui::PushFont(io.Fonts->Fonts[0]);
                lne_height = ImGui::GetTextLineHeightWithSpacing();
                ImGui::PopFont();
                frst = false;
            }
            ImVec2 title_pos = ImVec2(io.DisplaySize.x * 0.3 - 0.5 * sze.x, io.DisplaySize.y * 0.25 - 0.5 * sze.y); 
            y_pos = title_pos.y + 2 * sze.y + 0.5 * lne_height;
            x_ali = io.DisplaySize.x * 0.5 - 0.5 * sze.x;
            ImGui::SetCursorPosY(y_pos);
        } else {
            x_ali = 1.0f;
        }
        
        static bool highlight[3] = { false, false, false };
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
        render_field<in_popup>(256, api_key,        "API-KEY:",        "##API-KEY", ImGuiInputTextFlags_None, &highlight[0], x_ali);
        render_field<in_popup>(64,  path_to_log,    "LOG PATH:",       "##LogPath", ImGuiInputTextFlags_None, &highlight[1], x_ali);
        render_field<in_popup>(256, path_to_output, "WORKSPACE PATH:", "##OutPath", ImGuiInputTextFlags_None, &highlight[2], x_ali);
        ImGui::PopStyleColor();

        // Workspace browse button
        if constexpr (!in_popup) { ImGui::SetCursorPosX(x_ali); }
        if (ImGui::Button("Browse Workspace Folder...##WorkspaceBrowse")) {
            std::string selected_dir = FileManager::SelectFolderDialog();
            if (!selected_dir.empty()) {
                strncpy(path_to_output, selected_dir.c_str(), sizeof(path_to_output) - 1);
            }
        }

        static spdlog::level::level_enum level_ = spdlog::level::info;
        
        ImGui::PushFont(io.Fonts->Fonts[0]);
        if constexpr (!in_popup) {ImGui::SetCursorPosX(x_ali);}
        float context_width = 0.0f;
        if constexpr (!in_popup) { 
            context_width = sze.x;
        };
        if (ImGui::BeginTable("Logging Level", 5, ImGuiTableFlags_NoSavedSettings, ImVec2(context_width, 0.0f), context_width)) {

            ImGui::TableNextColumn();
            if (ImGui::Selectable("DEBUG",    level_ == spdlog::level::debug)) {
                level_ = spdlog::level::debug;
            };

            ImGui::TableNextColumn();
            if (ImGui::Selectable("INFO",     level_ == spdlog::level::info)) {
                level_ = spdlog::level::info;
            };

            ImGui::TableNextColumn();
            if (ImGui::Selectable("WARNING",  level_ == spdlog::level::warn)) {
                level_ = spdlog::level::warn;
            }; 

            ImGui::TableNextColumn();
            if (ImGui::Selectable("ERRORS",   level_ == spdlog::level::err)) {
                level_ = spdlog::level::err;
            }; 

            ImGui::TableNextColumn();
            if (ImGui::Selectable("CRITICAL", level_ == spdlog::level::critical)) {
                level_ = spdlog::level::critical;
            }; 

            ImGui::EndTable();
        }
        ImGui::PopFont();

        if constexpr (!in_popup) { ImGui::SetCursorPosX(io.DisplaySize.x*0.5 + sze.x * 0.5 - ImGui::CalcTextSize("Submit").x - 4 * ImGui::GetStyle().FramePadding.x); };
        ImGui::PushFont(io.Fonts->Fonts[0]);
        if (ImGui::Button("Submit")) { 
            this->_path_to_output = path_to_output;
            auto& fm = FileManager::GetInstance();
            fm.SetApiKey(api_key);
            fm.SetWorkspace(path_to_output);
            fm.GetConfig().log_path = path_to_log;
            fm.GetConfig().log_level = level_;
            fm.GetConfig().verbosity = verbosity;
            fm.SaveConfig(CONFIG_FILE_PATH);

            if (this->_underlying_client) {
                this->_underlying_client->update_api_key(api_key);
            } else {
                this->_underlying_client = std::make_shared<client::RiotApiClient>(api_key, std::string(path_to_log), level_, verbosity, true);
                this->request_sender->set_client(this->_underlying_client);
            }
            this->request_sender->set_output_directory(this->_path_to_output);
            this->batch_group->set_sender(this->request_sender);

            DatabaseManager::GetInstance().Init(fm.GetWorkspacePaths().db_file.string());
            submit = true;
        }
        ImGui::PopFont();
        return submit;
    }


    bool RestfulEzreal::configure_new_client(bool allow_close) {

        ImGui::OpenPopup("Configure Client");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        bool unused_open = true;
        if (ImGui::BeginPopupModal("Configure Client", allow_close ? &unused_open : NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (this->conf_form<true>()) {
                ImGui::CloseCurrentPopup();
            };
            ImGui::EndPopup();
        }
        return !unused_open;
    }

    void RestfulEzreal::config_check() {
        if (file_exists(CONFIG_FILE_PATH)) {
            this->configure_from_existing();
            return;
        }
        this->configure_new_client();
    }

    void RestfulEzreal::render_advanced_requests() {
        if (this->batch_group) {
            this->batch_group->set_sender(this->request_sender);
            this->batch_group->render_form();
        }
    }

    void RestfulEzreal::render_guide() {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("User Guide & Technical Reference");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| Architecture, API concepts, pipeline DAGs, and DuckDB schema");
        ImGui::Separator();
        ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 80.0f));
        ImGui::BeginChild("GuideContentScroll", ImVec2(0, 0), false);

        // 1. Overview & Setup
        if (ImGui::CollapsingHeader("1. Overview & Setup (API Key, Expiration, Rate Limits)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextWrapped("RESTfulEzreal is a desktop analytical client for Riot Games developer APIs across League of Legends, Teamfight Tactics, Legends of Runeterra, and Valorant.");
            ImGui::Spacing();
            ImGui::BulletText("Riot Developer Portal Keys: Free personal development keys start with 'RGAPI-' and expire automatically every 24 hours.");
            ImGui::BulletText("Rate Limiting: Riot standard development keys enforce two tier limits: 20 requests per 1 second and 100 requests per 2 minutes (120 seconds).");
            ImGui::BulletText("Zero Leakage Security: RESTfulEzreal loads active credentials at runtime through internal configuration or .env without leaking secrets in logs or terminal outputs.");
            ImGui::BulletText("Key Updates: When your 24h key expires, update it directly via the top-right 'Client State' overlay or the 'Client Configuration' page.");
            ImGui::Spacing();
        }

        // 2. Single Requests (Home)
        if (ImGui::CollapsingHeader("2. Single Requests (Endpoints, Products, Routing Regions)")) {
            ImGui::TextWrapped("The Home page allows executing single ad-hoc API queries across all Riot Games products.");
            ImGui::Spacing();
            ImGui::BulletText("Products: Riot Account-v1, League of Legends, Teamfight Tactics, Legends of Runeterra, Valorant.");
            ImGui::BulletText("Regional Routing vs Platform Routing: Platform endpoints (e.g. League-v4, Champion Mastery) query specific servers like 'KR', 'NA1', 'EUW1'. Regional routes (Account-v1, Match-v5) query continental clusters: 'ASIA', 'AMERICAS', 'EUROPE', 'ESPORTS'.");
            ImGui::BulletText("Summoner-v4 Deprecation: Riot Games has deprecated Summoner name lookup. All user identification begins with Account-v1 (by Riot ID or by PUUID).");
            ImGui::Spacing();
        }

        // 3. Advanced Linked Requests & Pipelines
        if (ImGui::CollapsingHeader("3. Advanced Linked Requests & Pipelines (DAGs, ValuePool, Zip Mode)")) {
            ImGui::TextWrapped("The Advanced Requests canvas provides a visual Directed Acyclic Graph (DAG) editor to chain requests sequentially without writing scripts.");
            ImGui::Spacing();
            ImGui::BulletText("Directed Acyclic Graph (DAG): Request nodes produce JSON responses; child nodes declare links to parent nodes and extract response fields into query parameters.");
            ImGui::BulletText("ValuePool Architecture: Each parameter slot is backed by a ValuePool. Whenever a parent produces an array of results (e.g. 20 match IDs), the child node iterates automatically.");
            ImGui::BulletText("Binding Modes (PoolBindMode):");
            ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.01f, 1.0f), "    - CROSS Mode (Default):");
            ImGui::SameLine();
            ImGui::TextWrapped("Executes Cartesian product across all parameter pools.");
            ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.01f, 1.0f), "    - ZIP Mode:");
            ImGui::SameLine();
            ImGui::TextWrapped("Steps multi-value pools in lockstep 1-to-1 by index (0..N-1) while broadcasting 1-element static pools (e.g. routing region 'ASIA'). Used for correlated tuples like (gameName, tagLine).");
            ImGui::BulletText("ValuePool Flags: 'Unique' enforces deduplication across single or multiple parents. 'Skip DB' checks DuckDB to skip matches or timelines already stored locally.");
            ImGui::Spacing();
        }

        // 4. JSON Extraction Syntax
        if (ImGui::CollapsingHeader("4. JSON Extraction Syntax (@, info.participants[*].puuid, Nested Arrays)")) {
            ImGui::TextWrapped("Edge links specify extraction paths to pluck values from parent JSON responses into child parameters:");
            ImGui::Spacing();
            ImGui::BulletText("Root Arrays: Use '@' or leave blank to extract all elements of a root array (e.g. Match-v5 match list returns an array of strings).");
            ImGui::BulletText("Single-Nested Object Arrays: 'info.participants[*].puuid' extracts PUUIDs across all 10 participants in a match.");
            ImGui::BulletText("Correlated Multi-Key Extraction: 'info.participants[*].riotIdGameName' and 'info.participants[*].riotIdTagline' combined with ZIP mode allow batch profile lookups.");
            ImGui::BulletText("Multi-Level Nested Arrays: 'info.teams[*].bans[*].championId' extracts champion bans across both teams.");
            ImGui::Spacing();
        }

        // 5. Starter Templates
        if (ImGui::CollapsingHeader("5. Starter Templates & Root Parameter Quick-Fill")) {
            ImGui::TextWrapped("RESTfulEzreal includes built-in starter templates accessible via the 'Templates' menu on the Advanced Requests canvas:");
            ImGui::Spacing();
            ImGui::BulletText("Starter Template 1: Match History & Timelines");
            ImGui::TextDisabled("    Node 0: Account-v1 By Riot ID -> Node 1: Match-v5 By Puuid -> Node 2: Match-v5 By Match ID + Node 3: Match-v5 Timeline.");
            ImGui::BulletText("Starter Template 2: Match Participants Expansion");
            ImGui::TextDisabled("    Node 0: Account-v1 By Riot ID -> Node 1: Match-v5 By Puuid -> Node 2: Match-v5 By Match ID -> Node 3: Account-v1 By Riot ID (ZIP mode).");
            ImGui::BulletText("Root Parameter Quick-Fill: Loading a template automatically prompts for starting root inputs (Routing, Game Name, Tagline) and injects them cleanly into root nodes.");
            ImGui::Spacing();
        }

        // 6. DuckDB Universal Schema
        if (ImGui::CollapsingHeader("6. DuckDB Universal Multi-Developer Schema")) {
            ImGui::TextWrapped("All fetched accounts, matches, participants, and timelines are streamed directly into an embedded DuckDB database file (<workspace>/db/ezreal_data.duckdb):");
            ImGui::Spacing();
            ImGui::BulletText("Multi-Developer Key PUUID Resolution: Riot encrypts PUUIDs per developer key. DuckDB maintains 'dev_puuid_mappings(dev_key_prefix, puuid, player_id)' so multiple API keys resolve to the same canonical 'players(player_id)'.");
            ImGui::BulletText("Player Aliases ('player_aliases'): Tracks historical and current Riot IDs (game_name, tag_line, case-insensitive) mapped to player_id.");
            ImGui::BulletText("Matches ('matches'): Stores match_id, game_creation, game_duration, game_version, queue_id, and full raw_json.");
            ImGui::BulletText("Match Participants ('match_participants'): Stores unnested per-participant stats (champion, role, kills, deaths, damage) automatically linked to canonical player_id.");
            ImGui::BulletText("Timelines ('match_timelines'): Stores match_id, frame_interval, and raw JSON timeline frames.");
            ImGui::Spacing();
        }

        // 7. Troubleshooting & Error Codes
        if (ImGui::CollapsingHeader("7. Troubleshooting & Error Codes (429, 403, 404)")) {
            ImGui::TextWrapped("Common Riot API error codes and recommended resolutions:");
            ImGui::Spacing();
            ImGui::BulletText("401 Unauthorized / 403 Forbidden: API key has expired (personal keys expire every 24 hours) or the endpoint is not allowed. Obtain a new key from developer.riotgames.com and update it.");
            ImGui::BulletText("404 Not Found: The player Riot ID, PUUID, or match ID does not exist in the specified routing region. Verify region cluster (ASIA, AMERICAS, EUROPE).");
            ImGui::BulletText("429 Rate Limit Exceeded: The request volume exceeded Riot's quota. RESTfulEzreal's background dispatcher automatically respects rate limit headers and pauses until reset.");
            ImGui::BulletText("500 / 503 Service Unavailable: Riot API gateway or game server downtime. Check Riot status page or retry after a few minutes.");
            ImGui::Spacing();
        }

        ImGui::EndChild();
    }

    void RestfulEzreal::render_client_config() {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Client Configuration");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| Manage API credentials, workspace directories, database, and logging");
        ImGui::Separator();
        ImGui::Spacing();

        auto& fm = FileManager::GetInstance();

        if (!this->_config_key_initialized) {
            std::string current_key = fm.GetApiKey();
            strncpy(this->_config_key_input, current_key.c_str(), sizeof(this->_config_key_input) - 1);
            this->_config_log_level = static_cast<int>(fm.GetConfig().log_level);
            this->_config_verbosity = fm.GetConfig().verbosity;
            this->_config_key_initialized = true;
        }

        ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 80.0f));
        ImGui::BeginChild("ClientConfigContentScroll", ImVec2(0, 0), false);

        // Card 1: API Key & Developer Identity
        ImGui::BeginChild("ApiKeyConfigCard", ImVec2(0, 175), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Riot API Key & Developer Identity");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        std::string current_key = fm.GetApiKey();
        if (!current_key.empty()) {
            std::string masked = "RGAPI-**** (" + std::to_string(current_key.length()) + " chars)";
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "Status: Active API Key configured [%s]", masked.c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Status: No API Key configured!");
        }

        std::string prefix = fm.GetDevKeyPrefix();
        ImGui::Text("Developer Key Prefix: %s (used for multi-dev PUUID identity resolution)", prefix.empty() ? "None" : "RGAPI-**** (active)");

        ImGui::Spacing();
        ImGui::SetNextItemWidth(450.0f);
        ImGui::InputText("##ConfigKeyInput", this->_config_key_input, sizeof(this->_config_key_input), ImGuiInputTextFlags_Password);
        ImGui::SameLine();
        if (ImGui::Button("Save API Key")) {
            if (this->_config_key_input[0] != '\0') {
                fm.SetApiKey(this->_config_key_input);
                fm.SaveConfig(CONFIG_FILE_PATH);
                if (this->_underlying_client) {
                    this->_underlying_client->update_api_key(this->_config_key_input);
                }
                this->_config_status_message = "API key updated and saved successfully.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload from .env")) {
            if (fm.LoadEnvKey(".env") && !fm.GetApiKey().empty()) {
                strncpy(this->_config_key_input, fm.GetApiKey().c_str(), sizeof(this->_config_key_input) - 1);
                if (this->_underlying_client) {
                    this->_underlying_client->update_api_key(fm.GetApiKey());
                }
                this->_config_status_message = "Reloaded API key from .env file.";
            } else {
                this->_config_status_message = "Failed to load .env file or key is empty.";
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();

        // Card 2: Workspace & Storage
        ImGui::BeginChild("WorkspaceConfigCard", ImVec2(0, 210), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Active Workspace & Database Storage");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        const auto& wp = fm.GetWorkspacePaths();
        ImGui::Text("Workspace Directory: %s", wp.root_dir.string().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Browse Workspace...")) {
            std::string chosen = fm.SelectFolderDialog();
            if (!chosen.empty()) {
                fm.SetWorkspace(chosen);
                fm.SaveConfig(CONFIG_FILE_PATH);
                this->_path_to_output = fm.GetConfig().active_workspace_path;
                this->request_sender->set_output_directory(this->_path_to_output);
                DatabaseManager::GetInstance().Reopen(fm.GetWorkspacePaths().db_file.string());
                this->_config_status_message = "Workspace switched to: " + chosen;
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Subdirectory Structure:");
        ImGui::BulletText("Raw Responses: %s", wp.raw_dir.string().c_str());
        ImGui::BulletText("Templates: %s", wp.templates_dir.string().c_str());
        ImGui::BulletText("Logs: %s", wp.log_file.string().c_str());

        ImGui::Spacing();
        ImGui::Text("DuckDB Database: %s", wp.db_file.string().c_str());
        ImGui::SameLine();
        if (DatabaseManager::GetInstance().IsInitialized()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "[Connected: %ld players, %ld matches]",
                               static_cast<long>(DatabaseManager::GetInstance().GetPlayerCount()),
                               static_cast<long>(DatabaseManager::GetInstance().GetMatchCount()));
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[Not Initialized]");
        }
        ImGui::SameLine();
        if (ImGui::Button("Reconnect DB")) {
            DatabaseManager::GetInstance().Reopen(wp.db_file.string());
            this->_config_status_message = "Reconnected DuckDB database.";
        }
        ImGui::EndChild();
        ImGui::Spacing();

        // Card 3: Logging & Network
        ImGui::BeginChild("LoggingConfigCard", ImVec2(0, 160), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Client Logging & Network Defaults");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        static const char* log_levels[] = { "Trace", "Debug", "Info", "Warn", "Error", "Critical", "Off" };
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::Combo("Log Level", &this->_config_log_level, log_levels, IM_ARRAYSIZE(log_levels))) {
            fm.GetConfig().log_level = static_cast<spdlog::level::level_enum>(this->_config_log_level);
        }
        ImGui::SameLine(300.0f);
        if (ImGui::Checkbox("Verbose Logging", &this->_config_verbosity)) {
            fm.GetConfig().verbosity = this->_config_verbosity;
        }

        ImGui::Spacing();
        if (ImGui::Button("Save Client Settings")) {
            fm.GetConfig().log_level = static_cast<spdlog::level::level_enum>(this->_config_log_level);
            fm.GetConfig().verbosity = this->_config_verbosity;
            fm.SaveConfig(CONFIG_FILE_PATH);
            this->_config_status_message = "Saved client configuration settings to disk.";
        }

        if (!this->_config_status_message.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.01f, 1.0f), "%s", this->_config_status_message.c_str());
        }

        ImGui::EndChild();
        ImGui::EndChild();
    }

    void RestfulEzreal::refresh_history_files() {
        this->_cached_file_list.clear();
        auto raw_dir = FileManager::GetInstance().GetWorkspacePaths().raw_dir;
        if (std::filesystem::exists(raw_dir)) {
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(raw_dir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".json") {
                        std::string rel = std::filesystem::relative(entry.path(), raw_dir).string();
                        std::string abs = entry.path().string();
                        this->_cached_file_list.push_back({rel, abs});
                    }
                }
            } catch (...) {}
        }
    }

    static void format_json_pretty(const std::string& raw, std::string& out_formatted, std::vector<std::string>& out_lines) {
        out_formatted.clear();
        out_lines.clear();
        if (raw.empty()) return;

        out_formatted.reserve(raw.size() + raw.size() / 4);
        std::string cur_line;
        cur_line.reserve(128);
        int indent = 0;
        bool in_string = false;
        bool escaped = false;

        auto push_line = [&]() {
            out_lines.push_back(cur_line);
            out_formatted.append(cur_line);
            out_formatted.push_back('\n');
            cur_line.clear();
            if (indent > 0) {
                cur_line.append(static_cast<std::size_t>(indent * 2), ' ');
            }
        };

        const std::size_t len = raw.size();
        for (std::size_t i = 0; i < len; ++i) {
            char c = raw[i];
            if (c == '\0') continue;

            if (in_string) {
                cur_line.push_back(c);
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    in_string = false;
                }
                continue;
            }

            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                continue;
            }

            if (c == '"') {
                in_string = true;
                cur_line.push_back(c);
            } else if (c == '{' || c == '[') {
                char close_ch = (c == '{') ? '}' : ']';
                std::size_t j = i + 1;
                while (j < len && (raw[j] == ' ' || raw[j] == '\t' || raw[j] == '\r' || raw[j] == '\n')) {
                    ++j;
                }
                if (j < len && raw[j] == close_ch) {
                    cur_line.push_back(c);
                    cur_line.push_back(close_ch);
                    i = j;
                } else {
                    cur_line.push_back(c);
                    ++indent;
                    push_line();
                }
            } else if (c == '}' || c == ']') {
                indent = std::max(0, indent - 1);
                push_line();
                cur_line.push_back(c);
            } else if (c == ',') {
                cur_line.push_back(c);
                push_line();
            } else if (c == ':') {
                cur_line.append(": ");
            } else {
                cur_line.push_back(c);
            }
        }
        if (!cur_line.empty()) {
            out_lines.push_back(cur_line);
            out_formatted.append(cur_line);
        }
    }

    void RestfulEzreal::render_json() {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Request History & Raw JSON Inspector");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| Browse and view saved JSON responses from the active workspace");
        ImGui::Separator();
        ImGui::Spacing();

        int current_frame = ImGui::GetFrameCount();
        if (current_frame - this->_history_last_refresh_frame > 120 || this->_history_last_refresh_frame < 0) {
            this->refresh_history_files();
            this->_history_last_refresh_frame = current_frame;
        }

        // Category filter
        static const char* categories[] = { "All Files", "Accounts", "Matches", "Timelines" };
        ImGui::SetNextItemWidth(180.0f);
        ImGui::Combo("Filter", &this->_history_category, categories, IM_ARRAYSIZE(categories));
        ImGui::SameLine();
        if (ImGui::Button("Refresh Files")) {
            this->refresh_history_files();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu files found in workspace raw storage)", this->_cached_file_list.size());
        ImGui::Spacing();

        ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 80.0f));
        float avail_h = ImGui::GetContentRegionAvail().y;
        ImGui::BeginChild("HistoryFileListPanel", ImVec2(380.0f, avail_h), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Response Files");
        ImGui::PopStyleColor();
        ImGui::Separator();

        std::vector<std::pair<std::string, std::string>> filtered;
        for (const auto& item : this->_cached_file_list) {
            if (this->_history_category == 1 && item.first.find("account") == std::string::npos) continue;
            if (this->_history_category == 2 && item.first.find("match") == std::string::npos) continue;
            if (this->_history_category == 3 && item.first.find("timeline") == std::string::npos) continue;
            filtered.push_back(item);
        }

        if (filtered.empty()) {
            ImGui::TextDisabled("No JSON files found matching filter.");
        } else {
            for (const auto& [rel, abs] : filtered) {
                bool selected = (this->_history_selected_file == abs);
                if (ImGui::Selectable(rel.c_str(), selected)) {
                    this->_history_selected_file = abs;
                    std::ifstream f(abs);
                    if (f.is_open()) {
                        std::ostringstream ss;
                        ss << f.rdbuf();
                        format_json_pretty(ss.str(), this->_history_file_content, this->_history_formatted_lines);
                        this->_history_content_loaded = true;
                    } else {
                        this->_history_file_content = "Failed to open file: " + abs;
                        this->_history_formatted_lines = { this->_history_file_content };
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("HistoryFileViewPanel", ImVec2(0, avail_h), true);
        if (!this->_history_selected_file.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
            ImGui::TextWrapped("File: %s", this->_history_selected_file.c_str());
            ImGui::PopStyleColor();
            if (ImGui::Button("Copy JSON")) {
                ImGui::SetClipboardText(this->_history_file_content.c_str());
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(%zu lines, %zu bytes formatted)", this->_history_formatted_lines.size(), this->_history_file_content.size());
            ImGui::Separator();

            ImGui::BeginChild("JsonViewerScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(this->_history_formatted_lines.size()));
            while (clipper.Step()) {
                for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; ++line_i) {
                    ImGui::TextDisabled("%5d  ", line_i + 1);
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::TextUnformatted(this->_history_formatted_lines[static_cast<std::size_t>(line_i)].c_str());
                }
            }
            ImGui::EndChild();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
            ImGui::TextUnformatted("JSON Inspector");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("Select a response file from the list on the left to inspect its formatted JSON content.");
        }
        ImGui::EndChild();
    }

    void RestfulEzreal::render_rate_status() {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Request Status & System Telemetry");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| Live rate limit monitors, async queues, DuckDB counts, and server status");
        ImGui::Separator();
        ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 80.0f));
        ImGui::BeginChild("StatusContentScroll", ImVec2(0, 0), false);

        auto snapshots = this->request_sender ? this->request_sender->get_rate_limit_snapshot() : std::vector<riotcpp::rate::RegionRateSnapshot>{};
        int wait_sec = this->request_sender ? this->request_sender->get_current_wait_seconds() : 0;

        float rate_card_height = 175.0f;
        if (!snapshots.empty()) {
            rate_card_height = 65.0f;
            for (const auto& s : snapshots) {
                rate_card_height += 40.0f + static_cast<float>(s.app_buckets.size()) * 42.0f + static_cast<float>(s.method_limits.size()) * 24.0f;
            }
            rate_card_height = std::max(175.0f, std::min(460.0f, rate_card_height));
        }

        // Section 1: Rate Limiter Status
        ImGui::BeginChild("RateLimitCard", ImVec2(0, rate_card_height), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Riot API Rate Limiter (Per-Region & Per-Method Token Buckets)");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        if (wait_sec > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f),
                "Status: THROTTLED — Waiting %d s for active rate-limit window reset", wait_sec);
        } else {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f),
                "Status: Normal (No Active Rate-Limit Backoff)");
        }
        ImGui::Spacing();

        if (snapshots.empty()) {
            ImGui::TextDisabled("No regional requests dispatched yet in this session. Baseline Personal Key buckets:");
            ImGui::Text("App Rate Limit (Short-Term: 20 req / 1s):");
            ImGui::ProgressBar(0.0f, ImVec2(420.0f, 0.0f), "0 / 20 req (1s)");
            ImGui::Spacing();
            ImGui::Text("App Rate Limit (Long-Term: 100 req / 120s):");
            ImGui::ProgressBar(0.0f, ImVec2(420.0f, 0.0f), "0 / 100 req (120s)");
        } else {
            for (const auto& snap : snapshots) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
                ImGui::Text("[%s - %s Cluster]", snap.routing_name.c_str(), snap.routing_category.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine();
                if (snap.app_wait_time_sec > 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "(App Limit Cooldown: %d s)", snap.app_wait_time_sec);
                } else {
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "(App Bucket Ready)");
                }

                for (const auto& b : snap.app_buckets) {
                    float frac = (b.limit > 0) ? std::min(1.0f, static_cast<float>(b.count) / static_cast<float>(b.limit)) : 0.0f;
                    char overlay[128];
                    if (b.seconds_until_reset > 0) {
                        std::snprintf(overlay, sizeof(overlay), "%d / %d req (%ds window, resets in %ds)",
                                      b.count, b.limit, b.duration_sec, b.seconds_until_reset);
                    } else {
                        std::snprintf(overlay, sizeof(overlay), "%d / %d req (%ds window)",
                                      b.count, b.limit, b.duration_sec);
                    }
                    ImGui::Text("  App Window (%ds):", b.duration_sec);
                    ImGui::SameLine(180.0f);
                    if (frac >= 1.0f) {
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.25f, 0.2f, 1.0f));
                    } else if (frac >= 0.8f) {
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.95f, 0.6f, 0.1f, 1.0f));
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
                    }
                    ImGui::ProgressBar(frac, ImVec2(420.0f, 0.0f), overlay);
                    ImGui::PopStyleColor();
                }

                for (const auto& m : snap.method_limits) {
                    if (m.buckets.empty()) continue;
                    const auto& mb = m.buckets.front();
                    if (mb.seconds_until_reset > 0) {
                        ImGui::BulletText("Method [%s]: %d / %d req (%ds window, resets in %ds)",
                                          m.method_key.c_str(), mb.count, mb.limit, mb.duration_sec, mb.seconds_until_reset);
                    } else {
                        ImGui::BulletText("Method [%s]: %d / %d req (%ds window)",
                                          m.method_key.c_str(), mb.count, mb.limit, mb.duration_sec);
                    }
                }
                ImGui::Spacing();
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();

        // Section 2: Queue Telemetry
        ImGui::BeginChild("QueueTelemetryCard", ImVec2(0, 215), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Asynchronous Request Queue Telemetry");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        int simple_q = this->request_sender ? this->request_sender->get_simple_size() : 0;
        int batch_q = this->request_sender ? this->request_sender->get_batch_size() : 0;
        int generic_q = this->request_sender ? this->request_sender->get_generic_size() : 0;
        int in_flight = this->request_sender ? this->request_sender->get_in_flight_count() : 0;
        int batch_disp = this->request_sender ? this->request_sender->get_batch_dispatched_count() : 0;
        int batch_nodes = this->request_sender ? this->request_sender->get_batch_active_nodes() : 0;
        int total_ok = this->request_sender ? this->request_sender->get_total_completed_count() : 0;
        int total_fail = this->request_sender ? this->request_sender->get_total_failed_count() : 0;
        int switches = this->request_sender ? this->request_sender->get_interleaved_switches() : 0;

        bool is_busy = (simple_q > 0 || batch_q > 0 || generic_q > 0 || in_flight > 0);
        ImGui::Text("Worker Thread: ");
        ImGui::SameLine();
        if (wait_sec > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f),
                "RATE-LIMIT WAIT (%ds remaining | %d batch active)", wait_sec, batch_q);
        } else if (is_busy) {
            ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.01f, 1.0f),
                "PROCESSING (%d active/queued tasks | %d HTTP in-flight)", simple_q + batch_q + generic_q, in_flight);
        } else {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "IDLE (Ready for queries)");
        }

        ImGui::BulletText("Simple Requests (Active + Queued): %d", simple_q);
        ImGui::BulletText("Linked Batch Pipelines (Active + Queued): %d  (Active DAG Nodes: %d | Current Batch Dispatched: %d)",
                          batch_q, batch_nodes, batch_disp);
        ImGui::BulletText("Session Requests Completed: %d Succeeded  |  %d Failed  |  %d Rate-Limit Node Switches",
                          total_ok, total_fail, switches);

        if (this->request_sender) {
            auto [rec_name, rec_params] = this->request_sender->get_recent_request_info();
            std::string param_preview;
            for (std::size_t i = 0; i < rec_params.size(); ++i) {
                if (i > 0) param_preview += ", ";
                if (rec_params[i].size() > 24) {
                    param_preview += rec_params[i].substr(0, 20) + "...";
                } else {
                    param_preview += rec_params[i];
                }
            }
            ImGui::Spacing();
            if (!param_preview.empty()) {
                ImGui::TextDisabled("Most Recent Dispatch: %s [%s]", rec_name.c_str(), param_preview.c_str());
            } else {
                ImGui::TextDisabled("Most Recent Dispatch: %s", rec_name.c_str());
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();

        // Section 3: DuckDB Database Telemetry
        ImGui::BeginChild("DatabaseTelemetryCard", ImVec2(0, 160), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Embedded DuckDB Telemetry");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        auto& db = DatabaseManager::GetInstance();
        if (db.IsInitialized()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "Connection: ONLINE (%s)", db.GetDatabasePath().c_str());
            std::error_code ec;
            auto sz = std::filesystem::file_size(db.GetDatabasePath(), ec);
            if (!ec) {
                ImGui::SameLine();
                ImGui::TextDisabled("[%zu KB]", sz / 1024);
            }
            ImGui::Spacing();
            ImGui::BulletText("Canonical Players ('players'): %ld", static_cast<long>(db.GetPlayerCount()));
            ImGui::BulletText("Matches Ingested ('matches'): %ld", static_cast<long>(db.GetMatchCount()));
            ImGui::BulletText("Timelines Ingested ('match_timelines'): %ld", static_cast<long>(db.GetTimelineCount()));
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Connection: NOT INITIALIZED");
        }
        ImGui::EndChild();
        ImGui::Spacing();

        // Section 4: Regional Server Availability Grid
        ImGui::BeginChild("RegionalStatusCard", ImVec2(0, 200), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::TextUnformatted("Regional Server Availability Grid (16 Regions)");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        static const char* reg_names[16] = {
            "BR1", "EUN1", "EUW1", "JP1", "KR", "LA1", "LA2", "NA1",
            "OC1", "TR1", "RU", "PH2", "SG2", "TH2", "TW2", "VN2"
        };

        if (ImGui::BeginTable("RegionsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            for (int r = 0; r < 16; r++) {
                if (r % 4 == 0) ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(r % 4);
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "[OK]");
                ImGui::SameLine();
                ImGui::Text("%s", reg_names[r]);
            }
            ImGui::EndTable();
        }
        ImGui::Spacing();
        if (this->request_sender && ImGui::Button("Test Regional Latency")) {
            this->request_sender->test_regions();
        }
        ImGui::EndChild();

        ImGui::EndChild();
    }


    static std::unique_ptr<std::string> display_custom_file(const std::string& path, const float x_begin, const float x_end) {
        /*
         * Display the file the user gave. 
         * ASSUMES VALID FILE PATH.
         */
        static std::unique_ptr<std::string> contents = std::make_unique<std::string>("");

        static ImGuiIO& io = ImGui::GetIO();

        static bool first_pass = true;
        static re_utils::FBF_STRING_256 disp;

        if (first_pass) {
            first_pass = false;
            std::ifstream file(path);
            std::ostringstream ss;
            ss << file.rdbuf();
            *contents = ss.str();
            file.close();
            contents->insert(contents->end(), simdjson::SIMDJSON_PADDING, '\0');
            disp = re_utils::create_fbf_string<256, re_utils::FBF_STRING_256>(*contents, 1);
        }


        ImGui::SetCursorPosY(io.DisplaySize.y * 0.333);
        
        ImGui::PushFont(io.Fonts->Fonts[3]);
        ImGui::SetCursorPosX(x_begin);
        if (re_utils::fbf_text(disp)) {
            ImGui::PopFont();
            return std::move(contents);    
        };
        ImGui::PopFont();

        return nullptr;
    }

    static bool display_in_valid_json(simdjson::ondemand::parser& parser, simdjson::ondemand::document& doc, const std::string& file_contents, bool retry = false) {
        /* 
         * Display valid message in green upon success
         * red upon failure to pass
         */
        static bool parsed = false;
        static bool success;
        if (retry) {
            parsed = false; // force retry
        }
        if (!parsed) {
            try {
                doc = parser.iterate(file_contents.data(), strlen(file_contents.data()), sizeof(file_contents.data()));
                success = true;
            }
            catch (simdjson::simdjson_error& ex) {
                success = false;
            }
        }
        if (success) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::TextUnformatted("Valid Json Format");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::TextUnformatted("Invalid Json");
            ImGui::PopStyleColor();
        }
        return success;
    }

    using Parser = simdjson::ondemand::parser;
    using Document = simdjson::ondemand::document;

    void RestfulEzreal::instantiate_client(int* int_out, int* parse_out, const std::string& file_path) {
        auto& fm = FileManager::GetInstance();
        bool ok = fm.LoadConfig(file_path.empty() ? CONFIG_FILE_PATH : file_path);
        if (!ok || fm.GetApiKey().empty() || fm.GetApiKey().find("YOUR-ACTUAL") != std::string::npos) {
            if (fm.LoadEnvKey(".env") && !fm.GetApiKey().empty()) {
                if (!ok) {
                    fm.SetWorkspace("./output");
                }
                ok = true;
            }
        }
        if (!ok) {
            *parse_out = 2;
            *int_out = 2;
            return;
        }
        *parse_out = 1;
        *int_out = 1;

        const auto& cfg = fm.GetConfig();
        this->_path_to_output = cfg.active_workspace_path;
        this->_underlying_client = std::make_shared<client::RiotApiClient>(cfg.api_key, cfg.log_path, cfg.log_level, cfg.verbosity, true);
        this->request_sender->set_client(this->_underlying_client);
        this->request_sender->set_output_directory(this->_path_to_output);
        DatabaseManager::GetInstance().Init(fm.GetWorkspacePaths().db_file.string());
    }

    bool RestfulEzreal::start_up_from_existing(int* ret) {
        static float    x_sze     = 0.0f;
        static bool     calc_once = true;
        static ImGuiIO& io        = ImGui::GetIO();

        static Parser parser;
        static Document doc;
        static int successful_parse = 0; // 0 - not finished, 1 - success, 2 - failure
        static int instantiate_success = 0;

        if (calc_once) {
            //avoid push/pop font on every frame
            ImGui::PushFont(io.Fonts->Fonts[1]);
            x_sze = 0.5 * ImGui::CalcTextSize("RESTfulEzreal;").x;
            ImGui::PopFont();

            this->request_sender->add_generic(std::bind(&RestfulEzreal::instantiate_client, this, &instantiate_success, &successful_parse, this->custom_config_path));
            this->request_sender->test_regions();
            calc_once = false;
        }
        const float x_begin = io.DisplaySize.x * 0.5 - x_sze;
        const float x_end   = io.DisplaySize.x * 0.5 + x_sze;

        bool finished = false;

        re_utils::full_display();
        // ------------ BEGIN WINDOW RENDER ---------------
        ImGui::Begin("From existing validation", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
        
        ImGui::SetCursorPos(ImVec2(x_begin, io.DisplaySize.y*0.25));
        ImGui::TextUnformatted("Validating File Format...");

        ImGui::SameLine();
        re_utils::pending_text_right("Pending", "Valid Json", "Invalid File Format", successful_parse, x_end);

        ImGui::SetCursorPosX(x_begin);
        ImGui::TextUnformatted("Instantiating Client...");

        ImGui::SameLine();
        re_utils::pending_text_right("Pending", "Successful", "Failure", instantiate_success, x_end);

        ImGui::NewLine();
        int result = this->request_sender->region_test_display();

        ImGui::End();
        // ------------ END WINDOW RENDER ----------------

        if (result == 2) {
            *ret = -1; // go back to home page
            calc_once = true; // reset calc_once to allowed for calling again
        } else if (result == 1) {
            calc_once = true;
            finished = true;
        }

        return finished;
    }

    bool RestfulEzreal::start_up_default(int* ret) {

        static const float title_size = re_utils::text_size(1, "RESTfulEzreal;");
        static ImGuiIO& io = ImGui::GetIO();

        re_utils::full_display();

        ImGui::Begin("From existing", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        ImGui::SetCursorPosY(io.DisplaySize.y * 0.5);
        ImGui::SetCursorPosX(io.DisplaySize.x * 0.5 - title_size * 0.5);
        if (ImGui::Button("back")) {
            *ret = -1;
        }
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
        static float lne_height = ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetCursorPosX(io.DisplaySize.x * 0.5 - title_size * 0.25 - ImGui::CalcTextSize("New API Key ").x);
        ImGui::TextUnformatted("New API Key ");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
        ImGui::SameLine();
        ImGui::TextUnformatted(": ");
        ImGui::PopStyleColor();
        
        static char api_key[256];
        ImGui::SameLine();
        ImGui::PushFont(io.Fonts->Fonts[3]);
        static float lne_height2 = ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetCursorPosY(io.DisplaySize.y * 0.5 - (lne_height2 - lne_height) * 0.5);
        ImGui::SetNextItemWidth(title_size * 0.75);
        ImGui::InputText("##NEW API KEY", api_key, 256, ImGuiInputTextFlags_None);
        ImGui::SameLine();
        const float end_ofline = ImGui::GetCursorPosX();
        ImGui::PopFont();
        
        ImGui::NewLine();
        ImGui::NewLine();
        ImGui::SameLine();
        if (api_key[0] == '\0') {
            ImGui::SetCursorPosX(end_ofline - ImGui::CalcTextSize("skip").x - 4 * ImGui::GetStyle().FramePadding.x);
            if (ImGui::Button("skip")) {
                this->custom_config_path = CONFIG_FILE_PATH; 
                *ret = 2;
            }
        } else {
            ImGui::SetCursorPosX(end_ofline - ImGui::CalcTextSize("add key").x);
            if (ImGui::Button("add key")) {
                FileManager::GetInstance().SetApiKey(api_key);
                FileManager::GetInstance().SaveConfig(CONFIG_FILE_PATH);
                this->custom_config_path = CONFIG_FILE_PATH;
                *ret = 2;
            }
        }

        ImGui::End();

        return true;
    }
}

