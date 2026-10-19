#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <optional>
#include "duckdb.h"

namespace restfulEz {

struct QueryResult {
    bool success{false};
    std::string error_message;
    std::vector<std::string> column_names;
    std::vector<std::vector<std::string>> rows;
    int64_t row_count{0};
    double execution_time_ms{0.0};
};

struct TableCounts {
    int64_t players{0};
    int64_t matches{0};
    int64_t match_participants{0};
    int64_t match_timelines{0};
    int64_t timeline_participant_frames{0};
    int64_t timeline_events{0};
};

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
    int64_t BackfillUnnestedTimelines();
    bool InitTables();

    // Player Reconciliation
    bool MergePlayers(const std::string& target_player_id, const std::string& source_player_id);

    // Queries
    bool HasPlayerPuuid(const std::string& puuid);
    bool HasMatch(const std::string& match_id);
    bool HasTimeline(const std::string& match_id);
    int64_t GetMatchCount();
    int64_t GetTimelineCount();
    int64_t GetTimelineParticipantFrameCount();
    int64_t GetTimelineEventCount();
    int64_t GetMatchParticipantCount(const std::string& match_id);
    std::string GetParticipantPlayerId(const std::string& match_id, int participant_id);
    int64_t GetPlayerCount();
    int64_t GetAccountCount(); // Alias for player count
    bool HasMatchesForPlayerPuuid(const std::string& puuid, int min_matches = 1);

    // Analytical Query Engine & Summaries
    QueryResult ExecuteQuery(const std::string& sql);
    TableCounts GetTableCounts();
    int64_t GetParticipantCount();
    int64_t GetTimelineFrameCount();

    QueryResult GetChampionPerformanceSummary(int limit = 50);
    QueryResult GetTopTrackedPlayersSummary(int limit = 50);
    QueryResult GetTimelineFramesSummary();
    QueryResult GetTimelineEventsSummary();

    // CSV Exporters
    bool ExportQueryToCSV(const std::string& sql, const std::string& output_csv_path);
    bool ExportTableToCSV(const std::string& table_name, const std::string& output_csv_path);

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

