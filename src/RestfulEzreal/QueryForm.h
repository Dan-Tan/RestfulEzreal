#pragma once
#include "Walnut/Application.h"
#include "BatchRequests.h"
#include "RequestQueue.h"
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

            std::string _ID; // Distinguish Form from other Query Forms
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
            virtual void render_form();
            bool check_remove() { return this->remove_form; };
            std::size_t get_n_params() {return this->_n_params;};

        protected:
            void render_singular_field(const int i, bool already_sent);

        private:
            virtual void render_required(bool already_sent);
            virtual void render_optionals(bool already_sent);
            void submit_request();
    };

    class LinkedForm : public QUERY_FORM{
        private:
            bool iterative = false;
            std::vector<std::shared_ptr<LinkedForm>> parents;
            std::vector<std::shared_ptr<LinkedForm>> children;
            std::vector<std::size_t> parent_ordering;
            std::vector<param_dependence_info> link_descriptions;
            PARAM_CONT iter_index;
            PARAM_CONT iter_limit;
            bool inputs_from_parents[5] = {false, false, false, false, false}; // forgive me

        public:
            LinkedForm(const QUERY_FORM& copy) : QUERY_FORM(copy) {
                for (int i = 0; i < this->get_n_params(); i++) {
                    this->link_descriptions.emplace_back(false, std::vector<std::string>(), 0, 0, i);
                }
            };
            ~LinkedForm() = default;

        private:
            void render_required(bool already_sent) override;
            void render_linked_fields(const int i);
            void add_parent(LinkedForm& parent);
    };

}
