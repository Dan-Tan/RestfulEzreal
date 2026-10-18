#include "FileManager.h"
#include "TemplateManager.h"
#include "simdjson.h"
#include <nfd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

namespace restfulEz {

FileManager::FileManager() {
    UpdateWorkspacePaths();
}

FileManager& FileManager::GetInstance() {
    static FileManager instance;
    return instance;
}

static std::string trim_string(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\"'");
    return str.substr(first, (last - first + 1));
}

static std::string expand_home(const std::string& path_str) {
    if (path_str.empty()) return path_str;
    if (path_str[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            return std::string(home) + path_str.substr(1);
        }
    }
    return path_str;
}

void FileManager::SanitizeConfig() {
    // 1. Trim and clean API key
    config_.api_key = trim_string(config_.api_key);

    // 2. Normalize active workspace path
    config_.active_workspace_path = trim_string(config_.active_workspace_path);
    if (config_.active_workspace_path.empty()) {
        config_.active_workspace_path = "./output";
    }
    config_.active_workspace_path = expand_home(config_.active_workspace_path);

    // 3. Normalize log path
    config_.log_path = trim_string(config_.log_path);
    if (config_.log_path.empty()) {
        config_.log_path = (fs::path(config_.active_workspace_path) / "logs" / "ezreal.log").string();
    }
    config_.log_path = expand_home(config_.log_path);

    // 4. Validate log level
    if (static_cast<int>(config_.log_level) < 0 || static_cast<int>(config_.log_level) > 6) {
        config_.log_level = spdlog::level::info;
    }

    // 5. Update workspace paths & ensure directories exist
    UpdateWorkspacePaths();
}

std::string FileManager::ResolveConfigPath(const std::string& preferred_path) const {
    if (!preferred_path.empty()) {
        return preferred_path;
    }

    // Priority candidate paths
    std::vector<std::string> candidates = {
        "./EzrealRunes.json",
        "../EzrealRunes.json",
        "/home/dtan/projects/EzrealRunes.json"
    };

    const char* home = getenv("HOME");
    if (home) {
        candidates.push_back(std::string(home) + "/.config/RestfulEzreal/EzrealRunes.json");
        candidates.push_back(std::string(home) + "/projects/EzrealRunes.json");
    }

    for (const auto& candidate : candidates) {
        try {
            if (fs::exists(candidate)) {
                return candidate;
            }
        } catch (...) {
            // Ignore filesystem access errors during probing
        }
    }

    return "./EzrealRunes.json";
}


void FileManager::UpdateWorkspacePaths() {
    try {
        workspace_paths_.root_dir = fs::path(config_.active_workspace_path);
        workspace_paths_.db_file = workspace_paths_.root_dir / "db" / "ezreal_data.duckdb";
        workspace_paths_.raw_dir = workspace_paths_.root_dir / "raw";
        workspace_paths_.raw_accounts = workspace_paths_.raw_dir / "accounts";
        workspace_paths_.raw_matches = workspace_paths_.raw_dir / "matches";
        workspace_paths_.raw_timelines = workspace_paths_.raw_dir / "timelines";
        workspace_paths_.templates_dir = workspace_paths_.root_dir / "templates";
        workspace_paths_.log_file = fs::path(config_.log_path);

        fs::create_directories(workspace_paths_.root_dir / "db");
        fs::create_directories(workspace_paths_.raw_accounts);
        fs::create_directories(workspace_paths_.raw_matches);
        fs::create_directories(workspace_paths_.raw_timelines);
        fs::create_directories(workspace_paths_.templates_dir);
        fs::create_directories(workspace_paths_.root_dir / "logs");

        TemplateManager::GetInstance().EnsureStarterTemplatesInWorkspace(workspace_paths_.templates_dir);

        if (workspace_paths_.log_file.has_parent_path()) {
            fs::create_directories(workspace_paths_.log_file.parent_path());
        }
    } catch (const std::exception& e) {
        std::cerr << "[FileManager] Error creating workspace directories: " << e.what() << std::endl;
    }
}

bool FileManager::LoadConfig(const std::string& config_path) {
    resolved_config_path_ = ResolveConfigPath(config_path);

    try {
        if (!fs::exists(resolved_config_path_)) {
            // Populate defaults when no file is present
            SanitizeConfig();
            config_loaded_ = true;
            return false;
        }

        simdjson::ondemand::parser parser;
        simdjson::padded_string contents = simdjson::padded_string::load(resolved_config_path_);
        simdjson::ondemand::document doc = parser.iterate(contents);

        std::string_view api_key_sv;
        if (doc["api-key"].get_string().get(api_key_sv) == simdjson::SUCCESS) {
            config_.api_key = std::string(api_key_sv);
        }

        std::string_view out_path_sv;
        if (doc["output-path"].get_string().get(out_path_sv) == simdjson::SUCCESS) {
            config_.active_workspace_path = std::string(out_path_sv);
        }

        std::string_view log_path_sv;
        if (doc["log-path"].get_string().get(log_path_sv) == simdjson::SUCCESS) {
            config_.log_path = std::string(log_path_sv);
        }

        int64_t log_level_val = 0;
        if (doc["log-level"].get_int64().get(log_level_val) == simdjson::SUCCESS) {
            config_.log_level = static_cast<spdlog::level::level_enum>(log_level_val);
        }

        bool verbosity_val = false;
        if (doc["verbosity"].get_bool().get(verbosity_val) == simdjson::SUCCESS) {
            config_.verbosity = verbosity_val;
        }

        SanitizeConfig();
        config_loaded_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[FileManager] Error reading config from " << resolved_config_path_ << ": " << e.what() << std::endl;
        SanitizeConfig();
        config_loaded_ = true;
        return false;
    }
}

bool FileManager::SaveConfig(const std::string& config_path) {
    std::string target_path = config_path.empty() ? resolved_config_path_ : config_path;
    SanitizeConfig();

    try {
        fs::path p(target_path);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        std::ofstream config_file(target_path, std::ios::trunc);
        if (!config_file.is_open()) {
            return false;
        }

        config_file << "{\n";
        config_file << "\t\"api-key\" : \"" << config_.api_key << "\",\n";
        config_file << "\t\"log-path\" : \"" << config_.log_path << "\",\n";
        config_file << "\t\"log-level\" : " << static_cast<int>(config_.log_level) << ",\n";
        config_file << "\t\"output-path\" : \"" << config_.active_workspace_path << "\",\n";
        config_file << "\t\"verbosity\" : " << (config_.verbosity ? "true" : "false") << "\n";
        config_file << "}\n";

        config_file.close();
        resolved_config_path_ = target_path;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[FileManager] Failed to save config to " << target_path << ": " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::LoadEnvKey(const std::string& env_path) {
    const char* env_key = std::getenv("RIOT_KEY");
    if (env_key && env_key[0] != '\0') {
        SetApiKey(env_key);
        if (!config_.api_key.empty()) {
            return true;
        }
    }

    std::vector<std::string> candidates = {env_path, "./.env", "../.env"};
    for (const auto& candidate : candidates) {
        if (candidate.empty() || !fs::exists(candidate)) continue;
        std::ifstream file(candidate);
        if (!file.is_open()) continue;

        std::string line;
        while (std::getline(file, line)) {
            std::string trimmed = trim_string(line);
            if (trimmed.empty() || trimmed[0] == '#') continue;

            if (trimmed.rfind("export ", 0) == 0) {
                trimmed = trim_string(trimmed.substr(7));
            }
            if (trimmed.rfind("RIOT_KEY", 0) == 0) {
                size_t eq_pos = trimmed.find('=');
                if (eq_pos != std::string::npos) {
                    std::string val = trim_string(trimmed.substr(eq_pos + 1));
                    if (!val.empty()) {
                        SetApiKey(val);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void FileManager::SetApiKey(const std::string& key) {
    config_.api_key = trim_string(key);
}

const std::string& FileManager::GetApiKey() const {
    return config_.api_key;
}


std::string FileManager::GetDevKeyPrefix() const {
    if (config_.api_key.empty()) {
        return "DEFAULT_DEV";
    }
    if (config_.api_key.rfind("RGAPI-", 0) == 0 && config_.api_key.length() >= 14) {
        return config_.api_key.substr(0, 14);
    }
    // Return hash prefix for non-standard key strings
    return std::to_string(std::hash<std::string>{}(config_.api_key)).substr(0, 8);
}

bool FileManager::SetWorkspace(const std::string& workspace_path) {
    config_.active_workspace_path = workspace_path;
    UpdateWorkspacePaths();
    return true;
}

const WorkspacePaths& FileManager::GetWorkspacePaths() const {
    return workspace_paths_;
}

fs::path FileManager::GetRawOutputPath(int game, int endpoint, int endpoint_method, const std::string& identifier) {
    std::string safe_id = identifier;
    if (safe_id.empty()) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        safe_id = std::to_string(now);
    }

    // Riot Account (Game 0, Endpoint 0)
    if (game == 0 && endpoint == 0) {
        return workspace_paths_.raw_accounts / (safe_id + ".json");
    }
    // League Match Details (Game 1, Endpoint 6, Method 0)
    if (game == 1 && endpoint == 6 && endpoint_method == 0) {
        return workspace_paths_.raw_matches / (safe_id + ".json");
    }
    // League Match Timeline (Game 1, Endpoint 6, Method 1)
    if (game == 1 && endpoint == 6 && endpoint_method == 1) {
        return workspace_paths_.raw_timelines / (safe_id + "_timeline.json");
    }

    // Default categorized raw files
    return workspace_paths_.raw_dir / ("req_" + std::to_string(game) + "_" + std::to_string(endpoint) + "_" + std::to_string(endpoint_method) + "_" + safe_id + ".json");
}

std::string FileManager::OpenFileDialog(const std::string& filter_desc, const std::string& filter_ext) {
    NFD_Init();
    std::string result_path = "";
    nfdchar_t* out_path = nullptr;
    
    nfdresult_t result;
    if (!filter_ext.empty()) {
        nfdfilteritem_t filter_item[1] = {{filter_desc.c_str(), filter_ext.c_str()}};
        result = NFD_OpenDialog(&out_path, filter_item, 1, nullptr);
    } else {
        result = NFD_OpenDialog(&out_path, nullptr, 0, nullptr);
    }

    if (result == NFD_OKAY && out_path) {
        result_path = std::string(out_path);
        NFD_FreePathN(out_path);
    }
    NFD_Quit();
    return result_path;
}

std::string FileManager::SaveFileDialog(const std::string& default_name, const std::string& filter_desc, const std::string& filter_ext) {
    NFD_Init();
    std::string result_path = "";
    nfdchar_t* out_path = nullptr;

    nfdresult_t result;
    if (!filter_ext.empty()) {
        nfdfilteritem_t filter_item[1] = {{filter_desc.c_str(), filter_ext.c_str()}};
        result = NFD_SaveDialog(&out_path, filter_item, 1, nullptr, default_name.empty() ? nullptr : default_name.c_str());
    } else {
        result = NFD_SaveDialog(&out_path, nullptr, 0, nullptr, default_name.empty() ? nullptr : default_name.c_str());
    }

    if (result == NFD_OKAY && out_path) {
        result_path = std::string(out_path);
        NFD_FreePathN(out_path);
    }
    NFD_Quit();
    return result_path;
}

std::string FileManager::SelectFolderDialog() {
    NFD_Init();
    std::string result_path = "";
    nfdchar_t* out_path = nullptr;
    nfdresult_t result = NFD_PickFolder(&out_path, nullptr);

    if (result == NFD_OKAY && out_path) {
        result_path = std::string(out_path);
        NFD_FreePathN(out_path);
    }
    NFD_Quit();
    return result_path;
}

} // namespace restfulEz
