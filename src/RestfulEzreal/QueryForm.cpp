#include "QueryForm.h"
#include "BatchRequests.h"
#include "RestfulEzreal.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <memory>
#include <string_view>


#define INPUT_TEXT_FRAC 2/3
#define OPT_INPUT_TEXT_FRAC 1/3


namespace restfulEz {

    bool QUERY_FORM::operator==(const QUERY_FORM& other_form) {
        return this->_ID == other_form._ID;
    }

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
        if (this->recalculate_height) {
            this->recalculate_height = false;
        }
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
                this->recalculate_height = true;
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
            this->recalculate_height = true;
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


    static inline void render_noniterative_form(param_dependence_info& link_description, std::size_t* next_index) {

        static char _par_id[] = "Select Parent##0";
        _par_id[15] = link_description.param_index;

        if (ImGui::Button(_par_id)) {
            *next_index = link_description.param_index;             // write in child form
        }
        static char _id[] = "##01";
        _id[2] = (char) link_description.param_index + '0';
        static int counter = 0;
        ImGui::Text("Json Index Keys");
        for (int i = 0; i < link_description.json_keys.size(); i++) {
            _id[3] = (char)counter + '0';
            ImGui::InputText(_id, link_description.json_keys[i], 256, ImGuiInputTextFlags_None);
            counter += 1;
        }
        counter = 0;

        // add key button
        static char _add_id[] = "Add Key##0";
        _add_id[9] = link_description.param_index;
        if (ImGui::Button(_add_id)) {
            link_description.json_keys.push_back("");
        }

        // remove key button
        static char _rem_id[] = "Remove Key##0";
        _rem_id[12] = link_description.param_index;
        if (link_description.json_keys.size() != 0) {
            if (ImGui::Button(_rem_id)) {
                link_description.json_keys.pop_back();
            }
        }
    }

    static inline void render_iterative_form(param_dependence_info& link_description, PARAM_CONT& iter_index, PARAM_CONT& iter_limit, std::size_t* next_ind) {
        // ImGui Ids to avoid conflicts
        static char _ind_id[] = "Json Array Index##0";
        static char _lim_id[] = "Iteration Limit##0";
        // change id to avoid conflicts
        _ind_id[18] = link_description.param_index;
        _ind_id[17] = link_description.param_index;
        // display input text fields
        ImGui::InputText(_ind_id, iter_index.param, 256, ImGuiInputTextFlags_CharsDecimal);
        ImGui::InputText(_lim_id, iter_limit.param, 256, ImGuiInputTextFlags_CharsDecimal);
        // display input fields for the json keys
        render_noniterative_form(link_description, next_ind);
    }

    void LinkedForm::render_linked_fields(const int i) {

        static char itera[] = "Iterative##1";
        itera[11] = (char) i + '0';
        ImGui::SameLine();

        ImGui::Checkbox(itera, &this->link_descriptions[i].iterative);

        if (this->link_descriptions[i].iterative) {
            render_iterative_form(this->link_descriptions[i], this->iter_index[i], this->iter_limit[i], &this->next_index); 
        } else {
            render_noniterative_form(this->link_descriptions[i], &this->next_index);
        }
    }

    void LinkedForm::render_required(bool already_sent) {
        if (already_sent) {
            ImVec4 disabled_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled_color);
        }
        static float text_width = ImGui::GetContentRegionAvail().x * INPUT_TEXT_FRAC;
        ImGui::PushItemWidth(text_width);
        static char check_link[] = "Link##1";

        for (int i = 0; i < this->get_n_params(); i++) {
            check_link[6] = i;
            ImGui::Checkbox(check_link, &this->inputs_from_parents[i]);
            if (!this->inputs_from_parents[i]) {
                this->render_singular_field(i, already_sent);
            } else {
                this->render_linked_fields(i);
            }
        }
        ImGui::PopItemWidth();
        if (already_sent) {
            ImGui::PopStyleColor();
        }
    }

    bool LinkedForm::render_form_return() {
        this->render_form();

        return this->next_index != -1;
    }

    bool LinkedForm::render_link_mode() {
        static float height_l = ImGui::GetStyle().ItemSpacing.y;
        ImGui::BeginChild(this->get_ID().data(), ImVec2(ImGui::GetContentRegionAvail().x, this->form_height + 2 * height_l), true, ImGuiWindowFlags_ChildWindow);
        this->render_title();
        if (this->next_index != -1) {
            ImGui::Text("Currently Selecting Parent...");
        } 
        else if (ImGui::Button("Select as Parent")) {
            ImGui::EndChild();
            return true;
        }
        ImGui::EndChild();
        return false;
    }

    void LinkedForm::complete_link(std::shared_ptr<LinkedForm> next_parent) {
        if (this->next_index == -1) {
            throw std::logic_error("parent added before index recorded");
        }
        this->parents[this->next_index] = next_parent;
        this->next_index = -1;
    }

    void LinkedForm::cancel_link() {
        this->next_index = -1;
    }

    void LinkedForm::insert_child(std::shared_ptr<LinkedForm> child) {
        this->children.push_back(child);
    }

    void LinkedForm::remove_parent(const LinkedForm& parent) {

        for (std::shared_ptr<LinkedForm>& form_ptr : this->parents) {
            if (form_ptr != nullptr && *form_ptr == parent) {
                form_ptr = nullptr;
            }
        }
    }

    void LinkedForm::remove_child(const LinkedForm& child) {

        std::vector<int> to_remove;

        for (int i = 0; i < this->children.size(); i++) {
            if (*this->children[i] == child) {
                to_remove.push_back(i);
            }
        }
        int counter = 0;
        for (auto& ind : to_remove) {
            this->children.erase(this->children.begin() + ind - counter);
            counter++;
        }
    }

    static inline bool check_iterative(const std::vector<param_dependence_info>& links) {
        for (auto& link : links) {
            if (link.iterative) {
                return true;
            }
        }
        return false;
    }

    bool LinkedForm::is_iterative() {

        if (this->iterative_final == 1) {
            return false; // already computed by another form NOT ITERATIVE
        } 
        else if (this->iterative_final == 2) {
            return true; // already computer by anoter form IS ITERATIVE
        }

        // if this request contains an iterative link then it must be iterative
        bool contains_iterative = check_iterative(this->link_descriptions);
        if (contains_iterative) {
            this->iterative_final = 2;
            return true;
        }
        // if this request has an iterative ancenstor it must also be iterative
        for (const auto& par : this->parents) {
            if (!par) { //  might be nullptr
                continue; 
            }
            if (par->is_iterative()) {
                this->iterative_final = 2;
                return true; // if we find an iterative ancestor we are done
            }
        }
        this->iterative_final = 1;
        return false;
    }

    void LinkedForm::construct_request_heap() {
        request new_request = this->construct_request();
        std::vector<std::size_t> required_pars;

        for (auto& parent : this->parents) {
            if (parent) {
                required_pars.push_back(1);
            }  else {
                required_pars.push_back(0);
            }
        }
        if (this->is_iterative()) {
            this->final_request = std::make_shared<Iterative_Request>(new_request, required_pars);
        } else {
            this->final_request = std::make_shared<Linked_Request>(new_request, required_pars);
        }
    }

    void LinkedForm::add_child_info(std::shared_ptr<Linked_Request> child, param_dependence_info& link_info) {
        
        // check if previous dependence already exists, if so add additional dependence info (rare case but possible)
        for (request_link& link_cont : this->final_request->child_links) {
            if (child.get() == link_cont.request.get()) {
                link_cont.dependence_information.push_back(link_info);
                return;
            }
        };

        // construct request link and add to child links
        this->final_request->child_links.emplace_back(child, std::vector<param_dependence_info>({link_info}));
    }

    void LinkedForm::inform_the_parents() {

        for (int i = 0; i < this->parents.size(); i++) { // should use zip iterator
            if (this->link_descriptions[i].iterative) {
                this->link_descriptions[i].iter_index = atoi(this->iter_index[i].param);
                this->link_descriptions[i].iter_limit = atoi(this->iter_limit[i].param);
            }
            if (this->parents[i])  {
                this->parents[i]->add_child_info(this->final_request, this->link_descriptions[i]);
            }
        }
        
    }

    static inline void delete_form(const std::shared_ptr<LinkedForm> to_remove) {

        for (std::shared_ptr<LinkedForm>& child_ptr : to_remove->get_children()) {
            child_ptr->remove_parent(*to_remove);
        }

        for (std::shared_ptr<LinkedForm>& parent_ptr : to_remove->get_parents()) {
            if (parent_ptr) { // parent_ptr can be nullptr
                parent_ptr->remove_child(*parent_ptr);
            }
        }
    }

    void FormGroup::render_group(RestfulEzreal& owner) {

        int counter = 0;
        int to_remove = -1;
        for (std::shared_ptr<LinkedForm>& form : this->forms) {
            if (form->check_remove()) {
                delete_form(form);
                to_remove = counter;
            }
            counter += 1;
        }
        if (to_remove != -1) {
            this->forms.erase(this->forms.begin() + to_remove);
        }
        
        if (this->link_mode) {
            for (auto& linked_form : this->forms) {
                if (linked_form->render_link_mode()) {
                    this->parent = linked_form;
                    this->child->complete_link(linked_form);
                    this->parent->insert_child(this->child);
                    this->link_mode = false;
                }
            }
        } else {
            for (auto& linked_form : this->forms) {
                if (linked_form->render_form_return()) {
                    this->child = linked_form;
                    this->link_mode = true;
                }
            }
        }

        if (ImGui::Button("Send Batch")) {
            this->send_batch_request();
        }
        owner.NewFormButton();
    }

    static std::shared_ptr<Linked_Request> construct_linked(Batch_Request current_progrss, std::shared_ptr<LinkedForm> unprocessed_form) {
        std::shared_ptr<Linked_Request> new_request;

        return new_request;
    }

    static inline bool has_dependencies(const std::vector<std::shared_ptr<LinkedForm>>& parents) {
        for (const auto& par : parents) {
            if (par) {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<Batch_Request> FormGroup::construct_batch() {

        // ensure all forms have constructed unlinked requests
        for (std::shared_ptr<LinkedForm>& form : this->forms) {
            form->construct_request_heap();
        }
        
        // construct links between forms
        for (std::shared_ptr<LinkedForm>& form : this->forms) {
            form->inform_the_parents();
        }

        std::shared_ptr<Batch_Request> final = std::make_shared<Batch_Request>(nullptr, nullptr, nullptr);
        
        // find first request (has no dependencies)
        for (std::shared_ptr<LinkedForm>& form : this->forms) {
            if (!has_dependencies(form->get_parents())) {
                insert_request(final, form->get_final_request());
            }
        }

        if (!final->request_node) { // this is not a definitive check, a request with circular dependencies may pass this.
            throw std::runtime_error("Batch request has circular dependencies");
        }

        return final;
    }

    void FormGroup::send_batch_request() {

        std::shared_ptr<Batch_Request> final_req =  this->construct_batch();

        this->sender->add_batch_request(final_req);
    }
}
