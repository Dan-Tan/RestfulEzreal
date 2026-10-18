#pragma once

#include <string>
#include <mutex>
#include <cstdint>
#include <optional>
#include "duckdb.h"

namespace restfulEz {

struct PlayerRecord {
    std::string player_id;
    std::string current_game_name;
    std::string current_tag_line;
    int64_t last_updated{0};
};

struct AccountRecord {
    std::string puuid;
    std::string game_name;
    std::string tag_line;
    int64_t last_updated{0};
};

struct MatchRecord {
    std::string match_id;
    int64_t game_creation{0};
    int64_t game_duration{0};
    std::string game_version;
    int32_t queue_id{0};
    std::string raw_json;
};

class DatabaseManager {
public:
    static DatabaseManager& GetInstance();

    // Initializes database file and creates required DuckDB tables
    bool Init(const std::string& db_path = "./output/db/ezreal_data.duckdb");
    bool Reopen(const std::string& new_db_path);
    void Close();

    bool IsInitialized() const { return initialized_; }
    const std::string& GetDatabasePath() const { return current_db_path_; }

    // Canonical Player Identity & PUUID Resolution
    std::string ResolveOrCreatePlayer(const std::string& dev_key_prefix, const std::string& puuid, const std::string& game_name, const std::string& tag_line);
    bool SaveAccount(const std::string& dev_key_prefix, const std::string& puuid, const std::string& game_name, const std::string& tag_line);
    bool SaveAccount(const AccountRecord& account); // Legacy overload

    // Table Operations
    bool SaveMatch(const std::string& match_id, const std::string& raw_json_str, const std::string& primary_player_id = "");
    bool SaveTimeline(const std::string& match_id, const std::string& raw_json_str);

    // Player Reconciliation
    bool MergePlayers(const std::string& target_player_id, const std::string& source_player_id);

    // Queries
    bool HasPlayerPuuid(const std::string& puuid);
    bool HasMatch(const std::string& match_id);
    bool HasTimeline(const std::string& match_id);
    int64_t GetMatchCount();
    int64_t GetTimelineCount();
    int64_t GetMatchParticipantCount(const std::string& match_id);
    std::string GetParticipantPlayerId(const std::string& match_id, int participant_id);
    int64_t GetPlayerCount();
    int64_t GetAccountCount(); // Alias for player count

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool CreateTables();

    duckdb_database db_{nullptr};
    duckdb_connection conn_{nullptr};
    mutable std::recursive_mutex db_mutex_;
    bool initialized_{false};
    std::string current_db_path_;
};

} // namespace restfulEz

