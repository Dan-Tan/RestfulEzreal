#pragma once

#include "QueryForm.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <utility>

namespace restfulEz {

class TemplateManager {
public:
    static TemplateManager& GetInstance();

    // Template serialization and saving
    std::string SerializeToString(
        const std::string& name,
        const std::string& desc,
        const BatchForm& batch_form
    );

    bool SaveTemplate(
        const std::string& filepath,
        const std::string& name,
        const std::string& desc,
        const BatchForm& batch_form
    );

    // Template loading and deserialization
    bool LoadTemplateFromString(
        const std::string& json_str,
        BatchForm& batch_form
    );

    bool LoadTemplate(
        const std::string& filepath,
        BatchForm& batch_form
    );

    // Starter template JSON strings
    const std::string& GetStarterMatchHistoryTimelinesJson();
    const std::string& GetStarterParticipantExpansionJson();
    const std::string& GetStarterChallengerDatabaseJson();

    // Quick-Fill Root Parameters applicator
    bool QuickFillRootParameters(
        BatchForm& batch_form,
        const std::string& routing,
        const std::string& game_name,
        const std::string& tag_line
    );

    // Workspace synchronization
    void EnsureStarterTemplatesInWorkspace(const std::filesystem::path& templates_dir);
    std::vector<std::pair<std::string, std::filesystem::path>> ListAvailableTemplates(const std::filesystem::path& templates_dir);

private:
    TemplateManager() = default;
    ~TemplateManager() = default;
    TemplateManager(const TemplateManager&) = delete;
    TemplateManager& operator=(const TemplateManager&) = delete;
};

} // namespace restfulEz
