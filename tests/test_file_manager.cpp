#include "FileManager.h"
#include <iostream>
#include <filesystem>
#include <cassert>

int main() {
    std::cout << "[Test] Running FileManager unit tests..." << std::endl;

    std::string test_ws = "./test_workspace";
    std::string test_cfg = "./test_workspace/test_config.json";

    if (std::filesystem::exists(test_ws)) {
        std::filesystem::remove_all(test_ws);
    }

    restfulEz::FileManager& fm = restfulEz::FileManager::GetInstance();

    // 1. Test SetApiKey and GetDevKeyPrefix
    fm.SetApiKey("RGAPI-12345678-abcd-1234-abcd-1234567890ab");
    assert(fm.GetApiKey() == "RGAPI-12345678-abcd-1234-abcd-1234567890ab");
    assert(fm.GetDevKeyPrefix() == "RGAPI-12345678");
    std::cout << "  [PASS] ApiKey & DevKeyPrefix" << std::endl;

    // 2. Test SetWorkspace and Directory Hierarchies
    bool ws_ok = fm.SetWorkspace(test_ws);
    assert(ws_ok);
    assert(std::filesystem::exists(test_ws + "/db"));
    assert(std::filesystem::exists(test_ws + "/raw/accounts"));
    assert(std::filesystem::exists(test_ws + "/raw/matches"));
    assert(std::filesystem::exists(test_ws + "/raw/timelines"));
    assert(std::filesystem::exists(test_ws + "/logs"));
    std::cout << "  [PASS] SetWorkspace & Directory Creation" << std::endl;

    // 3. Test SaveConfig and LoadConfig
    fm.GetConfig().log_path = "custom_logs.log";
    fm.GetConfig().log_level = spdlog::level::debug;
    fm.GetConfig().verbosity = true;

    bool save_ok = fm.SaveConfig(test_cfg);
    assert(save_ok);
    assert(std::filesystem::exists(test_cfg));

    // Reset and reload
    fm.SetApiKey("OLD_KEY");
    bool load_ok = fm.LoadConfig(test_cfg);
    assert(load_ok);
    assert(fm.GetApiKey() == "RGAPI-12345678-abcd-1234-abcd-1234567890ab");
    assert(fm.GetConfig().active_workspace_path == test_ws);
    assert(fm.GetConfig().log_level == spdlog::level::debug);
    assert(fm.GetConfig().verbosity == true);
    std::cout << "  [PASS] SaveConfig & LoadConfig" << std::endl;

    // 5. Test Reliability: Empty Log Path & Workspace Defaults
    fm.GetConfig().log_path = "";
    fm.GetConfig().active_workspace_path = "";
    fm.SetApiKey("  RGAPI-trimmed-key-1234  \n");
    fm.SanitizeConfig();
    assert(fm.GetApiKey() == "RGAPI-trimmed-key-1234");
    assert(fm.GetConfig().active_workspace_path == "./output");
    assert(!fm.GetConfig().log_path.empty());
    assert(fm.GetConfig().log_path.find("ezreal.log") != std::string::npos);
    std::cout << "  [PASS] Empty path sanitization & key trimming" << std::endl;

    // 6. Test Missing Config File Loading
    bool missing_load = fm.LoadConfig("./non_existent_config_123.json");
    assert(!missing_load); // Returns false gracefully
    assert(fm.GetConfig().active_workspace_path == "./output");
    std::cout << "  [PASS] Missing config file graceful fallback" << std::endl;

    // Cleanup
    std::filesystem::remove_all(test_ws);
    std::cout << "[Test] All FileManager tests passed successfully!" << std::endl;
    return 0;
}
