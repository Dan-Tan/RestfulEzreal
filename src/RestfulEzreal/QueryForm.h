#pragma once
#include "Walnut/Application.h"
#include "BatchRequests.h"
#include "RequestQueue.h"
#include <iterator>
#include <vector>
#include <string>
#include <cstring>

namespace restfulEz {

    class QUERY_FORM {
        private:
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
            std::shared_ptr<RequestSender> sender;
            
            bool form_execute = false;
            bool remove_form = false;
            

        public:
            QUERY_FORM(const QUERY_FORM& copy) {*this = copy;};
            QUERY_FORM(const std::size_t n_params, const int game_ind, const int endpoint_ind, 
                    const int endpoint_method_ind, const char* game_name, 
                    const char* endpoint, const char* endpoint_method, 
                    std::vector<P_NAME> param_names, std::vector<ImGuiInputTextFlags> type_ordering,
                    bool accepts_optional = false, std::vector<P_NAME> optional_names = {}, 
                    std::vector<PARAM_CONT> optional_inputs = {}, std::vector<ImGuiInputTextFlags> optional_types = {});
            ~QUERY_FORM() = default;

            void set_sender(std::shared_ptr<RequestSender> sender_client) {this->sender = sender_client;}
            void set_id(int new_id) {
                this->_ID = std::string("##FORM") + std::to_string(new_id);
            }
            virtual void render_form();
            bool check_remove() { return this->remove_form; };
            std::size_t get_n_params() {return this->_n_params;};
            void calc_height() {this->recalculate_height = true;}

            bool operator==(const QUERY_FORM& other_form);
            std::string _ID; // Distinguish Form from other Query Forms

        protected:
            void render_title();
            void render_singular_field(const int i, bool already_sent);
            const std::string& get_ID() {return this->_ID;};
            bool recalculate_height = true;
            float form_height = 0.0f;

        private:
            virtual void render_required(bool already_sent);
            virtual void render_optionals(bool already_sent);
            void submit_request();
    };

    class LinkedForm : public QUERY_FORM{
        private:
            bool iterative = false;
            std::vector<std::shared_ptr<LinkedForm>> parents; // list of parents for easy request construction upon submit
            std::vector<std::shared_ptr<LinkedForm>> children; // list of children to allow for easier deletion

            std::vector<param_dependence_info> link_descriptions; // one possible link description per input
            std::vector<PARAM_CONT> iter_index; // link_description encodes iter_index/limit as ints, IMGUI requires char*
            std::vector<PARAM_CONT> iter_limit;
            bool inputs_from_parents[5] = {false, false, false, false, false}; // forgive me
            
            std::size_t next_index = -1;

        public:
            LinkedForm(const QUERY_FORM& copy) : QUERY_FORM(copy) {
                for (std::size_t i = 0; i < this->get_n_params(); i++) {
                    this->link_descriptions.push_back({false, std::vector<PARAM_CONT>(), i, 0, 0});
                    this->iter_index.push_back({});
                    this->iter_limit.push_back({});
                    this->parents.push_back(nullptr);
                }
            };
            ~LinkedForm() = default;
            bool render_form_return();
            bool render_link_mode();

            void complete_link(std::shared_ptr<LinkedForm> next_parent);
            void cancel_link(); // resets next_index
            
            void insert_child(std::shared_ptr<LinkedForm> child);
            std::vector<std::shared_ptr<LinkedForm>> get_children() const {return this->children;}; // for FormGroup to notify paqrent of removed children
            std::vector<std::shared_ptr<LinkedForm>> get_parents() const {return this->parents;}; // for FormGroup to notify paqrent of removed children
            void remove_parent(const LinkedForm& parent);
            void remove_child(const LinkedForm& child);
        private:
            void render_required(bool already_sent) override;
            void render_linked_fields(const int i);
    };

}
