#include "QueryForm.h"
#include "TemplateManager.h"
#include "FileManager.h"
#include "BatchRequests.h"
#include "RestfulEzreal.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <memory>
#include <functional>
#include <numeric>
#include <format>
#include <numeric>
#include <string_view>



namespace restfulEz {

    bool QUERY_FORM::operator==(const QUERY_FORM& other_form) {
        return this->_ID == other_form._ID;
    }

    BaseForm::BaseForm(const BaseForm& copy) {
        this->_game_ind = copy._game_ind;
        this->_endpoint_ind = copy._endpoint_ind;
        this->_endpoint_method_ind = copy._endpoint_method_ind;

        this->_game_name = copy._game_name;
        this->_endpoint = copy._endpoint;
        this->_endpoint_method = copy._endpoint_method;
        
        this->_param_names = copy._param_names;
        this->_params_in_form = copy._params_in_form;
        this->_type_ordering = copy._type_ordering;

        this->_accepts_optional = copy._accepts_optional;
        this->_optional_names = copy._optional_names;
        this->_optional_inputs = copy._optional_inputs;
        this->_optional_types = copy._optional_types;
        this->_optionals_to_send = copy._optionals_to_send;

        this->_n_used_optional_p1 = copy._n_used_optional_p1;
        this->_n_params = copy._n_params;
        
        this->_ID = copy._ID;
    }

    BaseForm::BaseForm(const std::size_t n_params, const int game_ind, const int endpoint_ind, const int endpoint_method_ind, const char* game_name, 
            const char* endpoint, const char* endpoint_method, 
            std::vector<P_NAME> param_names, std::vector<ImGuiInputTextFlags> type_ordering,
            bool accepts_optional, std::vector<P_NAME> optional_names, 
            std::vector<PARAM_CONT> optional_inputs, std::vector<ImGuiInputTextFlags> optional_types) 
        : _game_name(game_name), _endpoint(endpoint), _endpoint_method(endpoint_method)
    {
        this->_n_params = n_params;
        this->_param_names = param_names;

        this->_type_ordering = type_ordering;
        this->_params_in_form.resize(n_params);

        this->_accepts_optional = accepts_optional;
        this->_optional_names = optional_names;
        this->_optional_inputs = optional_inputs;
        this->_optional_types = optional_types;

        for (int i = 0; i < optional_names.size(); i++) {
            this->_optionals_to_send.push_back(0);
        }

        if (this->_accepts_optional) {this->_n_used_optional_p1 = 1;}

        this->_game_ind = game_ind;
        this->_endpoint_ind = endpoint_ind;
        this->_endpoint_method_ind = endpoint_method_ind;
    };

    static std::string trim_str(const std::string& s) {
        std::size_t start = 0;
        std::size_t end = s.size();
        while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) ++start;
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) --end;
        return s.substr(start, end - start);
    }

    static std::string to_upper_ascii(std::string s) {
        for (char& c : s) {
            if (c >= 'a' && c <= 'z') c = static_cast<char>(c - ('a' - 'A'));
        }
        return s;
    }

    int BaseForm::expected_routing_indicator() const noexcept {
        if (this->_game_name == "Riot" || this->_game_ind == 0) {
            return REGIONAL_INDICATOR;
        }
        if (this->_game_name == "Legends of Runeterra") {
            return REGIONAL_INDICATOR;
        }
        if (this->_game_name == "Valorant") {
            return VAL_PLATFORM_INDICATOR;
        }
        if (this->_game_name == "League of Legends" || this->_game_ind == 1) {
            if (this->_endpoint == "Match" || this->_endpoint_ind == 6) {
                return REGIONAL_INDICATOR;
            }
            return PLATFORM_INDICATOR;
        }
        if (this->_game_name == "Teamfight Tactics" || this->_game_ind == 2) {
            if (this->_endpoint == "Match" || this->_endpoint_ind == 1) {
                return REGIONAL_INDICATOR;
            }
            return PLATFORM_INDICATOR;
        }
        return PLATFORM_INDICATOR;
    }

    const std::vector<const char*>& BaseForm::valid_routing_options() const noexcept {
        static const std::vector<const char*> regional_opts = {
            "AMERICAS", "ASIA", "EUROPE", "SEA"
        };
        static const std::vector<const char*> platform_opts = {
            "OC1", "KR", "NA1", "EUW1", "EUN1", "JP1", "BR1", "LA1",
            "LA2", "TR1", "RU", "SG2", "PH2", "TH2", "TW2", "VN2"
        };
        static const std::vector<const char*> val_opts = {
            "AP", "BR", "EU", "KR", "LATAM", "NA", "ESPORTS"
        };
        int ind = this->expected_routing_indicator();
        if (ind == REGIONAL_INDICATOR) return regional_opts;
        if (ind == VAL_PLATFORM_INDICATOR) return val_opts;
        return platform_opts;
    }

    const char* BaseForm::routing_type_label() const noexcept {
        int ind = this->expected_routing_indicator();
        if (ind == REGIONAL_INDICATOR) {
            return "Region (AMERICAS, ASIA, EUROPE, SEA)";
        }
        if (ind == VAL_PLATFORM_INDICATOR) {
            return "Valorant Shard (AP, BR, EU, KR, LATAM, NA, ESPORTS)";
        }
        return "Platform Shard (OC1, KR, NA1, EUW1, ...)";
    }

    bool BaseForm::validate_routing_input(const std::string& raw_val, std::string* err_msg, std::string* suggested_fix) const {
        if (err_msg) err_msg->clear();
        if (suggested_fix) suggested_fix->clear();

        int expected_ind = this->expected_routing_indicator();
        std::vector<std::string> tokens;
        std::stringstream ss(raw_val);
        std::string item;
        while (std::getline(ss, item, ',')) {
            std::string trimmed = trim_str(item);
            if (!trimmed.empty()) {
                tokens.push_back(trimmed);
            }
        }
        if (tokens.empty()) {
            return false;
        }

        for (const auto& tok : tokens) {
            try {
                riotcpp::args::str_to_routing_for_indicator(tok, expected_ind);
            } catch (const std::invalid_argument& ex) {
                if (err_msg) {
                    *err_msg = ex.what();
                }
                if (suggested_fix) {
                    if (expected_ind == REGIONAL_INDICATOR) {
                        auto pf = riotcpp::args::str_to_platform(tok);
                        if (pf.has_value()) {
                            *suggested_fix = riotcpp::args::regional_to_str(riotcpp::args::platform_to_regional(pf.value()));
                        }
                    } else if (expected_ind == PLATFORM_INDICATOR) {
                        auto reg = riotcpp::args::str_to_regional(tok);
                        if (reg.has_value()) {
                            switch (reg.value()) {
                                case riotcpp::args::regional::SEA: *suggested_fix = "OC1"; break;
                                case riotcpp::args::regional::ASIA: *suggested_fix = "KR"; break;
                                case riotcpp::args::regional::AMERICAS: *suggested_fix = "NA1"; break;
                                case riotcpp::args::regional::EUROPE: *suggested_fix = "EUW1"; break;
                            }
                        }
                    }
                }
                return false;
            }
        }
        return true;
    }

    bool BaseForm::validate_param_input(std::size_t ind, const std::string& raw_val, std::string* err_msg, std::string* suggested_fix) const {
        if (err_msg) err_msg->clear();
        if (suggested_fix) suggested_fix->clear();
        if (ind == 0) {
            return this->validate_routing_input(raw_val, err_msg, suggested_fix);
        }
        if (raw_val.empty()) {
            return false;
        }
        if (ind < this->_param_names.size() && (this->_game_name == "League of Legends" || this->_game_ind == 1) &&
            (this->_endpoint == "League" || this->_endpoint_ind == 3)) {
            std::string pname(this->_param_names[ind].name);
            std::stringstream ss(raw_val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                std::string tok = trim_str(item);
                if (tok.empty()) continue;
                if (pname == "Queue") {
                    if (!riotcpp::args::valid_queue(tok)) {
                        if (err_msg) {
                            *err_msg = "Invalid Queue '" + tok + "' (expected RANKED_SOLO_5x5, RANKED_FLEX_SR, or RANKED_FLEX_TT)";
                        }
                        if (suggested_fix) {
                            *suggested_fix = "RANKED_SOLO_5x5";
                        }
                        return false;
                    }
                } else if (pname == "Tier") {
                    std::string up = to_upper_ascii(tok);
                    bool exp_tier = (this->_endpoint_method == "Experimental" || this->_endpoint_method_ind == 6) &&
                                    (up == "CHALLENGER" || up == "GRANDMASTER" || up == "MASTER");
                    if (!riotcpp::args::valid_tier(tok) && !exp_tier) {
                        if (err_msg) {
                            *err_msg = "Invalid Tier '" + tok + "' (expected IRON, BRONZE, SILVER, GOLD, PLATINUM, EMERALD, or DIAMOND)";
                        }
                        return false;
                    }
                } else if (pname == "Division") {
                    if (!riotcpp::args::valid_division(tok)) {
                        if (err_msg) {
                            *err_msg = "Invalid Division '" + tok + "' (expected I, II, III, or IV)";
                        }
                        return false;
                    }
                }
            }
        }
        return true;
    }

    std::string BaseForm::sanitize_routing_input(const std::string& raw_val) const {
        if (raw_val.find(',') == std::string::npos) {
            return to_upper_ascii(trim_str(raw_val));
        }
        std::stringstream ss(raw_val);
        std::string item;
        std::string out;
        while (std::getline(ss, item, ',')) {
            std::string tok = to_upper_ascii(trim_str(item));
            if (!tok.empty()) {
                if (!out.empty()) out += ", ";
                out += tok;
            }
        }
        return out;
    }

    std::string BaseForm::sanitize_param_input(std::size_t ind, const std::string& raw_val) const {
        if (ind == 0) {
            return this->sanitize_routing_input(raw_val);
        }
        if (ind < this->_param_names.size()) {
            std::string pname(this->_param_names[ind].name);
            if (pname == "Queue") {
                std::stringstream ss(raw_val);
                std::string item;
                std::string out;
                while (std::getline(ss, item, ',')) {
                    std::string trimmed = trim_str(item);
                    if (trimmed.empty()) continue;
                    auto q = riotcpp::args::str_to_queue(trimmed);
                    std::string tok = q.has_value() ? riotcpp::args::queue_to_str(q.value()) : to_upper_ascii(trimmed);
                    if (!out.empty()) out += ", ";
                    out += tok;
                }
                return out.empty() ? trim_str(raw_val) : out;
            }
            if (pname == "Tier" || pname == "Division") {
                return this->sanitize_routing_input(raw_val);
            }
        }
        return raw_val;
    }

    void QUERY_FORM::render_title() {
        ImGui::Text((this->_game_name + " | " + this->_endpoint + " | " + this->_endpoint_method).data());
    }

    // Render Endpoint Submission Forms
    void QUERY_FORM::render_form() {
        
        // setup style
        static float height_l = ImGui::GetStyle().ItemSpacing.y;
        ImGui::BeginChild(this->_ID.data(), ImVec2(ImGui::GetContentRegionAvail().x, this->form_height + 2 * height_l), true, ImGuiWindowFlags_ChildWindow);
        this->render_title();
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("My Button").x);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        if (ImGui::Button("Close")) {
            this->remove_form = true;
        };


        this->render_required(this->form_execute);

        if (this->_accepts_optional) {
            this->render_optionals(this->form_execute);
        }

        if (!this->form_execute) {
            if (ImGui::Button("Send")) {
                bool all_valid = true;
                for (int i = 0; i < this->_n_params; i++) {
                    all_valid &= this->validate_param_input(static_cast<std::size_t>(i), this->_params_in_form[i].param);
                }
                if (all_valid) {
                    this->submit_request();
                }
            }
        }
        this->form_height = ImGui::GetCursorPosY();
        ImGui::EndChild();
    };	

    void QUERY_FORM::render_singular_field(const int i, bool already_sent) {
        ImGui::InputText(this->_param_names[i], this->_params_in_form[i], 256, already_sent ? ImGuiInputTextFlags_None : this->_type_ordering[i]);
        if (!already_sent) {
            if (i == 0) {
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.65f, 1.0f), "Expected: %s", this->routing_type_label());
                ImGui::SameLine();
                std::string pick_btn = "Pick Routing##QF" + this->_ID;
                std::string pick_pop = "QFRoutingPickerPopup" + this->_ID;
                if (ImGui::SmallButton(pick_btn.c_str())) {
                    ImGui::OpenPopup(pick_pop.c_str());
                }
                if (ImGui::BeginPopup(pick_pop.c_str())) {
                    for (const char* opt : this->valid_routing_options()) {
                        if (ImGui::Selectable(opt)) {
                            this->_params_in_form[0] = opt;
                        }
                    }
                    ImGui::EndPopup();
                }
            }
            if (strlen(this->_params_in_form[i].param) > 0) {
                std::string err_msg;
                std::string suggested_fix;
                if (!this->validate_param_input(static_cast<std::size_t>(i), this->_params_in_form[i].param, &err_msg, &suggested_fix)) {
                    ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "%s", err_msg.c_str());
                    if (!suggested_fix.empty()) {
                        ImGui::SameLine();
                        if (ImGui::SmallButton(("Use " + suggested_fix + "##QFFix" + std::to_string(i)).c_str())) {
                            this->_params_in_form[i] = suggested_fix.c_str();
                        }
                    }
                }
            }
        }
    }

    void QUERY_FORM::render_required(bool already_sent) {
        if (already_sent) {
            ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
        }

        static float text_width = ImGui::GetContentRegionAvail().x * INPUT_TEXT_FRAC;
        ImGui::PushItemWidth(text_width);

        for (int i = 0; i < this->_n_params; i++) {
            this->render_singular_field(i, already_sent);
        }

        ImGui::PopItemWidth();

        if (already_sent) {
            ImGui::PopStyleColor();
        }
    }


    void QUERY_FORM::render_optionals(bool already_sent) {

        if (!this->_accepts_optional) {
            throw std::logic_error("QUERY_FORM::render_optional should never be called for endpoints with no optional arguements");
        }

        static char id[5] = "x## ";
        static int opt_index = 0;
        static float text_width = ImGui::GetContentRegionMax().x * OPT_INPUT_TEXT_FRAC;
        
        // change style for when the request has already been executed
        if (already_sent) {
            ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
        }
        ImGui::PushItemWidth(text_width);

        // render input forms
        for (int i = 0; i < this->_optionals_to_send.size(); i++) {
            if (this->_optionals_to_send[i] != 1) {
                continue;
            }
            ImGui::InputText(this->_optional_names[i], this->_optional_inputs[i], 256, already_sent ? ImGuiInputTextFlags_ReadOnly : this->_optional_types[i]);

            // display delete button
            if (already_sent) {
                continue;
            }

            ImGui::SameLine(ImGui::GetContentRegionAvail().x * INPUT_TEXT_FRAC - 2 * ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("x").x);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
            id[3] = i;
        
            if (ImGui::Button(id)) {
                this->_optionals_to_send[i] = 0;
                this->_n_used_optional_p1 -= 1;
                opt_index = i;
            };
        }

        // remove style if already executed
        if (already_sent) {
            ImGui::PopStyleColor();
        }
        ImGui::PopItemWidth();

        float drop_down_width = (0.3f * ImGui::GetContentRegionAvail().x);
        ImGui::SetNextItemWidth(drop_down_width);
        if (opt_index == -1) {
            ;
        }
        else if (ImGui::BeginCombo("##OPTIONAL", this->_optional_names[opt_index], ImGuiComboFlags_None))
        {
            for (int n = 0; n < this->_optional_names.size(); n++)
            {
                if (this->_optionals_to_send[n] == 0) {

                    const bool is_selected = (opt_index == n);
                    if (ImGui::Selectable(this->_optional_names[n], is_selected)) {
                        opt_index = n;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }

        // Render add optional button besides dropdown
        const static char button_name[] = "+";
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * INPUT_TEXT_FRAC - 2 * ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize(button_name).x);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

        if (ImGui::Button(button_name)) {
            if (this->_optionals_to_send[opt_index] == 0) {this->_n_used_optional_p1 += 1;};
            this->_optionals_to_send[opt_index] = 1;
            opt_index = find_next_focus(this->_optionals_to_send);
        }

    }

    request QUERY_FORM::construct_request() {
        request new_task; new_task._game = this->_game_ind; new_task._endpoint = this->_endpoint_ind; new_task._endpoint_method = this->_endpoint_method_ind;
        for (int i = 0; i < this->_n_params; i++) { // add inputs into the request structure
            std::string sanitized = this->sanitize_param_input(static_cast<std::size_t>(i), this->_params_in_form[i].param);
            this->_params_in_form[i] = sanitized.c_str();
            new_task.params.push_back(this->_params_in_form[i]);
        }
        if (this->_accepts_optional) { // add optional inputs into the request structure
            for (int i = 0; i < this->_optional_names.size(); i++) {
                if (this->_optionals_to_send[i]) { // check which inputs to send
                    new_task.optional_names.push_back(this->_optional_names[i]);
                    new_task.optional_inputs.push_back(this->_optional_inputs[i]);
                } else {
                    new_task.optional_names.push_back(""); // giving empty string makes the underlying client not send the request
                    new_task.optional_inputs.push_back("0");
                }
            }
        }
        return new_task;
    }

    void QUERY_FORM::submit_request() {
        request new_task = this->construct_request();
        this->sender->add_request(new_task);
        this->form_execute = true;
    }
    template<std::size_t N>
    ImVec2 LinkedForm<N>::default_size = ImVec2(0, 0);

    void BatchForm::render_form() {
        if (this->lifecycle == BatchLifecycleState::RUNNING) {
            if (this->active_batch && this->active_batch->is_completed()) {
                bool has_failed = false;
                for (const auto& form : this->forms) {
                    if (form && form->get_tracker()) {
                        if (form->get_tracker()->state.load() == NodeExecState::FAILED) {
                            has_failed = true;
                            break;
                        }
                    }
                }
                this->lifecycle = has_failed ? BatchLifecycleState::FAILED : BatchLifecycleState::COMPLETED;
            }
        }

        this->newFormButton();

        // Templates Menu & Pipeline Controls (Row 2)
        if (ImGui::Button("Templates")) {
            ImGui::OpenPopup("TemplatesPopup");
        }
        if (ImGui::BeginPopup("TemplatesPopup")) {
            if (ImGui::Selectable("Starter: Match History & Timelines")) {
                TemplateManager::GetInstance().LoadTemplateFromString(
                    TemplateManager::GetInstance().GetStarterMatchHistoryTimelinesJson(),
                    *this
                );
                this->show_quick_fill = true;
            }
            if (ImGui::Selectable("Starter: Match Participants Expansion")) {
                TemplateManager::GetInstance().LoadTemplateFromString(
                    TemplateManager::GetInstance().GetStarterParticipantExpansionJson(),
                    *this
                );
                this->show_quick_fill = true;
            }
            auto sync_quick_fill_from_root = [this]() -> bool {
                for (const auto& f : this->forms) {
                    if (f && !f->check_dependent()) {
                        if (f->get_param_count() >= 1) {
                            std::string r = f->get_param_value(0);
                            if (!r.empty()) {
                                strncpy(this->quick_fill_routing, r.c_str(), sizeof(this->quick_fill_routing) - 1);
                                this->quick_fill_routing[sizeof(this->quick_fill_routing) - 1] = '\0';
                            }
                        }
                        if (f->get_param_count() >= 3) {
                            std::string gn = f->get_param_value(1);
                            std::string tl = f->get_param_value(2);
                            if (!gn.empty()) {
                                strncpy(this->quick_fill_game_name, gn.c_str(), sizeof(this->quick_fill_game_name) - 1);
                                this->quick_fill_game_name[sizeof(this->quick_fill_game_name) - 1] = '\0';
                            }
                            if (!tl.empty()) {
                                strncpy(this->quick_fill_tagline, tl.c_str(), sizeof(this->quick_fill_tagline) - 1);
                                this->quick_fill_tagline[sizeof(this->quick_fill_tagline) - 1] = '\0';
                            }
                            return true;
                        }
                        return false;
                    }
                }
                return false;
            };
            if (ImGui::Selectable("Starter: Challenger Database Builder")) {
                TemplateManager::GetInstance().LoadTemplateFromString(
                    TemplateManager::GetInstance().GetStarterChallengerDatabaseJson(),
                    *this
                );
                sync_quick_fill_from_root();
            }
            ImGui::Separator();
            auto tdir = FileManager::GetInstance().GetWorkspacePaths().templates_dir;
            auto available = TemplateManager::GetInstance().ListAvailableTemplates(tdir);
            for (const auto& tmpl_pair : available) {
                if (ImGui::Selectable(tmpl_pair.first.c_str())) {
                    TemplateManager::GetInstance().LoadTemplate(tmpl_pair.second.string(), *this);
                    this->show_quick_fill = sync_quick_fill_from_root();
                }
            }
            if (!available.empty()) {
                ImGui::Separator();
            }
            if (ImGui::Selectable("Load Template File...")) {
                std::string file_path = FileManager::OpenFileDialog("Template JSON (*.json)", "json");
                if (!file_path.empty()) {
                    TemplateManager::GetInstance().LoadTemplate(file_path, *this);
                    this->show_quick_fill = sync_quick_fill_from_root();
                }
            }
            if (ImGui::Selectable("Save Current Template...")) {
                this->show_save_modal = true;
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Save Template")) {
            this->show_save_modal = true;
        }
        ImGui::SameLine();

        if (ImGui::Button("Quick Fill")) {
            for (const auto& f : this->forms) {
                if (f && !f->check_dependent() && f->get_param_count() >= 1) {
                    std::string r = f->get_param_value(0);
                    if (!r.empty()) {
                        strncpy(this->quick_fill_routing, r.c_str(), sizeof(this->quick_fill_routing) - 1);
                        this->quick_fill_routing[sizeof(this->quick_fill_routing) - 1] = '\0';
                    }
                    break;
                }
            }
            this->show_quick_fill = true;
        }
        ImGui::SameLine();

        if (this->lifecycle == BatchLifecycleState::RUNNING) {
            ImGui::BeginDisabled();
            ImGui::Button("Running Pipeline...");
            ImGui::EndDisabled();
        } else if (this->lifecycle == BatchLifecycleState::COMPLETED || this->lifecycle == BatchLifecycleState::FAILED) {
            if (ImGui::Button("Reset Pipeline")) {
                this->reset_pipeline();
            }
            ImGui::SameLine();
            if (ImGui::Button("Re-run")) {
                this->reset_pipeline();
                this->execute_request();
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.92f, 0.68f, 0.01f, 0.40f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.68f, 0.01f, 0.70f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.92f, 0.68f, 0.01f, 1.0f));
            if (ImGui::Button("Execute")) {
                this->execute_request();
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Canvas")) {
            this->clear_canvas();
        }

        // Render Quick Fill Modal
        if (this->show_quick_fill) {
            ImGui::OpenPopup("Quick Fill Starting Parameters");
        }
        if (ImGui::BeginPopupModal("Quick Fill Starting Parameters", &this->show_quick_fill, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter Starting Account Parameters for Root Request:");
            ImGui::Spacing();

            ImGui::Text("Routing Region:");
            ImGui::SameLine(130.0f);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("##Routing", this->quick_fill_routing, sizeof(this->quick_fill_routing));

            ImGui::Text("Game Name:");
            ImGui::SameLine(130.0f);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("##Game Name", this->quick_fill_game_name, sizeof(this->quick_fill_game_name));

            ImGui::Text("Tagline:");
            ImGui::SameLine(130.0f);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("##Tagline", this->quick_fill_tagline, sizeof(this->quick_fill_tagline));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Apply")) {
                TemplateManager::GetInstance().QuickFillRootParameters(
                    *this,
                    std::string(this->quick_fill_routing),
                    std::string(this->quick_fill_game_name),
                    std::string(this->quick_fill_tagline)
                );
                this->show_quick_fill = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                this->show_quick_fill = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // Render Save Template Modal
        if (this->show_save_modal) {
            ImGui::OpenPopup("Save Pipeline Template");
        }
        if (ImGui::BeginPopupModal("Save Pipeline Template", &this->show_save_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Save Pipeline Graph Template");
            ImGui::Spacing();

            ImGui::Text("Template Name:");
            ImGui::SetNextItemWidth(300.0f);
            ImGui::InputText("##SaveTemplateName", this->save_template_name, sizeof(this->save_template_name));

            ImGui::Text("Description:");
            ImGui::SetNextItemWidth(300.0f);
            ImGui::InputText("##SaveTemplateDesc", this->save_template_desc, sizeof(this->save_template_desc));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Save to Workspace")) {
                auto tdir = FileManager::GetInstance().GetWorkspacePaths().templates_dir;
                std::string filename = std::string(this->save_template_name);
                for (char& c : filename) {
                    if (c == ' ' || c == '/' || c == '\\' || c == ':') c = '_';
                }
                std::string full_path = (tdir / (filename + ".json")).string();
                TemplateManager::GetInstance().SaveTemplate(full_path, this->save_template_name, this->save_template_desc, *this);
                this->show_save_modal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Browse & Save...")) {
                std::string file_path = FileManager::SaveFileDialog("pipeline_template.json", "Template JSON (*.json)", "json");
                if (!file_path.empty()) {
                    TemplateManager::GetInstance().SaveTemplate(file_path, this->save_template_name, this->save_template_desc, *this);
                    this->show_save_modal = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                this->show_save_modal = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (this->first_iter) {
            this->first_iter = false;
            ImVec2 new_size = ImGui::GetContentRegionAvail();
            LinkedForm<1>::set_default_size(new_size);
            LinkedForm<2>::set_default_size(new_size);
            LinkedForm<3>::set_default_size(new_size);
            LinkedForm<4>::set_default_size(new_size);
            LinkedForm<5>::set_default_size(new_size);
        }
        ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 80.0f));
        if (this->linking_mode) {
            for (std::shared_ptr<LinkedInterface>& form : this->forms) {
                if (form->render_form(true)) {
                    this->parent = form;
                    this->child->insert_parent(form);
                    form->insert_child(this->child);
                    this->child->configure();
                    this->linking_mode = false;
                };
            }
        } else {
            for (std::shared_ptr<LinkedInterface>& form : this->forms) {
                if (form->render_form()) {
                    this->child = form;
                    this->linking_mode = true;
                };
            }
        }

        // Deferred node removal pass
        for (auto it = this->forms.begin(); it != this->forms.end(); ) {
            if ((*it)->check_remove()) {
                if (this->child == *it) { this->child = nullptr; this->linking_mode = false; }
                if (this->parent == *it) { this->parent = nullptr; this->linking_mode = false; }
                (*it)->disconnect_all();
                it = this->forms.erase(it);
            } else {
                ++it;
            }
        }
    }

    void BatchForm::newFormButton() {
        static const char* GAMES_[] = { "Riot", "League of Legends", "TeamfightTactics", "Legends of Runeterra", "Valorant"};

        static const char GAME_ENDPOINTS_[5][9][18] = {
        { "Account" },
        { "Champion Mastery", "Champion Rotation", "Clash", "League", "Challenges", "Status", "Match", "Summoner", "Spectator" },
        { "League", "Match", "Status", "Summoner" },
        { "Match", "Ranked", "Status" },
        { "Content", "Match", "Ranked", "Status" }
        };


        static const char ENDPOINT_METHODS_[5][9][7][24] = {
        {{"By Puuid", "By Riot ID", "By Game"}},
        {{ "By Summoner ID", "By Summoner By Champion", "Top By Summoner", "Scores By Summoner" },
        { "Rotation" },
        {"By Summoner ID", "By Team", "Tourament By Team", "By Tournament"},
        {"Challenger", "Grandmaster", "Master", "By Summoner ID", "By League ID", "Specific League", "Experimental"},
        {"Configuration", "Percentiles", "Challenge Configuration", "Challenge Leaderboard", "Challenge Percentiles", "By Puuid"},
        {"v4 (recommended)", "v3"},
        {"By Match ID", "Timeline", "By Puuid"},
        {"By RSO Puuid", "By Account ID", "By Name", "By Puuid", "By Summoner ID"},
        {"By Summoner ID", "Featured Games"}
        },
        {{ "Challenger", "Grandmaster", "Master", "By Summoner ID", "By League ID", "Queue Top", "By Tier Division"},
            { "By Puuid", "By Match ID"},
            {"v1"},
            {"By Account", "By Name", "By Puuid", "By Summoner ID"}
        },
        {{ "By Puuid", "By Match ID"},
            { "Leaderboards"},
            {"v1"}
        },
        {{ "Content"},
            { "By Match ID", "By Puuid", "By  Queue"},
            {"By Act"},
            {"Status"}
        }
        };

        static const int Game_endpoint_length[5] = {1, 9, 4, 3, 4};

        static const int endpoint_lengths[5][9] = {
            {3},
            {4, 1, 4, 7, 6, 2, 3, 5, 2},
            {7, 2, 1, 3},
            {2, 1, 1},
            {1, 3, 1, 1}
        };

        static int last_iter[] = {0, 0, 0};

        static int game = 0;
        static int endpoint = 0;
        static int endpoint_method = 0;

        if (game != last_iter[0]) {
            last_iter[0] = game;
            endpoint = 0;
            endpoint_method = 0;
            last_iter[1] = 0;
            last_iter[2] = 0;
        }
        else if (last_iter[1] != endpoint) {
            last_iter[1] = endpoint;
            endpoint_method = 0;
            last_iter[2] = 0;
        }

        float drop_down_width = std::min(135.0f, (ImGui::GetContentRegionAvail().x - 60.0f) / 3.0f);
        if (drop_down_width < 100.0f) drop_down_width = 100.0f;
        ImGui::SetNextItemWidth(drop_down_width);
        if (ImGui::BeginCombo("##input1", GAMES_[game], ImGuiComboFlags_None))
        {
            for (int n = 0; n < IM_ARRAYSIZE(GAMES_); n++)
            {
                const bool is_selected = (game == n);
                if (ImGui::Selectable(GAMES_[n], is_selected))
                    game = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(drop_down_width);
        if (ImGui::BeginCombo("##input2", GAME_ENDPOINTS_[game][endpoint]))
        {
            for (int n = 0; n < Game_endpoint_length[game]; n++)
            {
                const bool is_selected = (endpoint == n);
                if (ImGui::Selectable(GAME_ENDPOINTS_[game][n], is_selected))
                    endpoint = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(drop_down_width);
        if (ImGui::BeginCombo("##input3", ENDPOINT_METHODS_[game][endpoint][endpoint_method]))
        {
            for (int n = 0; n < endpoint_lengths[game][endpoint]; n++)
            {
                const bool is_selected = (endpoint_method == n);
                if (ImGui::Selectable(ENDPOINT_METHODS_[game][endpoint][n], is_selected))
                    endpoint_method = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();

        if (ImGui::Button("Add")) {
            this->pushNewForm(game, endpoint, endpoint_method);
        }
    }
    
    void BatchForm::pushNewForm(const int game, const int endpoint, const int endpoint_method) {
        this->forms.push_back(LinkedInterface::make_linked(game, endpoint, endpoint_method));
        this->forms.back()->set_id(this->current_ID);
        this->current_ID++;
    }

    void BatchForm::add_form(std::shared_ptr<LinkedInterface> form) {
        if (form) {
            this->forms.push_back(form);
            if (form->get_id() >= this->current_ID) {
                this->current_ID = form->get_id() + 1;
            }
        }
    }
    
    std::shared_ptr<BatchRequest> BatchForm::construct_request() {
        for (auto& form : this->forms) {
            form->construct_base();
        }

        for (auto& form : this->forms) {
            form->link_final_requests();
        }

        std::vector<std::shared_ptr<RequestNode>> initial_requests = std::accumulate(this->forms.begin(), this->forms.end(), std::vector<std::shared_ptr<RequestNode>>(), 
                [](std::vector<std::shared_ptr<RequestNode>> acc, std::shared_ptr<LinkedInterface> frm) {
                    if (!frm->check_dependent()) {
                        acc.push_back(frm->get_base_request());
                    }
                    return acc;
                }
                );
        
        return std::make_shared<BatchRequest>(initial_requests);
    }

    void BatchForm::clear_canvas() {
        for (auto& form : this->forms) {
            if (form) {
                form->disconnect_all();
                form->detach_base();
            }
        }
        this->forms.clear();
        this->current_ID = 0;
        this->linking_mode = false;
        this->parent = nullptr;
        this->child = nullptr;
        this->lifecycle = BatchLifecycleState::IDLE;
        this->active_batch = nullptr;
    }

    void BatchForm::reset_pipeline() {
        for (auto& form : this->forms) {
            if (form) {
                form->reset_tracker();
                form->detach_base();
            }
        }
        this->lifecycle = BatchLifecycleState::IDLE;
        this->active_batch = nullptr;
    }

    void BatchForm::execute_request() {
        if (this->lifecycle == BatchLifecycleState::RUNNING) {
            std::cout << "[BatchForm] Pipeline is already running." << std::endl;
            return;
        }

        bool ready = true;
        for (const auto& form : this->forms) {
            if (form) {
                ready &= form->check_ready();
            }
        }
        if (!ready) {
            std::cout << "[BatchForm] Not all forms are ready for execution." << std::endl;
            return;
        }

        std::shared_ptr<BatchRequest> final_form = this->construct_request();
        if (!final_form) {
            std::cerr << "[BatchForm] Failed to construct batch request." << std::endl;
            return;
        }

        this->active_batch = final_form;
        this->lifecycle = BatchLifecycleState::RUNNING;

        if (this->sender) {
            std::cout << "[BatchForm] Enqueuing batch request..." << std::endl;
            this->sender->add_batch_request(final_form);
        } else {
            std::cerr << "[BatchForm] ERROR: sender is null on BatchForm!" << std::endl;
        }
        
        // remove forms pointer to original pointer to prevent user interference during requests
        for (auto& frm : this->forms) {
            if (frm) {
                frm->detach_base();
            }
        }
    }


    bool render_json_form(std::vector<KEY_CONT>& keys, const int ind, const float x_align, const float width, bool active_field, bool second) {

        static char _id[] = "##00a";
        _id[2] = (char) (ind + '0');
        _id[4] = second ? 'b' : 'a';

        char counter = '0';
        bool active = false;
        ImVec2 avail = ImGui::GetWindowSize();
        for (KEY_CONT& key : keys) {
            _id[3] = counter;
            ImGui::SetCursorPosX(x_align);
            ImGui::SetNextItemWidth(width - x_align - 0.5 * (x_align - avail.x * 0.25));
            ImGui::InputTextWithHint(_id, "Json Key", key.key, KEY_LENGTH, ImGuiTextFlags_None);
            active |= ImGui::IsItemActive();
            counter++;
        }
        
        // add button
        static char _add_id[] = "Add##0a";
        _add_id[5] = (char) (ind + '0');
        _add_id[6] = second ? 'b' : 'a';
        ImGui::SetCursorPosX(x_align);
        if (ImGui::Button(_add_id)) {
            keys.emplace_back("\0");
            active = true;
        }

        // remove button
        static char _del_id[] = "Remove##0a";
        _del_id[8] = (char)('0' + ind);
        _del_id[9] = second ? 'b' : 'a';
        
        if (keys.size() != 0) {
            ImGui::SameLine();
            if (ImGui::Button(_del_id)) {
                keys.pop_back();
                active = true;
            }
        }

        return active;

    }

    bool render_iter_json(std::vector<KEY_CONT>& before, std::vector<KEY_CONT>& after, char iter_limit[8], const int ind, bool active_field, const float x_align, const float width) {
        bool active = render_json_form(before, ind, x_align, width, active_field, false);

        static char _lim_id[] = "Iteration Limit##0";
        _lim_id[17] = (char)('0' + ind);
        ImGui::SetCursorPosX(x_align);
        ImGui::SetNextItemWidth(width - x_align - 0.5 * (x_align - ImGui::GetWindowSize().x * 0.25));
        ImGui::InputText(_lim_id, iter_limit, 8, ImGuiInputTextFlags_CharsDecimal);
        active |= ImGui::IsItemActive();

        active |= render_json_form(after, ind, x_align, width, active_field, true);
        return active;
    }
}
