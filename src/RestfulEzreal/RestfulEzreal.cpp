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

    void RestfulEzreal::OnUIRender() {
        // Main render function for the user interface
#if DEBUG_MODE
        ImGui::ShowMetricsWindow();
#endif
        static bool welcome = true;
        static int ret = -1;
        if (welcome) {
            switch (ret) {
                case -1:
                    ret = this->render_welcome(); break;
                case 1:
                    welcome = this->start_up_default(&ret); break;
                case 2:
                    welcome = !this->start_up_from_existing(&ret);
                    break;
                case 3:
                    ret = this->render_welcome_config(); break;
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
        ImGui::SetNextWindowPos(ImVec2(0.1225 * io.DisplaySize.x, 0));
        ImGui::SetNextWindowSize(ImVec2((1-0.1225) * io.DisplaySize.x, io.DisplaySize.y));

        ImGui::Begin("RestfulEzreal", NULL, window_flags);

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
        ImGui::Text("Most Recent Request");
        ImGui::Text(req_name.data()); // null terminated
        for (const std::string& param_desc : param_descriptions) {
            ImGui::Text(param_desc.data());
        }
    }

    void RestfulEzreal::render_client_status() {

        static bool open = true;
        static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::Begin("Client State", &open, window_flags);
        
        if (!this->_underlying_client) {
            ImGui::Text("Client not intialised");
            ImGui::End();
            return;
        }
        static bool reconfiguring = false;
        if (ImGui::Button("Reconfigure Client")) {
            reconfiguring = true;
        }
        if (reconfiguring) {
            if (this->configure_new_client(true)) {
                reconfiguring = false;
            }// allow the user to close the modal incase of not wanting to reconfigure
        }

        render_recent_request(this->request_sender->recent_request, this->request_sender->recent_params);

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

    void RestfulEzreal::configure_from_existing(bool custom_path, const std::string& file_path) {
            simdjson::ondemand::parser parser;
            simdjson::padded_string contents;
            if (custom_path) {
                contents = simdjson::padded_string::load(file_path);
            } else {
                contents = simdjson::padded_string::load(CONFIG_FILE_PATH);
            }
            simdjson::ondemand::document doc = parser.iterate(contents);

            logging::LEVEL report_level = static_cast<logging::LEVEL>(doc["log-level"].get_int64().value());
            this->_path_to_output = doc["output-path"].get_string().value();

            std::string log_path;
            log_path = doc["log-path"].get_string().value();

            this->_underlying_client = std::make_shared<client::RiotApiClient>(CONFIG_FILE_PATH, log_path, report_level, doc["verbosity"].get_bool().value());
            this->request_sender->set_client(this->_underlying_client);
            this->request_sender->set_output_directory(this->_path_to_output);
            this->batch_group->set_sender(this->request_sender);
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
        if constexpr (!in_popup) { ImGui::SetCursorPosX(x_align); } // dont align in popups only title
        if (*active) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); }
        ImGui::Text(title);
        ImGui::PopFont();

        ImGui::PushFont(io.Fonts->Fonts[3]);
        if constexpr (!in_popup) { 
            ImGui::SetCursorPosX(x_align); 
            static bool frst = true;
            static float txt_width;
            if (frst) {
                ImGui::PushFont(io.Fonts->Fonts[1]); // width of title 
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
        static char path_to_output[64] = "";
        static bool verbosity = false;
        static bool selected[5] = {};

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
        
        // highlight array
        static bool highlight[3] = { false, false, false };
        //render_fields;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.58f, 1.0f)); // dull color
        render_field<in_popup>(256, api_key,        "API-KEY:",     "##API-KEY", ImGuiInputTextFlags_None, &highlight[0], x_ali);
        render_field<in_popup>(64,  path_to_log,    "LOG PATH:",    "##LogPath", ImGuiInputTextFlags_None, &highlight[1], x_ali);
        render_field<in_popup>(256, path_to_output, "OUTPUT PATH:", "##OutPath", ImGuiInputTextFlags_None, &highlight[2], x_ali);
        ImGui::PopStyleColor();

        static logging::LEVEL level_ = logging::LEVEL::INFO;
        
        ImGui::PushFont(io.Fonts->Fonts[0]);
        if constexpr (!in_popup) {ImGui::SetCursorPosX(x_ali);}
        float context_width = 0.0f;
        if constexpr (!in_popup) { 
            context_width = sze.x;
        };
        if (ImGui::BeginTable("Logging Level", 5, ImGuiTableFlags_NoSavedSettings, ImVec2(context_width, 0.0f), context_width)) {

            ImGui::TableNextColumn();
            if (ImGui::Selectable("DEBUG",    level_ == logging::LEVEL::DEBUG)) {
                level_ = logging::LEVEL::DEBUG;
            };

            ImGui::TableNextColumn();
            if (ImGui::Selectable("INFO",     level_ == logging::LEVEL::INFO)) {
                level_ = logging::LEVEL::INFO;
            };

            ImGui::TableNextColumn();
            if (ImGui::Selectable("WARNING",  level_ == logging::LEVEL::WARNING)) {
                level_ = logging::LEVEL::WARNING;
            }; 

            ImGui::TableNextColumn();
            if (ImGui::Selectable("ERRORS",   level_ == logging::LEVEL::ERRORS)) {
                level_ = logging::LEVEL::ERRORS;
            }; 

            ImGui::TableNextColumn();
            if (ImGui::Selectable("CRITICAL", level_ == logging::LEVEL::CRITICAL)) {
                level_ = logging::LEVEL::CRITICAL;
            }; 

            ImGui::EndTable();
        }
        ImGui::PopFont();

        if constexpr (!in_popup) { ImGui::SetCursorPosX(io.DisplaySize.x*0.5 + sze.x * 0.5 - ImGui::CalcTextSize("Submit").x - 4 * ImGui::GetStyle().FramePadding.x); };
        ImGui::PushFont(io.Fonts->Fonts[0]);
        if (ImGui::Button("Submit")) { 
            this->_path_to_output = path_to_output;
            write_config_file(api_key, path_to_log, path_to_output, verbosity, level_);
            this->_underlying_client = std::make_shared<client::RiotApiClient>(CONFIG_FILE_PATH, path_to_log, level_, verbosity);
            this->request_sender->set_client(this->_underlying_client);
            this->request_sender->set_output_directory(this->_path_to_output);
            this->batch_group->set_sender(this->request_sender);
            this->_path_to_output = path_to_output;
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

        this->batch_group->render_form();

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

        Parser parser;
        Document doc;
        std::string contents;
        try {
            std::ifstream config(file_path);
            if (!config.is_open()) {
                throw std::runtime_error("Invalid file path given, exiting config...");
            }
            std::stringstream intermediate; intermediate << config.rdbuf();
            config.close();
            contents = intermediate.str();
            contents.insert(contents.end(), simdjson::SIMDJSON_PADDING, '\0');
            doc = parser.iterate(contents.data(), strlen(contents.data()), sizeof(contents.data()));
            *parse_out = 1; // success
        }
        catch (simdjson::simdjson_error& ex) {
            std::cerr << "Invalid Json Format, exiting config..." << std::endl;
            *parse_out = 2;
            *int_out = 2;
            return;
        }
        catch (std::runtime_error& ex) {
            std::cerr << ex.what() << std::endl;
            *parse_out = 2;
            *int_out = 2;
            return;
        }
        // can probably template
        std::string log_path;
        try {
            log_path = doc["log-path"].get_string().value();
        }
        catch (simdjson::simdjson_error& ex) {
            std::cerr << "Log path not given, defaulting to logs.log in binary directory" << std::endl;
            log_path = "logs.log";
        }

        logging::LEVEL loglevel;
        try {
            loglevel = static_cast<logging::LEVEL>(doc["log-level"].get_int64().value());
        }
        catch (simdjson::simdjson_error& ex) {
            std::cerr << "Log Level not given, defaulting to INFO" << std::endl;
            loglevel = logging::LEVEL::INFO;
        }
        bool verbosity;
        try {
            verbosity = doc["verbosity"].get_bool().value();
        }
        catch (simdjson::simdjson_error& ex) {
            std::cerr << "Log verbosity not given, defaulting to false" << std::endl;
            verbosity = false;
        }
        
        try {
            this->_path_to_output = doc["output-path"].get_string().value();
        }
        catch (simdjson::simdjson_error& ex) {
            std::cerr << "Output path not given, defaulting to \".\\output\\\" in binary dir" << std::endl;
            this->_path_to_output = "./output/";
        }

        try {
            doc["api-key"];
            *int_out = 1;
        } 
        catch (simdjson::simdjson_error& ex) {
            *int_out = 2;
        }

        this->_underlying_client = std::make_shared<client::RiotApiClient>(file_path, log_path, loglevel, verbosity);
        this->request_sender->set_client(this->_underlying_client);
        this->request_sender->set_output_directory(this->_path_to_output);
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

    static void re_write_config(const char* new_key) {

        std::ifstream default_config(CONFIG_FILE_PATH);
        std::ofstream    temp_config("./tempEzrealRunes.json");

        if (!default_config.is_open()) {
            throw std::runtime_error("No Configuration File");
        }

        temp_config << "{\n\t\"api-key\" : \"" << new_key << "\"\n,";

        std::string lne;
        int counter = 0;
        while (getline(default_config, lne)) {
            if (counter > 1) {
                temp_config << lne << "\n";
            }
            counter++;
        }
        default_config.close();
        temp_config.close();

        remove(CONFIG_FILE_PATH);
        rename("./tempEzrealRunes.json", CONFIG_FILE_PATH);
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
                re_write_config(api_key);
                this->custom_config_path = CONFIG_FILE_PATH;
                *ret = 2;
            }
        }


        ImGui::End();

        return true;
    }
}
