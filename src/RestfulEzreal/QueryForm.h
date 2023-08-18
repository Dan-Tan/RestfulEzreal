#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "Walnut/Application.h"
#include "BatchRequests.h"
#include "RequestQueue.h"
#include <iterator>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <functional>
#include <numeric>
#include <format>
#include <numeric>
#include <string_view>

namespace restfulEz {

    class QUERY_FORM;

    class BaseForm {

        public:
            BaseForm(const std::size_t n_params, const int game_ind, const int endpoint_ind, 
                    const int endpoint_method_ind, const char* game_name, 
                    const char* endpoint, const char* endpoint_method, 
                    std::vector<P_NAME> param_names, std::vector<ImGuiInputTextFlags> type_ordering,
                    bool accepts_optional = false, std::vector<P_NAME> optional_names = {}, 
                    std::vector<PARAM_CONT> optional_inputs = {}, std::vector<ImGuiInputTextFlags> optional_types = {});
            ~BaseForm() = default;
            static BaseForm make_form(const int game_ind, const int endpoint_ind, const int endpoint_method_ind);
            BaseForm(const BaseForm& copy);
            const std::size_t n_params() const {return this->_n_params;}; //lazy

        protected:
            int _game_ind;
            int _endpoint_ind;
            int _endpoint_method_ind;

            std::string _game_name;
            std::string _endpoint;
            std::string _endpoint_method;

            std::vector<P_NAME> _param_names;
            std::vector<PARAM_CONT> _params_in_form = {};
            std::vector<ImGuiInputTextFlags> _type_ordering;

            bool _accepts_optional;
            std::vector<P_NAME> _optional_names;
            std::vector<PARAM_CONT> _optional_inputs;
            std::vector<ImGuiInputTextFlags> _optional_types;
            std::vector<int> _optionals_to_send; 

            int _n_used_optional_p1 = 0;
            std::size_t _n_params;

            std::string _ID; // Distinguish Form from other Query Forms

        private:
            static BaseForm make_form_LOL(const int endpoint_ind, const int endpoint_method_ind); 
            static BaseForm make_form_TFT(const int endpoint_ind, const int endpoint_method_ind); 
            static BaseForm make_form_VAL(const int endpoint_ind, const int endpoint_method_ind); 
            static BaseForm make_form_LOR(const int endpoint_ind, const int endpoint_method_ind); 
    };

    class QUERY_FORM : public BaseForm {
        private:
            std::shared_ptr<RequestSender> sender;
            
            bool form_execute = false;
            bool remove_form = false;


        public:
            QUERY_FORM(const BaseForm& copy) : BaseForm(copy) {};
            QUERY_FORM(const std::size_t n_params, const int game_ind, const int endpoint_ind, 
                    const int endpoint_method_ind, const char* game_name, 
                    const char* endpoint, const char* endpoint_method, 
                    std::vector<P_NAME> param_names, std::vector<ImGuiInputTextFlags> type_ordering,
                    bool accepts_optional = false, std::vector<P_NAME> optional_names = {}, 
                    std::vector<PARAM_CONT> optional_inputs = {}, std::vector<ImGuiInputTextFlags> optional_types = {});
            ~QUERY_FORM() = default;
            void set_id(int new_id) {
                this->_ID = std::string("##FORM") + std::to_string(new_id);
            }

            static QUERY_FORM make_form(const int game_ind, const int endpoint_ind, const int endpoint_method_ind); 

            void set_sender(std::shared_ptr<RequestSender> sender_client) {this->sender = sender_client;}
            void render_form();
            bool check_remove() { return this->remove_form; };
            void calc_height() {this->recalculate_height = true;}

            bool operator==(const QUERY_FORM& other_form);

            std::size_t get_n_params() const {return this->_n_params;};

        protected:
            void render_title();
            void render_singular_field(const int i, bool already_sent);
            request construct_request();
            const std::string& get_ID() {return this->_ID;};
            bool recalculate_height = true;
            float form_height = 0.0f;

        private:
            static QUERY_FORM make_form_LOL(const int endpoint_ind, const int endpoint_method_ind); 
            static QUERY_FORM make_form_TFT(const int endpoint_ind, const int endpoint_method_ind); 
            static QUERY_FORM make_form_VAL(const int endpoint_ind, const int endpoint_method_ind); 
            static QUERY_FORM make_form_LOR(const int endpoint_ind, const int endpoint_method_ind); 

            virtual void render_required(bool already_sent);
            virtual void render_optionals(bool already_sent);
            void submit_request();
    };

    class LinkedInterface {
        // public functions BatchForm needs to be aware of
        public:
            virtual bool render_form(bool linking = false) = 0;
            virtual void set_id(int i) = 0;

            //virtual void insert_parent(std::shared_ptr<LinkedInterface> child) = 0;
            //virtual void insert_child(std::shared_ptr<LinkedInterface> parent) = 0;

            //virtual bool delete_parent(std::shared_ptr<LinkedInterface> parent) = 0;
            //virtual bool delete_child(std::shared_ptr<LinkedInterface> child) = 0;

            static std::shared_ptr<LinkedInterface> make_linked(const int game, const int endpoint, const int endpoint_method);

        private:
            static std::shared_ptr<LinkedInterface> make_linked_LOL(const int game, const int endpoint, const int endpoint_method);
            static std::shared_ptr<LinkedInterface> make_linked_TFT(const int game, const int endpoint, const int endpoint_method);
            static std::shared_ptr<LinkedInterface> make_linked_LOR(const int game, const int endpoint, const int endpoint_method);
            static std::shared_ptr<LinkedInterface> make_linked_VAL(const int game, const int endpoint, const int endpoint_method);
    };
    
    template<std::size_t N>
    class LinkedForm : public LinkedInterface, public BaseForm {
        private:
            // should consider switching to a friendly container class to handle request contants and the owner handles rendering
            ImVec2 windowposition;
            bool movement_lock = false;
            bool dragging = false;
            bool window_initiated  = false;
            // should be the number of parameters - 1
            std::array<LinkedForm*, N-1> parents;
            std::array<std::shared_ptr<iter_access_info>, N-1> iter_info;
            std::array<char[8], N-1> iter_limits;
            std::vector<std::string> display_form;

            std::vector<LinkedForm*> children; // variables size

            bool linked[N] = { 0 };
            bool iterative[N] = { 0 };

            bool configuring = false;

            // let user know if the form is finished
            bool ready = false;
            
            bool linking = false;
            std::size_t next_index = -1;

        public:
            LinkedForm() {};
            LinkedForm(const BaseForm& copy) : BaseForm(copy) {
                this->parents.fill(nullptr);
                for (int i = 0; i < N-1; i++) {
                    this->display_form.emplace_back("");
                    this->iter_info[i] = std::make_shared<iter_access_info>();
                }
                this->display_form.emplace_back("");
            };
            bool render_form(bool linking = false) override;
            void set_id(int new_id) {
                this->_ID = std::string("##FORM") + std::to_string(new_id);
            }
            
            //void insert_parent(std::shared_ptr<LinkedInterface> child) override;
            //void insert_child(std::shared_ptr<LinkedInterface> parent) override;

            //bool delete_parent(std::shared_ptr<LinkedInterface> parent) override;
            //bool delete_child(std::shared_ptr<LinkedInterface> child) override;

        private:
            bool render_linking();
            void render_summary();
            void display_field(const std::size_t ind) const;

            bool render_popup();
            void render_all_fields();
            void render_routing();
            void render_field(const std::size_t ind);
            
            // when the user closes the form we want to update certain fields to 
            // to avoid performing calculations on every frame
            void on_close_popup();
            bool validate_field(const std::size_t ind) const;

            // update display form for the current ind
            void format_field_display(const std::size_t ind);
    };

    class BatchForm {

        std::vector<std::shared_ptr<LinkedInterface>> forms;
        int current_ID = 0;
        
        // for linking
        bool linking_mode = false;
        std::shared_ptr<LinkedInterface> parent;
        std::shared_ptr<LinkedInterface> child;

        std::shared_ptr<RequestSender> sender;

        public:
            BatchForm() = default;
            ~BatchForm() = default;

            void render_form();
            void set_sender(std::shared_ptr<RequestSender> send_ptr) {
                this->sender = send_ptr;
            }

        private:
            void newFormButton();
            void pushNewForm(const int game, const int endpoint, const int endpoint_method);

    };

    static inline void render_json_form(std::vector<KEY_CONT>& keys, const int ind, bool second = false) {
        
        // render form input text
        static char _id[] = "##00a";
        _id[2] = ind;
        _id[4] = second ? 'b' : 'a';

        char counter = '0';
        for (KEY_CONT& key : keys) {
            _id[3] = counter;
            ImGui::InputText(_id, key.key, KEY_LENGTH, ImGuiTextFlags_None);
            counter++;
        }
        
        // add button
        static char _add_id[] = "Add##0a";
        _add_id[5] = ind;
        _add_id[6] = second ? 'b' : 'a';

        if (ImGui::Button(_add_id)) {
            keys.emplace_back("");
        }
        

        // remove button
        static char _del_id[] = "Remove##0a";
        _del_id[8] = ind;
        _del_id[9] = second ? 'b' : 'a';

        if (ImGui::Button(_del_id)) {
            keys.pop_back();
        }
    }

    static inline void render_iter_json(std::vector<KEY_CONT>& before, std::vector<KEY_CONT> after, char iter_limit[8], const int ind) {
        render_json_form(before, ind, false);

        static char _lim_id[] = "Iteration Limit##0";
        _lim_id[17] = ind;
        ImGui::InputText(_lim_id, iter_limit, 8, ImGuiInputTextFlags_CharsDecimal);

        render_json_form(after, ind, true);

        if (ImGui::Button("Select Parent")) {}; // todo
    };

    template<std::size_t N>
    void LinkedForm<N>::render_routing() {
        ImGui::InputText(this->_param_names[0], this->_params_in_form[0].param, 256, this->_type_ordering[0]);
    }
    
    template<std::size_t N>
    void LinkedForm<N>::render_field(const std::size_t ind) {

        static char _link_id[] = "Linked##0";
        _link_id[8] = (char)ind;
        ImGui::Checkbox(_link_id, &this->linked[ind]);

        static char _iter_id[] = "Iterative##0";
        _iter_id[11] = (char)ind;
        if (this->linked[ind]) {
            ImGui::Checkbox(_iter_id, &this->iterative[ind]);

            if (this->iterative[ind]) {
                render_iter_json(this->iter_info[ind]->keys, this->iter_info[ind]->access_after_iter.keys, this->iter_limits[ind], ind);
            } else {
                render_json_form(this->iter_info[ind]->keys, ind);
            }
        } else {
            ImGui::InputText(this->_param_names[ind].name, this->_params_in_form[ind].param, P_INPUT_LENGTH, this->_type_ordering[ind]);
        }
    };

    template<bool add, std::size_t... Ixs>
    void for_(std::function<void(const std::size_t)> func, std::index_sequence<Ixs...>) {
        if constexpr (add) {
            (func(Ixs+1), ...);
        } else {
            (func(Ixs), ...);
        }
    }
    

    template<std::size_t N>
    void LinkedForm<N>::render_summary() {
        if (ImGui::Button("Configure")) {this->configuring = true;};
        if (this->ready) {
            ImGui::Text("Ready");
        } else {
            ImGui::Text("Not Ready");
        }
        for_<true>(static_cast<std::function<void(const std::size_t)>>(std::bind_front(&LinkedForm<N>::display_field, this)), std::make_index_sequence<N-1>{});
    }

    template<std::size_t N>
    bool LinkedForm<N>::validate_field(const std::size_t ind) const {

        // check if the field is linked (not a full validation, full validation would require knowledge of the response structure (I'm too lazy))
        if (this->linked[ind]) { return this->parents[ind-1];
        } else {
            return this->_params_in_form[ind].param != "";
        }
    }

    template<std::size_t N>
    void LinkedForm<N>::on_close_popup() {
        // update summary display
        for_<false>(static_cast<std::function<void(const std::size_t)>>(std::bind_front(&LinkedForm<N>::format_field_display, this)), std::make_index_sequence<N>{});

        // check if the form is ready
        std::array<std::size_t, N> range_arr = {0};
        std::iota(range_arr.begin(), range_arr.end(), 0);
        this->ready = std::accumulate(range_arr.begin(), range_arr.end(), true, 
                [this](bool valid, std::size_t ind)
            {
                valid &= this->validate_field(ind);
                return valid;
            });
    };

    template<std::size_t N>
    void LinkedForm<N>::display_field(const std::size_t ind) const {
        ImGui::Text(this->display_form[ind].data());
    }

    static inline std::string display_iter_keys(const P_NAME& field_name, const std::vector<KEY_CONT>& keys, const std::vector<KEY_CONT>& keys2, const char iter_limit[8], bool linked) {
        std::stringstream formatted;
        // something like name: (Keys) key1, key2, ,key3, ..., final_key
        formatted << field_name.name << ':' << "(Keys) \n    Before:";
        for (int i = 0; i < keys.size() - 1; i++) {
            formatted << keys[i].key << ", ";
        }
        formatted << keys[keys.size() - 1].key << ". \n    After";

        for (int i = 0; i < keys2.size() - 1; i++) {
            formatted << keys2[i].key << ", ";
        }
        formatted << keys2[keys2.size() - 1].key << ". \n    Iteration Limit: " << iter_limit << "\n";

        if (linked) {
            formatted << "(LINKED)";
        } else {
            formatted  << "(NOT LINKED)";
        }

        return formatted.str();

    }

    static inline std::string display_json_keys(const P_NAME& field_name, const std::vector<KEY_CONT>& keys, bool linked) {
        std::stringstream formatted;
        // something like name: (Keys) key1, key2, ,key3, ..., final_key
        formatted << field_name.name << ':' << "(Keys) ";
        for (int i = 0; i < keys.size() - 1; i++) {
            formatted << keys[i].key << ", ";
        }
        formatted << keys[keys.size() - 1].key << ". ";

        if (linked) {
            formatted << "(LINKED)";
        } else {
            formatted  << "(NOT LINKED)";
        }

        return formatted.str();

    }

    template<std::size_t N>
    void LinkedForm<N>::format_field_display(const std::size_t ind) {
        bool linked_to_parent = this->parents[ind-1];
        if (this->iterative[ind]) {
            this->display_form[ind] = display_iter_keys(this->_param_names[ind], this->iter_info[ind-1]->keys, this->iter_info[ind-1]->access_after_iter.keys, this->iter_limits[ind-1], linked_to_parent);
        } else if (this->linked[ind]) {
            this->display_form[ind] = display_json_keys(this->_param_names[ind], this->iter_info[ind-1]->keys, linked_to_parent); 
        } else {
            this->display_form[ind] = std::string(this->_param_names[ind].name) + ": " + std::string(this->_params_in_form[ind].param);
        }
    }

    template<std::size_t N>
    void LinkedForm<N>::render_all_fields() {
        for_<false>(static_cast<std::function<void(const std::size_t)>>(std::bind_front(&LinkedForm<N>::render_field, this)), std::make_index_sequence<N-1>{});
    }

    template<std::size_t N>
    bool LinkedForm<N>::render_popup() {
        if (ImGui::Button("Close")) {
            this->on_close_popup();
            this->configuring = false;
        }
        this->render_routing();
        this->render_all_fields();
        return false;
    }

    template<std::size_t N>
    bool LinkedForm<N>::render_linking() {
        return ImGui::Button("Select");
    }

    template<std::size_t N>
    bool LinkedForm<N>::render_form(bool linking) {

        ImVec2 available = ImGui::GetContentRegionAvail();
        if (this->window_initiated) {

            if (this->movement_lock && ImGui::IsMouseHoveringRect(this->windowposition, ImVec2(this->windowposition.x + 0.25 * available.x, this->windowposition.y + 0.25 * available.y), true)) {
                this->dragging = true;
            }
            if (this->dragging) {
                this->windowposition.x += ImGui::GetIO().MouseDelta.x;
                this->windowposition.y += ImGui::GetIO().MouseDelta.y;
            }

            if (this->dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                this->dragging = false;
            }
        }

        ImGui::SetWindowPos(this->windowposition);

        bool to_ret = false;
        ImGui::BeginChild(this->_ID.data(), ImVec2(0.25 * available.x, 0.25 * available.y), true, ImGuiWindowFlags_ChildWindow);
        if (!this->window_initiated) {
            this->windowposition = ImGui::GetCursorPos();
            this->on_close_popup();
            this->window_initiated = true;
        }
        ImGui::Checkbox("Lock Window", &this->movement_lock);
        ImGui::Text((this->_game_name + " | " + this->_endpoint + " | " + this->_endpoint_method).data());
        if (!linking) {
            if (this->configuring) {
                to_ret = this->render_popup();
            } else {
                this->render_summary();
            }
        } else {
            to_ret = this->render_linking();
        }

        ImGui::EndChild();

        return to_ret;
    }

    template<std::size_t N>
    LinkedForm<N> make_linked_form(const BaseForm& copy) {
        return LinkedForm<N>(copy);
    }

}
