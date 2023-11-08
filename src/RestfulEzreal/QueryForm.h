#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "BatchRequests.h"
#include "RequestQueue.h"
#include "utils/utils.h"
#include <ranges>
#include <bits/utility.h>
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

#ifdef DEBUG_MODE
#include <iostream>
#define D(x) std::cerr << x <<'\n'
#else
#define D(x)  
#endif

#define INPUT_TEXT_FRAC 2/3
#define OPT_INPUT_TEXT_FRAC 1/3

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
            ~QUERY_FORM() = default;

            void set_id(int new_id) {
                this->_ID = std::string("##FORM") + std::to_string(new_id);
            }

            void set_sender(std::shared_ptr<RequestSender> sender_client) {this->sender = sender_client;}
            void render_form();
            bool check_remove() { return this->remove_form; };

            bool operator==(const QUERY_FORM& other_form);

            std::size_t get_n_params() const {return this->_n_params;};

        protected:
            void render_title();
            void render_singular_field(const int i, bool already_sent);
            request construct_request();
            const std::string& get_ID() {return this->_ID;};
            float form_height = 0.0f;

        private:
            virtual void render_required(bool already_sent);
            virtual void render_optionals(bool already_sent);
            void submit_request();
    };

    class LinkedInterface {
        // public functions BatchForm needs to be aware of
        public:
            virtual bool render_form(bool linking = false) = 0;
            virtual void set_id(int i) = 0;
            virtual const ImVec2 get_window_position() = 0;

            virtual void configure() = 0;
            virtual bool check_ready() const = 0;
            virtual bool check_iterative() = 0;
            virtual bool check_dependent() = 0;
            virtual void detach_base() = 0;

            virtual void construct_base() = 0;
            virtual void link_final_requests() = 0;
            virtual std::shared_ptr<RequestNode> get_base_request() = 0;

            virtual void insert_parent(std::shared_ptr<LinkedInterface> child) = 0;
            virtual void insert_child(std::shared_ptr<LinkedInterface> parent) = 0;

            virtual void delete_parent(LinkedInterface* parent) = 0;
            virtual void delete_child(LinkedInterface* child) = 0;

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
            float window_height = LinkedForm<N>::default_size.y;
            bool movement_lock = false;
            bool dragging = false;
            bool window_initiated  = false;
            // should be the number of parameters - 1
            std::array<std::shared_ptr<LinkedInterface>, N-1> parents;
            std::array<std::shared_ptr<iter_access_info>, N-1> iter_info;
            std::array<char[8], N-1> iter_limits;
            std::vector<std::string> display_form;

            std::vector<std::shared_ptr<LinkedInterface>> children; // variables size

            bool linked[N] = { 0 };
            bool iterative[N] = { 0 };
            std::vector<std::size_t> optional_to_send; 
            bool configuring = false;
            std::vector<std::string> p_name_id;

            std::size_t active_field = -1;

            // let user know if the form is finished
            bool ready = false;

            bool linking = false;
            std::size_t next_index = -1;

            std::string popup_id;
            
            bool req_iterative = false;
            std::shared_ptr<RequestNode> base_request = nullptr;

        public:
            LinkedForm() {};
            LinkedForm(const BaseForm& copy);

            bool render_form(bool linking = false) override;
            void configure() override {this->configuring = true;}
            const ImVec2 get_window_position() override {return this->windowposition;};
            
            void insert_parent(std::shared_ptr<LinkedInterface> child) override;
            void insert_child(std::shared_ptr<LinkedInterface> parent) override;

            void delete_parent(LinkedInterface* parent) override;
            void delete_child(LinkedInterface* child) override;

            void set_id(int new_id) override {this->_ID = std::string("##FORM") + std::to_string(new_id); this-> popup_id = std::string("Configure Request##FORM") + std::to_string(new_id);};
            static void set_default_size(ImVec2 new_size) {LinkedForm::default_size = new_size;}

            static ImVec2 default_size;

            bool check_ready() const override {return this->ready;}
            void construct_base() override;
            void link_final_requests() override;
            bool check_iterative() override;
            std::shared_ptr<RequestNode> get_base_request() override {return this->base_request;};
            bool check_dependent() override {
                bool dep = false;
                for (int i = 0; i < N; i++) {
                    dep |= this->linked[i];
                }
                return dep;
            };
            void detach_base() override {this->base_request = nullptr;};

            std::string rep();

        private:
            bool render_linking();
            void render_summary();
            void display_field(const std::size_t ind) const;
            void draw_links();

            bool render_popup();
            bool render_all_fields();
            void render_routing();
            bool render_field(const std::size_t ind);
            void render_optionals();
            
            // when the user closes the form we want to update certain fields to 
            // to avoid performing calculations on every frame
            void on_close_popup();
            bool validate_field(const std::size_t ind) const;

            // update display form for the current ind
            void format_field_display(const std::size_t ind);
    };

    class BatchForm {
        private:
            bool first_iter = true;
            std::vector<std::shared_ptr<LinkedInterface>> forms;
            int current_ID = 0;
            
            // for linking
            bool linking_mode = false;
            std::shared_ptr<LinkedInterface> parent;
            std::shared_ptr<LinkedInterface> child;

            std::shared_ptr<RequestSender> sender;

            ImVec2 contained_form_size;

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
            std::shared_ptr<BatchRequest> construct_request();
            void execute_request();

    };

    template<std::size_t N>
    std::string LinkedForm<N>::rep() {
        std::stringstream ss;
        ss << "GAME: " << this->_game_ind << "\n";
        ss << "ENDPOINT: " << this->_endpoint_ind << "\n";
        ss << "END_METHOD: " << this->_endpoint_method_ind << "\n";
        ss << "From State\n";
        ss << "Linked Fields\n    ";
        for (int i = 0; i < N; i++) {
            ss << this->linked[i] ? 1 : 0;
        }
        ss << "\nIterative Fields\n    ";
        for (int i = 0; i < N; i++) {
            ss << this->iterative[i] ? 1 : 0;
        }
        ss << "\nParents\n    ";
        for (int i = 0; i < N-1; i++) {
            if (this->parents[i])  {
                ss << 1;
            } else {
                ss << 0;
            }
        }
        
        ss << "\nNumber of Children: " << this->children.size();
        return ss.str();
    }

    inline void draw_form_link(const ImVec2& parent_pos, const ImVec2& child_pos, float width, float par_height) {
        // there is a better solution to avoid less function calls on the render loop but I cant be bothered rn
        ImVec2 beginning = ImVec2(parent_pos.x + width * 1/2, parent_pos.y + par_height);
        ImVec2 end = ImVec2(child_pos.x + width * 1/2, child_pos.y);
        ImVec2 mid_point = ImVec2(0.5 * (beginning.x + end.x), 0.5 * (beginning.y + end.y));

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddBezierQuadratic(beginning, ImVec2(beginning.x, mid_point.y), mid_point, 0xFFFFFFFF, 1.0);
        draw_list->AddBezierQuadratic(mid_point, ImVec2(end.x, mid_point.y), end, 0xFFFFFFFF, 1.0);
    }

    template<std::size_t N>
    void LinkedForm<N>::draw_links() {
        for (const auto& child : this->children) {
            draw_form_link(this->windowposition, child->get_window_position(), 0.25 * this->default_size.x, this->window_height);
        }
    }

    template<std::size_t N>
    LinkedForm<N>::LinkedForm(const BaseForm& copy) : BaseForm(copy) {
        this->parents.fill(nullptr);
        for (int i = 0; i < N-1; i++) {
            this->display_form.emplace_back("");
            this->iter_info[i] = std::make_shared<iter_access_info>();
        }

        for (int i = 0; i < N-1; i++) {
            memset(this->iter_limits[i], 0, 8);
        }
        for (P_NAME& p_n : this->_param_names) {
            this->p_name_id.emplace_back(std::string("##") + std::string(p_n.name));
        }
        this->display_form.emplace_back("");
    }

    bool render_json_form(std::vector<KEY_CONT>& keys, const int ind, const float x_align, const float width, bool active_field, bool second = false);

    bool render_iter_json(std::vector<KEY_CONT>& before, std::vector<KEY_CONT>& after, char iter_limit[8], const int ind, bool active_field, const float x_ali, const float width);

    template<std::size_t N>
    void LinkedForm<N>::render_routing() {
        ImVec2 avail = ImGui::GetCurrentWindow()->Size;
        ImGuiStyle& style = ImGui::GetStyle();
        static bool hlg = false;
        re_utils::form_center_aligned(avail.x * 0.25, avail.x * 0.75, "Routing", this->p_name_id[0].data(), this->_params_in_form[0].param, 256, this->_type_ordering[0], &hlg);
        if (hlg) {
            this->active_field = 0;
        }
    }
    
    template<std::size_t N>
    bool LinkedForm<N>::render_field(const std::size_t ind) {

        ImVec2 avail = ImGui::GetCurrentWindow()->Size;
        int n_lines;
        if (this->iterative[ind]) {
            n_lines = this->iter_info[ind-1]->keys.size() + this->iter_info[ind-1]->access_after_iter.keys.size() + 5;
        } else if(this->linked[ind]) {
            n_lines = this->iter_info[ind-1]->keys.size() + 3;
        } else {
            n_lines = 2;
        }

        ImVec2 windPadding = ImGui::GetStyle().WindowPadding;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + windPadding.y); // add padding
        ImVec2 winPos  = ImGui::GetWindowPos(); 
        ImVec2 rec_min = ImVec2(winPos.x + windPadding.x, winPos.y + ImGui::GetCursorPosY());// + windPadding.y);
        ImVec2 rec_max = ImVec2(winPos.x + avail.x - windPadding.x, winPos.y + ImGui::GetCursorPosY() + (ImGui::GetFrameHeightWithSpacing()) * n_lines + 1.5 * windPadding.y);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRect(rec_min, rec_max, this->active_field==ind ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.92f, 0.68f, 0.01f, 1.0f)) : 0x888888FF, 0.0f, 0, 1.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + windPadding.y); // add padding

        re_utils::title_aligned(avail.x * 0.25, this->_param_names[ind].name, this->active_field == ind);
        ImGui::SameLine();
        float x_align = ImGui::GetCursorPosX();
        float r_align = avail.x * 0.75;
        // linking option checkboc
        bool linking = false;
        static char _link_id[] = "Linked##0";
        _link_id[8] = (char)ind;
        ImGui::Checkbox(_link_id, &this->linked[ind]);
        if (ImGui::IsItemActive()) {
            this->active_field = ind;
        }
        
        static char _iter_id[] = "Iterative##0";
        _iter_id[11] = (char)ind;
        if (this->linked[ind]) {
            ImGui::SameLine();
            ImGui::Checkbox(_iter_id, &this->iterative[ind]);
            if (ImGui::IsItemActive()) {
                this->active_field = ind;
            }
            static char _link_but_id[]  = "Link Parent##0";
            _link_but_id[13] = (char) ind;
            ImGui::SetCursorPosX(x_align);
            if (ImGui::Button("Link Parent")) {
                this->next_index = ind;
                linking = true;
                this->configuring = false;
                this->active_field = ind;
            }
            if (this->parents[ind-1] != nullptr) {
                ImGui::SameLine();
                if (ImGui::Button("Unlink Parent")) {
                    this->parents[ind-1]->delete_child(this);
                    this->parents[ind-1] = nullptr;
                    this->active_field = ind;
                }
            }
            if (this->iterative[ind]) {
                if (render_iter_json(this->iter_info[ind-1]->keys, this->iter_info[ind-1]->access_after_iter.keys, this->iter_limits[ind-1], ind, this->active_field == ind, x_align, r_align)) {
                    this->active_field = ind;
                }
            } else {
                if (render_json_form(this->iter_info[ind-1]->keys, ind, x_align, r_align, this->active_field == ind, false)) {
                    this->active_field = ind;
                }
            }
        } else {
            ImGui::SetCursorPosX(x_align);
            ImGui::SetNextItemWidth(r_align - x_align - 0.5 * (x_align - avail.x * 0.25));
            ImGui::InputTextWithHint(this->p_name_id[ind].data(), this->_param_names[ind].name, this->_params_in_form[ind].param, P_INPUT_LENGTH, this->_type_ordering[ind]);
            if (ImGui::IsItemActive()) {
                this->active_field = ind;
            }
        }
        return linking;
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
        for_<false>(static_cast<std::function<void(const std::size_t)>>(std::bind_front(&LinkedForm<N>::display_field, this)), std::make_index_sequence<N>{});
    }

    template<std::size_t N>
    bool LinkedForm<N>::validate_field(const std::size_t ind) const {

        // check if the field is linked (not a full validation, full validation would require knowledge of the response structure (I'm too lazy))
        if (this->linked[ind]) { return !this->parents[ind-1] ? false : true;
        } else {
            return strlen(this->_params_in_form[ind].param) != 0;
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
        for (int i = 0; i < keys.size(); i++) {
            formatted << keys[i].key << " ";
        }
        formatted << ". \n    After";

        for (int i = 0; i < keys2.size(); i++) {
            formatted << keys2[i].key << " ";
        }
        formatted << ". \n    Iteration Limit: " << iter_limit << "\n";

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
        for (int i = 0; i < keys.size(); i++) {
            formatted << keys[i].key << " ";
        }
        formatted << ". ";

        if (linked) {
            formatted << "(LINKED)";
        } else {
            formatted  << "(NOT LINKED)";
        }

        return formatted.str();

    }

    template<std::size_t N>
    void LinkedForm<N>::format_field_display(const std::size_t ind) {
        bool linked_to_parent = !this->parents[ind-1] ? false : true;
        if (this->iterative[ind]) {
            this->display_form[ind] = display_iter_keys(this->_param_names[ind], this->iter_info[ind-1]->keys, this->iter_info[ind-1]->access_after_iter.keys, this->iter_limits[ind-1], linked_to_parent);
        } else if (this->linked[ind]) {
            this->display_form[ind] = display_json_keys(this->_param_names[ind], this->iter_info[ind-1]->keys, linked_to_parent); 
        } else {
            this->display_form[ind] = std::string(this->_param_names[ind].name) + ": " + std::string(this->_params_in_form[ind].param);
        }
    }

    template<std::size_t N>
    bool LinkedForm<N>::render_all_fields() {
        if constexpr (N == 1) {
            return false;
        } else {
            auto check_ret = [this](bool acc, const std::size_t ind){
                acc = acc || this->render_field(ind);
                return acc;
            };
            std::array<std::size_t, N-1> inds = {0};
            std::iota(inds.begin(), inds.end(), 1);

            bool link_mode = std::accumulate(inds.begin(), inds.end(), false, check_ret);
            return link_mode;
        }
    }

    template<std::size_t N>
    bool LinkedForm<N>::render_popup() {
        bool linking = false;
        static ImGuiIO& io = ImGui::GetIO();
        ImGui::OpenPopup(this->popup_id.data());

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.5, io.DisplaySize.y * 0.75));
        
        bool open = true;

        if (ImGui::BeginPopupModal(this->popup_id.data(), &open, ImGuiWindowFlags_AlwaysAutoResize)) {
            this->render_routing();
            linking = this->render_all_fields();
            if (this->_accepts_optional) {
                this->render_optionals();
            }
            ImGui::EndPopup();
        }

        if (!open) {
            this->on_close_popup();
            this->configuring = false;
        }
        return linking;
    }

    template<std::size_t N>
    bool LinkedForm<N>::render_linking() {
        if (ImGui::Button("Select")) {
            return true;
        } else {
            return false;
        }
    }

    inline int find_next_focus(const std::vector<int>& opts_to_send) {

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

    template<std::size_t N>
    void LinkedForm<N>::render_optionals() {

        if (!this->_accepts_optional) {
            throw std::logic_error("QUERY_FORM::render_optional should never be called for endpoints with no optional arguements");
        }

        ImVec2 avail = ImGui::GetCurrentWindow()->Size;
        ImVec2 windPadding = ImGui::GetStyle().WindowPadding;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + windPadding.y); // add padding
        ImVec2 winPos  = ImGui::GetWindowPos(); 
        ImVec2 rec_min = ImVec2(winPos.x + windPadding.x, winPos.y + ImGui::GetCursorPosY());// + windPadding.y);
        ImVec2 rec_max = ImVec2(winPos.x + avail.x - windPadding.x, winPos.y + ImGui::GetCursorPosY() + (ImGui::GetFrameHeightWithSpacing()) * (this->_n_used_optional_p1) + 1.5 * windPadding.y);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRect(rec_min, rec_max, this->active_field==-2 ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.92f, 0.68f, 0.01f, 1.0f)) : 0x888888FF, 0.0f, 0, 1.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + windPadding.y); // add padding

        re_utils::title_aligned(avail.x * 0.25, "Optional", this->active_field == -2);
        ImGui::SameLine();
        float x_align = ImGui::GetCursorPosX();
        float r_align = avail.x * 0.75;

        static char id[5] = "x## ";
        static int opt_index = 0;

        // render input forms
        for (int i = 0; i < this->_optionals_to_send.size(); i++) {
            if (this->_optionals_to_send[i] != 1) {
                continue;
            }
            float text_width = 0.5 * (r_align - x_align);
            ImGui::SetCursorPosX(x_align);
            ImGui::SetNextItemWidth(text_width);
            ImGui::InputText(this->_optional_names[i], this->_optional_inputs[i], 256, this->_optional_types[i]);
            if (ImGui::IsItemActive()) {
                this->active_field = -2;
            }

            ImGui::SameLine(ImGui::GetContentRegionAvail().x * INPUT_TEXT_FRAC - 2 * ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("x").x);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
            id[3] = i;
        
            if (ImGui::Button(id)) {
                this->_optionals_to_send[i] = 0;
                this->_n_used_optional_p1 -= 1;
                opt_index = i;
                this->active_field = -2;
            };
        }
        ImGui::SetCursorPosX(x_align);
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
                        this->active_field = -2;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }

        if (this->_n_used_optional_p1 == this->_optionals_to_send.size()+1) {
            return;
        }

        // Render add optional button besides dropdown
        const static char button_name[] = "+";
        ImGui::SameLine();
        ImGui::SetCursorPosX(r_align - ImGui::CalcTextSize("+").x);
        if (ImGui::Button(button_name)) {
            if (this->_optionals_to_send[opt_index] == 0) {this->_n_used_optional_p1 += 1;};
            this->active_field = -2;
            this->_optionals_to_send[opt_index] = 1;
            opt_index = find_next_focus(this->_optionals_to_send);
        }
    }

    template<std::size_t N>
    bool LinkedForm<N>::render_form(bool linking) {

        if (!this->window_initiated) {
            this->windowposition = ImGui::GetWindowPos();
            this->on_close_popup();
            this->window_initiated = true;
        }

        if (this->window_initiated) {

            if (!this->movement_lock && ImGui::IsMouseHoveringRect(this->windowposition, ImVec2(this->windowposition.x + 0.25 * default_size.x, this->windowposition.y + this->window_height), true)) {
                this->dragging = true;
            }
            if (this->dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                this->windowposition.x += ImGui::GetIO().MouseDelta.x;
                this->windowposition.y += ImGui::GetIO().MouseDelta.y;
            }

            if (this->dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                this->dragging = false;
            }
        }

        ImGui::SetNextWindowPos(this->windowposition);

        this->draw_links();

        bool to_ret = false;
        if (ImGui::BeginChild(this->_ID.data(), ImVec2(0.25 * default_size.x, this->window_height), true, ImGuiWindowFlags_ChildWindow)) {;
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
        }
        this->window_height = ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y;
        ImGui::EndChild();
        return to_ret;
    }

    template<std::size_t N>
    LinkedForm<N> make_linked_form(const BaseForm& copy) {
        return LinkedForm<N>(copy);
    }

    // [SECTION] Handling connections between forms before execution
    
    template<std::size_t N>
    inline void LinkedForm<N>::insert_child(std::shared_ptr<LinkedInterface> child) {
        this->children.push_back(child);
    }

    template<std::size_t N> 
    void LinkedForm<N>::insert_parent(std::shared_ptr<LinkedInterface> parent) {
        if (this->next_index == -1) {
            throw std::logic_error("Trying to insert parent whilst the index has not been set");
        }
        this->parents[this->next_index-1] = parent;
        this->linked[this->next_index] = true;
        this->next_index == -1;
    }

    template<std::size_t N>
    void LinkedForm<N>::delete_child(LinkedInterface* child) {
        std::vector<int> inds;
        int counter = 0;
        for (std::shared_ptr<LinkedInterface>& possible : this->children) {
            if (possible.get() == child) {
                inds.push_back(counter);
            }
            counter++;
        }
        int dels = 0;
        for (int to_delete : inds) {
            this->children.erase(this->children.begin() + to_delete - dels);
            dels++;
        }
    }

    template<std::size_t N>
    void LinkedForm<N>::delete_parent(LinkedInterface* parent) {
        for (int i = 0; i < N-1; i++) {
            if (this->parents[i].get() == parent) {this->parents[i] = nullptr;}
        }
    }

    // [SECTION] Construction methods
    
    template<std::size_t N>
    bool LinkedForm<N>::check_iterative() {
        if (this->req_iterative) {
            // exit early we already know this request is iterative
            return true;
        }
        for (int i = 0; i < N; i++) {
            this->req_iterative |= iterative[i];
        }
        if (this->req_iterative) {
            // no need to check parent requests if we already know iterative
            return true;
        }
        for (auto& parent : this->parents) {
            // a child is iterative if any of its parents are (this will also calculate parent checks to prevent recomputation)
            if (parent) {
                this->req_iterative = parent->check_iterative();
            }
        }
        return this->req_iterative;
    }

    template<std::size_t N>
    void LinkedForm<N>::construct_base() {
        D("Constructing base request from form");
        D(this->rep());
        if (this->check_iterative()) {
            D("Iterative");
            this->base_request = std::make_shared<RequestNode>(std::make_unique<IterativeRequest>());
        } else {
            D("Not iterative");
            this->base_request = std::make_shared<RequestNode>(std::make_unique<LinkedRequest>());
        }
        // insert parameters into base request, linked params will be overwritten eventually
        for (int i = 0; i < this->_params_in_form.size(); i++) {
            this->base_request->_node->unsent_request->params.push_back(this->_params_in_form[i]);
        }
        this->base_request->_node->unsent_request->_game = this->_game_ind;
        this->base_request->_node->unsent_request->_endpoint = this->_endpoint_ind;
        this->base_request->_node->unsent_request->_endpoint_method = this->_endpoint_method_ind;

        // insert optional parameters in request
        for (int i = 0; i < this->_optional_inputs.size(); i++) {
            if (this->_optionals_to_send[i] == 1) {
                // write better todo
                this->base_request->_node->unsent_request->optional_names.push_back(this->_optional_names[i]);
                this->base_request->_node->unsent_request->optional_inputs.push_back(this->_optional_inputs[i]);
            } else {
                this->base_request->_node->unsent_request->optional_names.push_back("");
                this->base_request->_node->unsent_request->optional_inputs.push_back("");
            }
        }
    }
    
    template<typename Derived, typename Base>
    std::unique_ptr<Derived> dynamic_ptr_cast(std::unique_ptr<Base>&& base) noexcept {
            if (auto derived = dynamic_cast<Derived*>(base.release())) {
                    return std::unique_ptr<Derived>(derived);
            }
            return nullptr;
    }

    inline void insert_link_iter(std::shared_ptr<RequestNode> current, std::shared_ptr<LinkedInterface> parent, const iter_access_info& lnk_info, const int ind) {
        std::unique_ptr<IterativeRequest> child_node = dynamic_ptr_cast<IterativeRequest>(std::move(current->_node->unsent_request));
        if (!child_node) {
            throw std::logic_error("Inserting iterative link for non-iterative field");
        }
        child_node->param_indices.push_back(ind);

        // check if parent was aready inserted
        for (auto& lnk : child_node->iter_dependencies) {
            if (lnk.parent == parent->get_base_request()) {
                lnk.iter_link_info.push_back(lnk_info);
                current->_node->unsent_request = std::move(child_node);
                return;
            }
        }
        D("ITERATIVE: Adding child request into parent list");
        parent->get_base_request()->_node->unsent_request->children.push_back(current); // beautiful
        // insert new link didn't find existing with same parent
        child_node->iter_dependencies.push_back({});
        child_node->iter_dependencies.back().parent = parent->get_base_request();
        child_node->iter_dependencies.back().iter_link_info.push_back(lnk_info);
        current->_node->unsent_request = std::move(child_node);
        return;
    }

    inline void insert_link(std::shared_ptr<RequestNode> current, std::shared_ptr<LinkedInterface> parent, const json_access_info &lnk_info, const int ind) {
        std::unique_ptr<LinkedRequest> child_node = dynamic_ptr_cast<LinkedRequest>(std::move(current->_node->unsent_request));
        if (!child_node) {
            throw std::logic_error("Inserting iterative link for non-iterative field");
        }
        // check if parent was aready inserted
        for (auto& lnk : child_node->dependencies) {
            if (lnk.parent == parent->get_base_request()) {
                lnk.link_info.push_back(lnk_info);
                lnk.param_indices.push_back(ind);
                current->_node->unsent_request = std::move(child_node);
                return;
            }
        }
        D("NON-ITERATIVE: Adding child request into parent list");
        parent->get_base_request()->_node->unsent_request->children.push_back(current); // beautiful
        child_node->dependencies.push_back({});
        child_node->dependencies.back().parent = parent->get_base_request();
        child_node->dependencies.back().link_info.push_back(lnk_info);
        child_node->dependencies.back().param_indices.push_back(ind);
        current->_node->unsent_request = std::move(child_node);
        return;

    }

    template<std::size_t N>
    void LinkedForm<N>::link_final_requests() {

        D(static_cast<int>(N) << this->rep());
        
        // link iterative fields
        auto iter_fl = [this](int i){return this->iterative[i];};
        for (int i : std::views::iota(0, static_cast<int>(N)) | std::views::filter(iter_fl)) {
            D("Inserting iterative linked parent: index" << i);
            this->iter_info[i-1]->iter_limit = std::atoi(this->iter_limits[i-1]);
            insert_link_iter(this->base_request, this->parents[i-1], *this->iter_info[i-1], i);
        }
        
        // link linked fields (NOT ITERATIVE)
        auto link_fl = [this](int i){return this->linked[i] && !this->iterative[i];};
        for (int i : std::views::iota(0, static_cast<int>(N)) | std::views::filter(link_fl)) {
            D("Inserting non itertive linked parent: index" << i);
            insert_link(this->base_request, this->parents[i-1], this->iter_info[i-1]->get_base(), i);
        }
    };
}
