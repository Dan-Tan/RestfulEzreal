#include "Walnut/Application.h"
#include <iostream>
#include <thread>
#include <tuple>
#include <array>
#include <json/json.h>
#include "../include/RestfulEzreal.h"

#ifndef CONFIG_FILE_PATH
	#define CONFIG_FILE_PATH "./EzrealRunes.json"
#else
#endif

namespace restfulEz {

	void RestfulEzreal::OnUIRender() {
		// Main render function for the user interface

		ImGui::ShowMetricsWindow();

		static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
		static bool open = true;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		ImGui::Begin("RestfulEzreal", &open, window_flags);

		// We want to check which page is selected on the nav bar and render the selected page
		if (!this->_underlying_client) { this->config_check(); };

		if (*this->_on_display == current_page::HOME) { this->render_home(); }

		ImGui::End();
		
		// In a seperate window render the status of the client.
		this->render_client_status();
	}

	bool RestfulEzreal::validate_key() {
		// check if the api-key given is valid.
		bool valid_key = false;
		bool tested = false;

		if (!valid_key && !tested) {
			Json::Value response = this->_underlying_client->Lol_Status.v4("OC1");
			if (response.empty()) {
				tested = true;
				valid_key = false;
			}
			else if (!response.isMember("status")) {
				valid_key = true;
				tested = true;
			}
			else {
				if (response["status"].isMember("status_code")) {
					ImGui::Text("UNEXPECTED RESPONSE FORMAT");
				}
				const int  response_code = response["status"]["status_code"].asInt();
				if (response_code == 401 || response_code == 403) {
					tested = true;
					valid_key = false;
				}
			}
		}
		if (!valid_key && !tested) {
			Json::Value response = this->_underlying_client->Lol_Status.v4("KR");
			if (!response.empty() || !response.isMember("status")) {
				valid_key = true;
			}
			else {
				if (response["status"].isMember("status_code")) {
					ImGui::Text("UNEXPECTED RESPONSE FORMAT");
				}
				const int  response_code = response["status"]["status_code"].asInt();
				if (response_code == 401 || response_code == 403) {
					tested = true;
					valid_key = false;
				}
			}
			tested = true;
		}

		return valid_key;
	}

	void RestfulEzreal::render_client_status() {

		static bool valid_key = false;

		static bool open = true;
		static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;
		ImGui::Begin("Client State", &open, window_flags);
		
		if (!this->_underlying_client) {
			ImGui::Text("Client not intialised");
			ImGui::End();
			return;
		}

		if (!this->client_tested) {
			valid_key = this->validate_key();
			this->client_tested = true;
		}


		if (valid_key) {
			ImGui::Text("Client Initialised with valid key");
		}
		else {
			ImGui::Text("API-KEY given was not valid or unable to be validated, please check your connection or RIOT developer website");
		}

		ImGui::End();


	};

	void RestfulEzreal::render_home() {

		ImGui::Text("Home");

		int counter = 0;
		int to_remove = -1;
		for (QUERY_FORM& form : this->_current_forms) {
			if (form.check_remove()) {
				to_remove = counter;
			}
			counter += 1;
		}
		if (to_remove != -1) {
			this->_current_forms.erase(this->_current_forms.begin() + to_remove);
		}

		for (QUERY_FORM& form : this->_current_forms) {
			form.render_form();
		}

		this->NewFormButton();

		
		if (this->send_next_request) { 
			std::jthread t(std::bind_front(&RestfulEzreal::Send_Request, this));
		};
	}

	inline bool file_exists(const std::string& name) {
		if (FILE* file = fopen(name.c_str(), "r")) {
			fclose(file);
			return true;
		}
		else {
			return false;
		}
	}

	void RestfulEzreal::config_check() {
		Json::Value config;
		if (file_exists(CONFIG_FILE_PATH)) {
			std::ifstream config_file;
			config_file.open(CONFIG_FILE_PATH);

			Json::Reader reader;
			if (!reader.parse(config_file, config)) { return; }// log

			logging::LEVEL report_level = static_cast<logging::LEVEL>(config["log-level"].asInt());
			this->_path_to_output = config["output-path"].asString();

			this->_underlying_client = std::make_shared<client::RiotApiClient>(CONFIG_FILE_PATH, config["log-path"].asString(), report_level, config["verbosity"].asBool());
			config_file.open(CONFIG_FILE_PATH);
			return;
		}
		ImGui::OpenPopup("Configure Client");

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Configure Client", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			static char api_key[256] = "";
			static char path_to_log[64] = "";
			static char path_to_output[64] = "";
			static char logging_level[16] = "";
			static bool verbosity = false;
			static bool selected[5] = {};

			static bool submit = false;

			static logging::LEVEL level_ = logging::LEVEL::INFO;

			ImGui::InputText("API-KEY", api_key, 256);
			ImGui::InputText("OUTPUT DIRECTORY", path_to_output, 64);
			ImGui::InputText("PATH TO LOGFILE", path_to_log, 64);

			if (ImGui::BeginTable("Logging Level", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {

				ImGui::TableNextColumn();
				if (ImGui::Selectable("DEBUG", level_ == logging::LEVEL::DEBUG)) {
					level_ = logging::LEVEL::DEBUG;
				}; // FIXME-TABLE: Selection overlap
				ImGui::TableNextColumn();
				if (ImGui::Selectable("INFO", level_ == logging::LEVEL::INFO)) {
					level_ = logging::LEVEL::INFO;
				}; // FIXME-TABLE: Selection overlap
				ImGui::TableNextColumn();
				if (ImGui::Selectable("WARNING", level_ == logging::LEVEL::WARNING)) {
					level_ = logging::LEVEL::WARNING;
				}; // FIXME-TABLE: Selection overlap
				ImGui::TableNextColumn();
				if (ImGui::Selectable("ERRORS", level_ == logging::LEVEL::ERRORS)) {
					level_ = logging::LEVEL::ERRORS;
				}; // FIXME-TABLE: Selection overlap
				ImGui::TableNextColumn();
				if (ImGui::Selectable("CRITICAL", level_ == logging::LEVEL::CRITICAL)) {
					level_ = logging::LEVEL::CRITICAL;
				}; // FIXME-TABLE: Selection overlap

				ImGui::EndTable();
			}
			ImGui::Checkbox("Verbose Logging", &verbosity);

			if (ImGui::Button("Submit")) {
				config["api-key"] = api_key;
				config["log-path"] = path_to_log;
				config["output-path"] = path_to_output;
				config["verbosity"] = verbosity;
				config["log-level"] = level_;
				Json::StreamWriterBuilder builder;
				std::ofstream config_file;
				config_file.open(CONFIG_FILE_PATH);
				config_file << Json::writeString(builder, config);
				config_file.close();
				this->_underlying_client = std::make_shared<client::RiotApiClient>(CONFIG_FILE_PATH, path_to_log, level_, verbosity);
				this->_path_to_output = path_to_output;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}
