#include "QueryForm.h"
#include "imgui.h"


#define INPUT_TEXT_FRAC 2/3
#define OPT_INPUT_TEXT_FRAC 1/3


namespace restfulEz {

    QUERY_FORM::QUERY_FORM(const std::size_t n_params, const int game_ind, const int endpoint_ind, const int endpoint_method_ind, const char* game_name, 
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

        this->_req_in_form = { game_ind, endpoint_ind, endpoint_method_ind };
    };

    // Render Endpoint Submission Forms
    void QUERY_FORM::render_form() {
        float height_l = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild(this->_ID.data(), ImVec2(ImGui::GetContentRegionAvail().x, (height_l * (this->_n_params + this->_n_used_optional_p1 + 2) + 3 * ImGui::GetStyle().ItemSpacing.y)), true, ImGuiWindowFlags_ChildWindow);
        ImGui::Text((this->_game_name + " | " + this->_endpoint + " | " + this->_endpoint_method).data());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("My Button").x);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        if (ImGui::Button("Close")) {
            this->remove_form = true;
        };

        if (!this->form_execute) { this->render_input_fields();}

        else {
            ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
            for (int i = 0; i < this->_n_params; i++) {
                ImGui::InputText(this->_param_names[i], this->_params_in_form[i], 256, ImGuiInputTextFlags_ReadOnly);
            }
            if (this->_accepts_optional) {
                this->render_optionals(true);
            }
            ImGui::PopStyleColor();
        }

        if (!this->form_execute) {
            if (ImGui::Button("Send")) {
                bool empty = false;
                for (int i = 0; i < this->_n_params; i++) {
                    empty |= strlen(this->_params_in_form[i].param) == 0;
                }
                if (!empty) {
                    for (int i = 0; i < this->_n_params; i++) { // add inputs into the request structure
                        this->_req_in_form.params.push_back(this->_params_in_form[i]);
                    }
                    if (this->_accepts_optional) { // add optional inputs into the request structure
                        for (int i = 0; i < this->_optional_names.size(); i++) {
                            if (this->_optionals_to_send[i]) { // check which inputs to send
                                this->_req_in_form.optional_names.push_back(this->_optional_names[i]);
                                this->_req_in_form.optional_inputs.push_back(this->_optional_inputs[i]);
                            } else {
                                this->_req_in_form.optional_names.push_back(""); // giving empty string makes the underlying client not send the request
                                this->_req_in_form.optional_inputs.push_back("0");
                            }
                        }
                    }
                    *this->_execute = true;
                    this->form_execute = true;
                    *this->_client_req = this;
                }
                if (this->_accepts_optional) {
                    this->render_optionals(false);
                }
            }
        }
        else {
            if (this->request_result.size() != 0) {
                ImGui::Text(this->request_result.data());
            }
        }
        ImGui::EndChild();
    };	

    void QUERY_FORM::render_input_fields() {

        static float text_width = ImGui::GetContentRegionAvail().x * INPUT_TEXT_FRAC;
        ImGui::PushItemWidth(text_width);

        for (int i = 0; i < this->_n_params; i++) {

            ImGui::InputText(this->_param_names[i], this->_params_in_form[i], 256, this->_type_ordering[i]);
        }
        ImGui::PopItemWidth();

        if (this->_accepts_optional) {
            this->render_optionals(false);
        }
    }

    static inline int find_next_focus(const std::vector<int>& opts_to_send) {

        int next_focus = 0;
        bool found = false;

        for (const int& opt : opts_to_send) {
            if (opt == 0) {
                found = true;
                break;
            }
            next_focus++;
        }
        if (found) { 
            return next_focus;
        } 
        return -1;

    }

    void QUERY_FORM::render_optionals(bool already_sent) {

        if (!this->_accepts_optional) {
            throw std::logic_error("QUERY_FORM::render_optional should never be called for endpoints with no optional arguements");
        }

        static char id[5] = "x## ";
        static int opt_index = 0;
        static float text_width = ImGui::GetContentRegionAvail().x * OPT_INPUT_TEXT_FRAC;

        if (already_sent) {

            ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
            ImGui::PushItemWidth(text_width);

            for (int i = 0; i < this->_optionals_to_send.size(); i++) {
                if (this->_optionals_to_send[i] == 1) {
                    ImGui::InputText(this->_optional_names[i], this->_optional_inputs[i], 256, ImGuiInputTextFlags_ReadOnly);
                }
            }
            ImGui::PopItemWidth();
            ImGui::PopStyleColor();

        } else {
            for (int i = 0; i < this->_optionals_to_send.size(); i++) {
                if (this->_optionals_to_send[i]) {


                    // display input field
                    ImGui::PushItemWidth(text_width);
                    ImGui::InputText(this->_optional_names[i], this->_optional_inputs[i], 256, this->_optional_types[i]);
                    ImGui::PopItemWidth();
                    
                    // display delete button
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x * INPUT_TEXT_FRAC - 2 * ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("x").x);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
                    id[3] = i;

                    if (ImGui::Button(id)) {
                        this->_optionals_to_send[i] = 0;
                        this->_n_used_optional_p1 -= 1;
                        opt_index = i;
                    };
                }
            }
        }
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

}
