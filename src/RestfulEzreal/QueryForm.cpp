#include "QueryForm.h"
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
                bool empty = false;
                for (int i = 0; i < this->_n_params; i++) {
                    empty |= strlen(this->_params_in_form[i].param) == 0;
                }
                if (!empty) {
                    this->submit_request();
                }
            }
        }
        this->form_height = ImGui::GetCursorPosY();
        ImGui::EndChild();
    };	

    void QUERY_FORM::render_singular_field(const int i, bool already_sent) {
        ImGui::InputText(this->_param_names[i], this->_params_in_form[i], 256, already_sent ? ImGuiInputTextFlags_None : this->_type_ordering[i]);
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
        this->newFormButton();
        ImGui::SameLine();
        if (ImGui::Button("Execute")) {
            this->execute_request();
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

        float drop_down_width = (0.3f * ImGui::GetContentRegionAvail().x);
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

    void BatchForm::execute_request() {
        bool ready = true;
        for (const auto& form : this->forms) {
            ready &= form->check_ready();
        }
        if (!ready) {
            // ot all the forms are ready to be executed
            // DO SOMETHING HERE TO NOT THE USER
            return;
        }

        std::shared_ptr<BatchRequest> final_form = this->construct_request();
        this->sender->add_batch_request(final_form);
        
        // remove forms pointer to original pointer to prevent uses interference during requests
        for (auto& frm : this->forms) {
            frm->detach_base();
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
        _del_id[8] = ind;
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
        _lim_id[17] = ind;
        ImGui::SetCursorPosX(x_align);
        ImGui::SetNextItemWidth(width - x_align - 0.5 * (x_align - ImGui::GetWindowSize().x * 0.25));
        ImGui::InputText(_lim_id, iter_limit, 8, ImGuiInputTextFlags_CharsDecimal);
        active |= ImGui::IsItemActive();

        active |= render_json_form(after, ind, x_align, width, active_field, true);
        return active;
    }
}
