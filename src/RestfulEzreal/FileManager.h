#pragma once

#include <filesystem>
#include <string>
#include <memory>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace restfulEz {

struct AppConfig {
    std::string api_key = "";
    std::string active_workspace_path = "./output";
    std::string log_path = "logs.log";
    spdlog::level::level_enum log_level = spdlog::level::info;
    bool verbosity = false;
};

struct WorkspacePaths {
    fs::path root_dir;
    fs::path db_file;
    fs::path raw_dir;
    fs::path raw_accounts;
    fs::path raw_matches;
    fs::path raw_timelines;
    fs::path log_file;
};

class FileManager {
public:
    static FileManager& GetInstance();

    // Configuration Loading & Saving
    bool LoadConfig(const std::string& config_path = "");
    bool SaveConfig(const std::string& config_path = "");
    
    // Path resolution helper across common locations
    std::string ResolveConfigPath(const std::string& preferred_path = "") const;
    const std::string& GetResolvedConfigPath() const { return resolved_config_path_; }
    void SanitizeConfig();

    // API Key management (independent of workspace/db paths)
    void SetApiKey(const std::string& key);
    const std::string& GetApiKey() const;
    std::string GetDevKeyPrefix() const;

    // Workspace & Database Path Configuration
    bool SetWorkspace(const std::string& workspace_path);
    const WorkspacePaths& GetWorkspacePaths() const;

    // Formatted output path helper
    fs::path GetRawOutputPath(int game, int endpoint, int endpoint_method, const std::string& identifier = "");

    // Native file dialog helpers
    static std::string OpenFileDialog(const std::string& filter_desc = "", const std::string& filter_ext = "");
    static std::string SelectFolderDialog();

    AppConfig& GetConfig() { return config_; }
    const AppConfig& GetConfig() const { return config_; }
    bool IsLoaded() const { return config_loaded_; }

private:
    FileManager();
    ~FileManager() = default;

    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

    AppConfig config_;
    WorkspacePaths workspace_paths_;
    std::string resolved_config_path_ = "./EzrealRunes.json";
    bool config_loaded_{false};
    void UpdateWorkspacePaths();
};

} // namespace restfulEz

