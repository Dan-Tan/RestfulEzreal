#pragma once
#include "client/client.h"
#include <spdlog/spdlog.h>
#include "json/json.h"
#include "RequestQueue.h"
#include "QueryForm.h"
#include "simdjson.h"
#include "db/DatabaseManager.h"

namespace client = riotcpp::client;
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <vulkan/vulkan_core.h>
#include <imgui.h>

#define _NO_FLAG ImGuiInputTextFlags_None
#define _DEC_FLAG ImGuiInputTextFlags_CharsDecimal

enum current_page {
    HOME,
    GUIDE,
    ADVANCED_REQUESTS,
    CLIENT_CONFIG,
    REQUEST_HISTORY,
    REQUEST_STATUS
};

namespace restfulEz {

    class RestfulEzreal
    {
        private:
            std::shared_ptr<client::RiotApiClient> _underlying_client = nullptr;
            current_page* _on_display;
            std::string _path_to_output;

            std::vector<QUERY_FORM> _current_forms = {};
            std::shared_ptr<RequestSender> request_sender = std::make_shared<RequestSender>(); // default init create thread
            bool send_next_request = false;
            bool client_tested = false;

            std::shared_ptr<BatchForm> batch_group;

            int _next_form_id = 1;

            std::string custom_config_path;
            bool show_welcome = true;
            int welcome_state = -1;

        public:
            RestfulEzreal(current_page* on_display);
            ~RestfulEzreal() {
                this->request_sender->stop();
                DatabaseManager::GetInstance().Close();
            }

            void OnUIRender();
            void SkipWelcomeAndInit(const std::string& workspace_dir = "./output");
            int render_welcome();
            int render_welcome_config();
            QUERY_FORM* _next_request = nullptr;

        private:

            void NewFormButton();

            int _current_id = 1;

            void NewQueryForm(int gameName, int end_name, int endpoint_method); // purely for readability and maintenance

            void pushNewForm(QUERY_FORM new_form);

            void set_debug_level() {};
            void set_verbosity() {};

            void render_home();
            void render_client_status();
            void render_guide();
            void render_advanced_requests();
            void render_client_config();
            void render_json();
            void render_request_history();
            void render_tab_workspace_overview();
            void render_tab_duckdb_analytics();
            void render_tab_sql_export();
            void refresh_duckdb_analytics();
            void execute_custom_sql();
            void export_current_query_csv();
            void export_table_csv(const std::string& table_name);
            void render_rate_status();

            // Client Config page state
            char _config_key_input[256] = "";
            bool _config_key_initialized = false;
            int _config_log_level = 2; // spdlog::level::info
            bool _config_verbosity = false;
            std::string _config_status_message;

            // Request History page state
            std::string _history_selected_file;
            std::string _history_file_content;
            std::vector<std::string> _history_formatted_lines;
            bool _history_content_loaded = false;
            int _history_category = 0; // 0=All, 1=Accounts, 2=Matches, 3=Timelines
            std::vector<std::pair<std::string, std::string>> _cached_file_list; // {rel_path, abs_path}
            int _history_last_refresh_frame = -100;
            void refresh_history_files();

            // SQL Query & DuckDB Explorer State
            char _sql_query_input[4096] = "SELECT champion_name, COUNT(*) AS picks, SUM(CASE WHEN win THEN 1 ELSE 0 END) AS wins, ROUND(100.0 * SUM(CASE WHEN win THEN 1 ELSE 0 END) / COUNT(*), 1) AS winrate_pct, ROUND(AVG(kills), 1) AS avg_k, ROUND(AVG(deaths), 1) AS avg_d, ROUND(AVG(assists), 1) AS avg_a FROM match_participants GROUP BY champion_name ORDER BY picks DESC LIMIT 20;";
            QueryResult _current_query_result;
            std::string _query_status_message;
            bool _query_has_error = false;
            std::string _export_status_message;
            int _export_table_selected = 0;
            TableCounts _cached_table_counts;
            QueryResult _cached_champion_stats;
            QueryResult _cached_player_stats;
            QueryResult _cached_timeline_stats;
            QueryResult _cached_event_stats;
            int _analytics_last_refresh_frame = -100;
            std::string _backfill_status_message;

            // Client State collapse toggle
            bool _client_state_collapsed = true;
            
            void configure_from_existing(bool custom_path = false, const std::string& file_path = std::string(""));
            bool configure_new_client(bool allow_close = false);
            template<bool in_popup>
            bool conf_form();
            void reconfigure_client();
            void config_check();

            bool start_up_from_existing(int* ret);
            void instantiate_client(int* int_out, int* parse_out, const std::string& file_path);
            int display_configuration_process(std::unique_ptr<std::string> file_cont, const float x_align);
            bool start_up_default(int* ret);
    };
}
