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
            virtual ~LinkedInterface() = default;
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

            // Lifecycle & Deletion extensions (M1)
            virtual void disconnect_all() = 0;
            virtual bool check_remove() const = 0;
            virtual void mark_for_removal() = 0;
            virtual void request_remove() { mark_for_removal(); }
            virtual std::shared_ptr<NodeExecutionTracker> get_tracker() = 0;
            virtual std::shared_ptr<NodeExecutionTracker> get_execution_tracker() { return get_tracker(); }
            virtual void reset_tracker() = 0;
            virtual void reset_execution_state() { reset_tracker(); }
            virtual int get_form_num() const = 0;

            // Identification & layout accessors (M2)
            virtual int get_game() const = 0;
            virtual int get_endpoint() const = 0;
            virtual int get_endpoint_method() const = 0;
            virtual int get_id() const { return get_form_num(); }

            virtual void set_window_position(const ImVec2& pos) = 0;
            virtual bool is_locked() const = 0;
            virtual void set_locked(bool lock) = 0;
            virtual bool get_zip_mode() const = 0;
            virtual void set_zip_mode(bool zip) = 0;

            // Parameter accessors (M2)
            virtual std::size_t get_param_count() const = 0;
            virtual std::string get_param_name(std::size_t idx) const = 0;
            virtual std::string get_param_value(std::size_t idx) const = 0;
            virtual void set_param_value(std::size_t idx, const std::string& val) = 0;

            virtual bool is_param_linked(std::size_t idx) const = 0;
            virtual void set_param_linked(std::size_t idx, bool linked) = 0;
            virtual bool is_param_iterative(std::size_t idx) const = 0;
            virtual void set_param_iterative(std::size_t idx, bool iter) = 0;
            virtual bool is_param_deduplicate(std::size_t idx) const = 0;
            virtual void set_param_deduplicate(std::size_t idx, bool dedup) = 0;
            virtual bool is_param_skip_db(std::size_t idx) const = 0;
            virtual void set_param_skip_db(std::size_t idx, bool skip) = 0;

            // Parent edge connections (M2)
            virtual std::shared_ptr<LinkedInterface> get_parent_for_index(std::size_t idx) const = 0;
            virtual void set_parent_for_index(std::size_t idx, std::shared_ptr<LinkedInterface> parent) = 0;

            // Extraction path accessors (M2)
            virtual std::vector<std::string> get_keys_before(std::size_t idx) const = 0;
            virtual void set_keys_before(std::size_t idx, const std::vector<std::string>& keys) = 0;
            virtual std::vector<std::string> get_keys_after(std::size_t idx) const = 0;
            virtual void set_keys_after(std::size_t idx, const std::vector<std::string>& keys) = 0;

            virtual int get_iter_limit_val(std::size_t idx) const = 0;
            virtual void set_iter_limit_val(std::size_t idx, int limit) = 0;

            // Optionals accessors (M2)
            virtual std::size_t get_optional_count() const = 0;
            virtual std::string get_optional_name(std::size_t idx) const = 0;
            virtual std::string get_optional_value(std::size_t idx) const = 0;
            virtual bool is_optional_enabled(std::size_t idx) const = 0;
            virtual void set_optional(std::size_t idx, const std::string& val, bool enabled) = 0;

            virtual void refresh_display() = 0;

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
            bool deduplicate_pool[N] = { 0 };
            bool skip_existing_db[N] = { 0 };
            bool zip_mode = false;
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
            int form_num = 1;
            std::shared_ptr<RequestNode> base_request = nullptr;

            bool remove_requested = false;
            std::shared_ptr<NodeExecutionTracker> exec_tracker = std::make_shared<NodeExecutionTracker>();

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

            void disconnect_all() override;
            bool check_remove() const override { return this->remove_requested; }
            void mark_for_removal() override { this->remove_requested = true; }
            int get_form_num() const override { return this->form_num; }
            std::shared_ptr<NodeExecutionTracker> get_tracker() override { return this->exec_tracker; }
            void reset_tracker() override;

            void set_id(int new_id) override {this->form_num = new_id; this->_ID = std::string("##FORM") + std::to_string(new_id); this->popup_id = std::string("Configure Request##FORM") + std::to_string(new_id);};
            static void set_default_size(ImVec2 new_size) {LinkedForm::default_size = new_size;}

            static ImVec2 default_size;

            int get_game() const override { return this->_game_ind; }
            int get_endpoint() const override { return this->_endpoint_ind; }
            int get_endpoint_method() const override { return this->_endpoint_method_ind; }
            int get_id() const override { return this->form_num; }

            void set_window_position(const ImVec2& pos) override {
                this->windowposition = pos;
                this->window_initiated = true;
            }
            bool is_locked() const override { return this->movement_lock; }
            void set_locked(bool lock) override { this->movement_lock = lock; }
            bool get_zip_mode() const override { return this->zip_mode; }
            void set_zip_mode(bool zip) override { this->zip_mode = zip; }

            std::size_t get_param_count() const override { return N; }
            std::string get_param_name(std::size_t idx) const override {
                if (idx < this->_param_names.size()) {
                    return std::string(this->_param_names[idx].name);
                }
                return "";
            }
            std::string get_param_value(std::size_t idx) const override {
                if (idx < this->_params_in_form.size()) {
                    return std::string(this->_params_in_form[idx].param);
                }
                return "";
            }
            void set_param_value(std::size_t idx, const std::string& val) override {
                if (idx < this->_params_in_form.size()) {
                    this->_params_in_form[idx] = val.c_str();
                }
            }
            bool is_param_linked(std::size_t idx) const override {
                return (idx < N) ? this->linked[idx] : false;
            }
            void set_param_linked(std::size_t idx, bool linked_val) override {
                if (idx < N) this->linked[idx] = linked_val;
            }
            bool is_param_iterative(std::size_t idx) const override {
                return (idx < N) ? this->iterative[idx] : false;
            }
            void set_param_iterative(std::size_t idx, bool iter_val) override {
                if (idx < N) this->iterative[idx] = iter_val;
            }
            bool is_param_deduplicate(std::size_t idx) const override {
                return (idx < N) ? this->deduplicate_pool[idx] : false;
            }
            void set_param_deduplicate(std::size_t idx, bool dedup) override {
                if (idx < N) this->deduplicate_pool[idx] = dedup;
            }
            bool is_param_skip_db(std::size_t idx) const override {
                return (idx < N) ? this->skip_existing_db[idx] : false;
            }
            void set_param_skip_db(std::size_t idx, bool skip) override {
                if (idx < N) this->skip_existing_db[idx] = skip;
            }

            std::shared_ptr<LinkedInterface> get_parent_for_index(std::size_t idx) const override {
                if (idx > 0 && idx - 1 < this->parents.size()) {
                    return this->parents[idx - 1];
                }
                return nullptr;
            }
            void set_parent_for_index(std::size_t idx, std::shared_ptr<LinkedInterface> parent_node) override {
                if (idx > 0 && idx - 1 < this->parents.size()) {
                    this->parents[idx - 1] = parent_node;
                    this->linked[idx] = (parent_node != nullptr);
                }
            }

            std::vector<std::string> get_keys_before(std::size_t idx) const override {
                std::vector<std::string> res;
                if (idx > 0 && idx - 1 < this->iter_info.size() && this->iter_info[idx - 1]) {
                    for (const auto& k : this->iter_info[idx - 1]->keys) {
                        res.push_back(std::string(k.key));
                    }
                }
                return res;
            }
            void set_keys_before(std::size_t idx, const std::vector<std::string>& keys) override {
                if (idx > 0 && idx - 1 < this->iter_info.size() && this->iter_info[idx - 1]) {
                    this->iter_info[idx - 1]->keys.clear();
                    for (const auto& k : keys) {
                        this->iter_info[idx - 1]->keys.emplace_back(k.c_str());
                    }
                }
            }
            std::vector<std::string> get_keys_after(std::size_t idx) const override {
                std::vector<std::string> res;
                if (idx > 0 && idx - 1 < this->iter_info.size() && this->iter_info[idx - 1]) {
                    for (const auto& k : this->iter_info[idx - 1]->access_after_iter.keys) {
                        res.push_back(std::string(k.key));
                    }
                }
                return res;
            }
            void set_keys_after(std::size_t idx, const std::vector<std::string>& keys) override {
                if (idx > 0 && idx - 1 < this->iter_info.size() && this->iter_info[idx - 1]) {
                    this->iter_info[idx - 1]->access_after_iter.keys.clear();
                    for (const auto& k : keys) {
                        this->iter_info[idx - 1]->access_after_iter.keys.emplace_back(k.c_str());
                    }
                }
            }
            int get_iter_limit_val(std::size_t idx) const override {
                if (idx > 0 && idx - 1 < this->iter_limits.size()) {
                    return std::atoi(this->iter_limits[idx - 1]);
                }
                return 0;
            }
            void set_iter_limit_val(std::size_t idx, int limit) override {
                if (idx > 0 && idx - 1 < this->iter_limits.size()) {
                    std::string s = (limit > 0) ? std::to_string(limit) : "";
                    std::strncpy(this->iter_limits[idx - 1], s.c_str(), sizeof(this->iter_limits[idx - 1]) - 1);
                    this->iter_limits[idx - 1][sizeof(this->iter_limits[idx - 1]) - 1] = '\0';
                }
            }

            std::size_t get_optional_count() const override {
                return this->_optional_names.size();
            }
            std::string get_optional_name(std::size_t idx) const override {
                if (idx < this->_optional_names.size()) {
                    return std::string(this->_optional_names[idx].name);
                }
                return "";
            }
            std::string get_optional_value(std::size_t idx) const override {
                if (idx < this->_optional_inputs.size()) {
                    return std::string(this->_optional_inputs[idx].param);
                }
                return "";
            }
            bool is_optional_enabled(std::size_t idx) const override {
                if (idx < this->_optionals_to_send.size()) {
                    return this->_optionals_to_send[idx] == 1;
                }
                return false;
            }
            void set_optional(std::size_t idx, const std::string& val, bool enabled) override {
                if (idx < this->_optional_inputs.size()) {
                    this->_optional_inputs[idx] = val.c_str();
                }
                if (idx < this->_optionals_to_send.size()) {
                    bool was_enabled = (this->_optionals_to_send[idx] == 1);
                    this->_optionals_to_send[idx] = enabled ? 1 : 0;
                    if (enabled && !was_enabled) {
                        this->_n_used_optional_p1++;
                    } else if (!enabled && was_enabled) {
                        this->_n_used_optional_p1--;
                    }
                }
            }

            void refresh_display() override {
                this->on_close_popup();
            }

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

    enum class BatchLifecycleState { IDLE, RUNNING, COMPLETED, FAILED };
    using BatchLifecycle = BatchLifecycleState;

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

            BatchLifecycleState lifecycle = BatchLifecycleState::IDLE;
            std::shared_ptr<BatchRequest> active_batch = nullptr;

            // UI Modal States (M2)
            bool show_quick_fill = false;
            bool show_save_modal = false;
            char quick_fill_routing[64] = "ASIA";
            char quick_fill_game_name[128] = "Faker";
            char quick_fill_tagline[64] = "KR1";
            char save_template_name[128] = "My Custom Pipeline";
            char save_template_desc[256] = "Custom linked request pipeline graph";

        public:
            BatchForm() = default;
            ~BatchForm() = default;

            void render_form();
            void set_sender(std::shared_ptr<RequestSender> send_ptr) {
                this->sender = send_ptr;
            }

            void clear_canvas();
            void reset_pipeline();
            BatchLifecycleState get_lifecycle_state() const { return lifecycle; }
            const std::vector<std::shared_ptr<LinkedInterface>>& get_forms() const { return forms; }

            void add_form(std::shared_ptr<LinkedInterface> form);
            void set_current_id(int id) { this->current_ID = id; }
            int get_current_id() const { return this->current_ID; }

            void open_quick_fill_modal() { show_quick_fill = true; }
            void open_save_modal() { show_save_modal = true; }

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
            if (child) {
                draw_form_link(this->windowposition, child->get_window_position(), 0.25 * this->default_size.x, this->window_height);
            }
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
        if constexpr (N > 2) {
            ImGui::SetCursorPosX(avail.x * 0.25);
            ImGui::Checkbox("Zip Mode (Pair Multiple Pools 1-to-1)", &this->zip_mode);
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
        _link_id[8] = (char)('0' + ind);
        ImGui::Checkbox(_link_id, &this->linked[ind]);
        if (ImGui::IsItemActive()) {
            this->active_field = ind;
        }
        
        static char _iter_id[] = "Iterative##0";
        _iter_id[11] = (char)('0' + ind);
        if (this->linked[ind]) {
            ImGui::SameLine();
            ImGui::Checkbox(_iter_id, &this->iterative[ind]);
            if (ImGui::IsItemActive()) {
                this->active_field = ind;
            }
            static char _dedup_id[] = "Unique##0";
            _dedup_id[8] = (char)('0' + ind);
            ImGui::SameLine();
            ImGui::Checkbox(_dedup_id, &this->deduplicate_pool[ind]);
            if (ImGui::IsItemActive()) {
                this->active_field = ind;
            }
            static char _skip_db_id[] = "Skip DB##0";
            _skip_db_id[9] = (char)('0' + ind);
            ImGui::SameLine();
            ImGui::Checkbox(_skip_db_id, &this->skip_existing_db[ind]);
            if (ImGui::IsItemActive()) {
                this->active_field = ind;
            }
            static char _link_but_id[]  = "Link Parent##0";
            _link_but_id[13] = (char)('0' + ind);
            ImGui::SetCursorPosX(x_align);
            if (ImGui::Button(_link_but_id)) {
                this->next_index = ind;
                linking = true;
                this->configuring = false;
                this->active_field = ind;
            }
            if (this->parents[ind-1] != nullptr) {
                static char _unlink_but_id[] = "Unlink Parent##0";
                _unlink_but_id[15] = (char)('0' + ind);
                ImGui::SameLine();
                if (ImGui::Button(_unlink_but_id)) {
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

        NodeExecState st = this->exec_tracker->state.load();
        if (st == NodeExecState::IDLE && this->ready) {
            st = NodeExecState::READY;
            this->exec_tracker->state.store(NodeExecState::READY);
        } else if (st == NodeExecState::READY && !this->ready) {
            st = NodeExecState::IDLE;
            this->exec_tracker->state.store(NodeExecState::IDLE);
        }

        int dispatched = this->exec_tracker->processed_count.load();
        int total = this->exec_tracker->total_count.load();

        switch (st) {
            case NodeExecState::IDLE:
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[ Idle ]");
                break;
            case NodeExecState::READY:
                ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "[ Ready ]");
                break;
            case NodeExecState::WAITING_FOR_PARENTS:
                ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "[ Waiting for Parent ]");
                break;
            case NodeExecState::EXECUTING:
                if (total > 0) {
                    ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.01f, 1.0f), "[ Executing: %d / %d ]", dispatched, total);
                } else {
                    ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.01f, 1.0f), "[ Executing: %d ]", dispatched);
                }
                break;
            case NodeExecState::COMPLETED:
                ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.3f, 1.0f), "[ Completed (%d queries) ]", dispatched);
                break;
            case NodeExecState::FAILED:
                ImGui::TextColored(ImVec4(0.9f, 0.25f, 0.25f, 1.0f), "[ Failed ]");
                break;
        }
        for_<false>(static_cast<std::function<void(const std::size_t)>>(std::bind_front(&LinkedForm<N>::display_field, this)), std::make_index_sequence<N>{});
    }

    template<std::size_t N>
    bool LinkedForm<N>::validate_field(const std::size_t ind) const {
        if (ind == 0) {
            return strlen(this->_params_in_form[0].param) != 0;
        }
        if (this->linked[ind]) {
            return (ind - 1 < this->parents.size() && this->parents[ind-1] != nullptr);
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
        if (ind == 0) {
            this->display_form[0] = std::string(this->_param_names[0].name) + ": " + std::string(this->_params_in_form[0].param);
            return;
        }
        bool linked_to_parent = (ind - 1 < this->parents.size() && this->parents[ind-1] != nullptr);
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
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Done")) {
                open = false;
                ImGui::CloseCurrentPopup();
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
            ImVec2 base_pos = ImGui::GetWindowPos();
            int idx = (this->form_num >= 0) ? this->form_num : 0;
            float col_w = 0.27f * default_size.x;
            this->windowposition = ImVec2(
                base_pos.x + 12.0f + (idx % 3) * col_w,
                base_pos.y + 65.0f + (idx / 3) * 200.0f
            );
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

        this->draw_links();

        ImGui::SetCursorScreenPos(this->windowposition);

        bool to_ret = false;
        if (ImGui::BeginChild(this->_ID.data(), ImVec2(0.25 * default_size.x, this->window_height), true, ImGuiWindowFlags_ChildWindow)) {;
            ImGui::Checkbox("Lock Window", &this->movement_lock);
            ImGui::SameLine();
            if (ImGui::Button(("Delete##" + std::to_string(this->form_num)).c_str())) {
                this->remove_requested = true;
            }
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
        if (child && std::find(this->children.begin(), this->children.end(), child) == this->children.end()) {
            this->children.push_back(child);
        }
    }

    template<std::size_t N> 
    void LinkedForm<N>::insert_parent(std::shared_ptr<LinkedInterface> parent) {
        if (this->next_index == -1) {
            throw std::logic_error("Trying to insert parent whilst the index has not been set");
        }
        this->parents[this->next_index-1] = parent;
        this->linked[this->next_index] = true;
        this->next_index = -1;
    }

    template<std::size_t N>
    void LinkedForm<N>::delete_child(LinkedInterface* child) {
        std::erase_if(this->children, [child](const std::shared_ptr<LinkedInterface>& p) {
            return p.get() == child;
        });
    }

    template<std::size_t N>
    void LinkedForm<N>::delete_parent(LinkedInterface* parent) {
        for (std::size_t i = 0; i < N - 1; i++) {
            if (this->parents[i].get() == parent) {
                this->parents[i] = nullptr;
                this->linked[i + 1] = false;
                this->iterative[i + 1] = false;
                if (this->iter_info[i]) {
                    this->iter_info[i]->keys.clear();
                    this->iter_info[i]->access_after_iter.keys.clear();
                }
                std::memset(this->iter_limits[i], 0, sizeof(this->iter_limits[i]));
                this->format_field_display(i + 1);
            }
        }
        this->on_close_popup();
    }

    template<std::size_t N>
    void LinkedForm<N>::disconnect_all() {
        for (std::size_t i = 0; i < N - 1; ++i) {
            if (this->parents[i]) {
                this->parents[i]->delete_child(this);
                this->parents[i] = nullptr;
            }
            this->linked[i + 1] = false;
            this->iterative[i + 1] = false;
            if (this->iter_info[i]) {
                this->iter_info[i]->keys.clear();
                this->iter_info[i]->access_after_iter.keys.clear();
            }
            std::memset(this->iter_limits[i], 0, sizeof(this->iter_limits[i]));
            this->format_field_display(i + 1);
        }
        for (auto& ch : this->children) {
            if (ch) {
                ch->delete_parent(this);
            }
        }
        this->children.clear();
        this->on_close_popup();
    }

    template<std::size_t N>
    void LinkedForm<N>::reset_tracker() {
        this->on_close_popup();
        this->exec_tracker->reset(this->ready ? NodeExecState::READY : NodeExecState::IDLE);
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
                this->req_iterative |= parent->check_iterative();
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
        this->base_request->tracker = this->exec_tracker;
        this->base_request->_node->unsent_request->tracker = this->exec_tracker;
        if (this->check_dependent()) {
            this->exec_tracker->state.store(NodeExecState::WAITING_FOR_PARENTS);
        } else {
            this->exec_tracker->state.store(this->ready ? NodeExecState::READY : NodeExecState::IDLE);
        }
        auto& req = this->base_request->_node->unsent_request;
        req->set_bind_mode(this->zip_mode ? PoolBindMode::ZIP : PoolBindMode::CROSS);

        // Insert parameters into base request and initialize static ValuePools
        for (std::size_t i = 0; i < this->_params_in_form.size(); i++) {
            req->params.push_back(this->_params_in_form[i]);
            std::string raw_val(this->_params_in_form[i].param);
            if ((i < N && !this->linked[i]) && raw_val.find(',') != std::string::npos) {
                req->set_param_pool(i, ValuePool::FromDelimitedString(raw_val, ',', this->deduplicate_pool[i]));
            }
        }
        req->_game = this->_game_ind;
        req->_endpoint = this->_endpoint_ind;
        req->_endpoint_method = this->_endpoint_method_ind;

        // insert optional parameters in request
        for (int i = 0; i < this->_optional_inputs.size(); i++) {
            if (this->_optionals_to_send[i] == 1) {
                req->optional_names.push_back(this->_optional_names[i]);
                req->optional_inputs.push_back(this->_optional_inputs[i]);
            } else {
                req->optional_names.push_back("");
                req->optional_inputs.push_back("");
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
        if (!current || !current->_node || !current->_node->unsent_request || !parent || !parent->get_base_request() || !parent->get_base_request()->_node) return;

        auto& child_node = current->_node->unsent_request;
        child_node->param_indices.push_back(ind);

        // check if parent was already inserted
        for (auto& lnk : child_node->iter_dependencies) {
            if (lnk.parent == parent->get_base_request()) {
                lnk.iter_link_info.push_back(lnk_info);
                lnk.param_indices.push_back(ind);
                return;
            }
        }
        D("ITERATIVE: Adding child request into parent list");
        if (parent->get_base_request()->_node->unsent_request) {
            auto& p_children = parent->get_base_request()->_node->unsent_request->children;
            if (std::find(p_children.begin(), p_children.end(), current) == p_children.end()) {
                p_children.push_back(current);
            }
        }
        child_node->iter_dependencies.push_back({});
        child_node->iter_dependencies.back().parent = parent->get_base_request();
        child_node->iter_dependencies.back().iter_link_info.push_back(lnk_info);
        child_node->iter_dependencies.back().param_indices.push_back(ind);
    }

    inline void insert_link(std::shared_ptr<RequestNode> current, std::shared_ptr<LinkedInterface> parent, const json_access_info &lnk_info, const int ind) {
        if (!current || !current->_node || !current->_node->unsent_request || !parent || !parent->get_base_request() || !parent->get_base_request()->_node) return;

        auto& child_node = current->_node->unsent_request;
        // check if parent was already inserted
        for (auto& lnk : child_node->dependencies) {
            if (lnk.parent == parent->get_base_request()) {
                lnk.link_info.push_back(lnk_info);
                lnk.param_indices.push_back(ind);
                return;
            }
        }
        D("NON-ITERATIVE: Adding child request into parent list");
        if (parent->get_base_request()->_node->unsent_request) {
            auto& p_children = parent->get_base_request()->_node->unsent_request->children;
            if (std::find(p_children.begin(), p_children.end(), current) == p_children.end()) {
                p_children.push_back(current);
            }
        }
        child_node->dependencies.push_back({});
        child_node->dependencies.back().parent = parent->get_base_request();
        child_node->dependencies.back().link_info.push_back(lnk_info);
        child_node->dependencies.back().param_indices.push_back(ind);
    }

    template<std::size_t N>
    void LinkedForm<N>::link_final_requests() {
        if constexpr (N <= 1) return;
        if (!this->base_request || !this->base_request->_node || !this->base_request->_node->unsent_request) return;

        auto& req = this->base_request->_node->unsent_request;
        req->set_bind_mode(this->zip_mode ? PoolBindMode::ZIP : PoolBindMode::CROSS);

        for (std::size_t i = 1; i < N; ++i) {
            auto pool = req->get_or_create_pool(i);
            pool->set_deduplicate(this->deduplicate_pool[i]);
            if (this->skip_existing_db[i]) {
                if (this->_game_ind == 0 && this->_endpoint_ind == 0 && this->_endpoint_method_ind == 0) {
                    pool->set_filter(ValuePool::SkipExistingPlayersInDB());
                } else if (this->_game_ind == 1 && this->_endpoint_ind == 6 && this->_endpoint_method_ind == 1) {
                    pool->set_filter(ValuePool::SkipExistingTimelinesInDB());
                } else {
                    pool->set_filter(ValuePool::SkipExistingMatchesInDB());
                }
            }

            if (this->iterative[i] && this->parents[i-1] && this->iter_info[i-1]) {
                this->iter_info[i-1]->iter_limit = std::atoi(this->iter_limits[i-1]);
                insert_link_iter(this->base_request, this->parents[i-1], *this->iter_info[i-1], static_cast<int>(i));
            } else if (this->linked[i] && !this->iterative[i] && this->parents[i-1] && this->iter_info[i-1]) {
                insert_link(this->base_request, this->parents[i-1], this->iter_info[i-1]->get_base(), static_cast<int>(i));
            }
        }
    }
}


