#include "DatabaseManager.h"
#include "simdjson.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <random>

namespace restfulEz {

DatabaseManager& DatabaseManager::GetInstance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::~DatabaseManager() {
    Close();
}

static std::string GeneratePlayerId() {
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(100000, 999999);
    return "p_" + std::to_string(now) + "_" + std::to_string(dist(rng));
}

bool DatabaseManager::CreateTables() {
    if (!conn_) return false;

    duckdb_result result;

    // 1. Players table (canonical universal identity)
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS players (
            player_id VARCHAR PRIMARY KEY,
            current_game_name VARCHAR,
            current_tag_line VARCHAR,
            created_at BIGINT,
            last_updated BIGINT
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating players table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 2. Player Aliases table (name history mapping to player_id)
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS player_aliases (
            game_name VARCHAR,
            tag_line VARCHAR,
            player_id VARCHAR,
            first_seen BIGINT,
            last_seen BIGINT,
            PRIMARY KEY (game_name, tag_line, player_id)
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating player_aliases table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 3. Developer PUUID Mappings (maps dev-encrypted PUUIDs to player_id)
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS dev_puuid_mappings (
            dev_key_prefix VARCHAR,
            encrypted_puuid VARCHAR,
            player_id VARCHAR,
            PRIMARY KEY (dev_key_prefix, encrypted_puuid)
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating dev_puuid_mappings table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 4. Matches table
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS matches (
            match_id VARCHAR PRIMARY KEY,
            game_creation BIGINT,
            game_duration BIGINT,
            game_version VARCHAR,
            queue_id INTEGER,
            raw_json JSON
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating matches table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 5. Match Timelines table
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS match_timelines (
            match_id VARCHAR PRIMARY KEY,
            frame_interval INTEGER,
            raw_json JSON
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating match_timelines table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 5b. Timeline Participant Frames table (unnested per-frame participant snapshots)
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS timeline_participant_frames (
            match_id VARCHAR,
            frame_index INTEGER,
            participant_id INTEGER,
            timestamp BIGINT,
            total_gold INTEGER,
            current_gold INTEGER,
            xp INTEGER,
            level INTEGER,
            minions_killed INTEGER,
            jungle_minions_killed INTEGER,
            pos_x INTEGER,
            pos_y INTEGER,
            total_damage_done_to_champions INTEGER,
            PRIMARY KEY (match_id, frame_index, participant_id)
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating timeline_participant_frames table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 5c. Timeline Events table (unnested per-frame analytical events)
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS timeline_events (
            match_id VARCHAR,
            frame_index INTEGER,
            event_index INTEGER,
            timestamp BIGINT,
            event_type VARCHAR,
            killer_id INTEGER,
            victim_id INTEGER,
            creator_id INTEGER,
            participant_id INTEGER,
            item_id INTEGER,
            monster_type VARCHAR,
            monster_sub_type VARCHAR,
            building_type VARCHAR,
            lane_type VARCHAR,
            team_id INTEGER,
            pos_x INTEGER,
            pos_y INTEGER,
            assisting_participant_ids VARCHAR,
            PRIMARY KEY (match_id, frame_index, event_index)
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating timeline_events table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // Indices for analytical timeline queries
    if (duckdb_query(conn_, R"(
        CREATE INDEX IF NOT EXISTS idx_timeline_events_type ON timeline_events(event_type);
        CREATE INDEX IF NOT EXISTS idx_timeline_events_match ON timeline_events(match_id);
        CREATE INDEX IF NOT EXISTS idx_timeline_pf_match ON timeline_participant_frames(match_id);
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating timeline indices: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 6. Match Participants table
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS match_participants (
            match_id VARCHAR,
            participant_id INTEGER,
            player_id VARCHAR,
            game_name VARCHAR,
            tag_line VARCHAR,
            champion_name VARCHAR,
            champion_id INTEGER,
            team_id INTEGER,
            kills INTEGER,
            deaths INTEGER,
            assists INTEGER,
            win BOOLEAN,
            PRIMARY KEY (match_id, participant_id)
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating match_participants table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    // 7. Legacy Accounts table for backwards compatibility
    if (duckdb_query(conn_, R"(
        CREATE TABLE IF NOT EXISTS accounts (
            puuid VARCHAR PRIMARY KEY,
            game_name VARCHAR,
            tag_line VARCHAR,
            last_updated BIGINT
        );
    )", &result) == DuckDBError) {
        std::cerr << "[DatabaseManager] Error creating accounts table: " << duckdb_result_error(&result) << std::endl;
        duckdb_destroy_result(&result);
        return false;
    }
    duckdb_destroy_result(&result);

    return true;
}

bool DatabaseManager::Init(const std::string& db_path) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);

    if (initialized_) {
        if (current_db_path_ == db_path) {
            return true;
        }
        // If path changed, close existing
        if (conn_) {
            duckdb_disconnect(&conn_);
            conn_ = nullptr;
        }
        if (db_) {
            duckdb_close(&db_);
            db_ = nullptr;
        }
        initialized_ = false;
    }

    try {
        std::filesystem::path p(db_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        if (duckdb_open(db_path.c_str(), &db_) == DuckDBError) {
            std::cerr << "[DatabaseManager] Failed to open DuckDB at " << db_path << std::endl;
            return false;
        }

        if (duckdb_connect(db_, &conn_) == DuckDBError) {
            std::cerr << "[DatabaseManager] Failed to connect to DuckDB" << std::endl;
            duckdb_close(&db_);
            db_ = nullptr;
            return false;
        }

        if (!CreateTables()) {
            return false;
        }

        initialized_ = true;
        current_db_path_ = db_path;
        std::cout << "[DatabaseManager] Initialized DuckDB database at: " << db_path << std::endl;
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[DatabaseManager] Failed to initialize DuckDB: " << ex.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::Reopen(const std::string& new_db_path) {
    Close();
    return Init(new_db_path);
}

void DatabaseManager::Close() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (initialized_) {
        if (conn_) {
            duckdb_disconnect(&conn_);
            conn_ = nullptr;
        }
        if (db_) {
            duckdb_close(&db_);
            db_ = nullptr;
        }
        initialized_ = false;
        current_db_path_ = "";
        std::cout << "[DatabaseManager] Closed DuckDB database connection." << std::endl;
    }
}

std::string DatabaseManager::ResolveOrCreatePlayer(const std::string& dev_key_prefix, const std::string& puuid, const std::string& game_name, const std::string& tag_line) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return "";

    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string player_id = "";

    // Tier 1: Check dev_puuid_mappings if PUUID is provided
    if (!puuid.empty()) {
        duckdb_prepared_statement stmt;
        if (duckdb_prepare(conn_, "SELECT player_id FROM dev_puuid_mappings WHERE dev_key_prefix = $1 AND encrypted_puuid = $2;", &stmt) != DuckDBError) {
            duckdb_bind_varchar(stmt, 1, dev_key_prefix.c_str());
            duckdb_bind_varchar(stmt, 2, puuid.c_str());
            duckdb_result res;
            if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
                if (duckdb_row_count(&res) > 0) {
                    char* val = duckdb_value_varchar(&res, 0, 0);
                    if (val) {
                        player_id = std::string(val);
                        duckdb_free(val);
                    }
                }
            }
            duckdb_destroy_result(&res);
            duckdb_destroy_prepare(&stmt);
        }
    }

    // Tier 2: Check player_aliases by (game_name, tag_line)
    if (player_id.empty() && !game_name.empty() && !tag_line.empty()) {
        duckdb_prepared_statement stmt;
        if (duckdb_prepare(conn_, "SELECT player_id FROM player_aliases WHERE lower(game_name) = lower($1) AND lower(tag_line) = lower($2) ORDER BY last_seen DESC LIMIT 1;", &stmt) != DuckDBError) {
            duckdb_bind_varchar(stmt, 1, game_name.c_str());
            duckdb_bind_varchar(stmt, 2, tag_line.c_str());
            duckdb_result res;
            if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
                if (duckdb_row_count(&res) > 0) {
                    char* val = duckdb_value_varchar(&res, 0, 0);
                    if (val) {
                        player_id = std::string(val);
                        duckdb_free(val);
                    }
                }
            }
            duckdb_destroy_result(&res);
            duckdb_destroy_prepare(&stmt);
        }
    }

    // Tier 3: Create new canonical player if not found
    if (player_id.empty()) {
        player_id = GeneratePlayerId();
        duckdb_prepared_statement stmt;
        if (duckdb_prepare(conn_, "INSERT INTO players (player_id, current_game_name, current_tag_line, created_at, last_updated) VALUES ($1, $2, $3, $4, $5);", &stmt) != DuckDBError) {
            duckdb_bind_varchar(stmt, 1, player_id.c_str());
            duckdb_bind_varchar(stmt, 2, game_name.c_str());
            duckdb_bind_varchar(stmt, 3, tag_line.c_str());
            duckdb_bind_int64(stmt, 4, now);
            duckdb_bind_int64(stmt, 5, now);
            duckdb_result res;
            duckdb_execute_prepared(stmt, &res);
            duckdb_destroy_result(&res);
            duckdb_destroy_prepare(&stmt);
        }
    } else {
        // Update latest name on player record
        if (!game_name.empty() && !tag_line.empty()) {
            duckdb_prepared_statement stmt;
            if (duckdb_prepare(conn_, "UPDATE players SET current_game_name = $1, current_tag_line = $2, last_updated = $3 WHERE player_id = $4;", &stmt) != DuckDBError) {
                duckdb_bind_varchar(stmt, 1, game_name.c_str());
                duckdb_bind_varchar(stmt, 2, tag_line.c_str());
                duckdb_bind_int64(stmt, 3, now);
                duckdb_bind_varchar(stmt, 4, player_id.c_str());
                duckdb_result res;
                duckdb_execute_prepared(stmt, &res);
                duckdb_destroy_result(&res);
                duckdb_destroy_prepare(&stmt);
            }
        }
    }

    // Upsert alias mapping
    if (!game_name.empty() && !tag_line.empty()) {
        duckdb_prepared_statement stmt;
        if (duckdb_prepare(conn_, "INSERT OR REPLACE INTO player_aliases (game_name, tag_line, player_id, first_seen, last_seen) VALUES ($1, $2, $3, $4, $5);", &stmt) != DuckDBError) {
            duckdb_bind_varchar(stmt, 1, game_name.c_str());
            duckdb_bind_varchar(stmt, 2, tag_line.c_str());
            duckdb_bind_varchar(stmt, 3, player_id.c_str());
            duckdb_bind_int64(stmt, 4, now);
            duckdb_bind_int64(stmt, 5, now);
            duckdb_result res;
            duckdb_execute_prepared(stmt, &res);
            duckdb_destroy_result(&res);
            duckdb_destroy_prepare(&stmt);
        }
    }

    // Upsert dev_puuid mapping if PUUID present
    if (!puuid.empty()) {
        duckdb_prepared_statement stmt;
        if (duckdb_prepare(conn_, "INSERT OR REPLACE INTO dev_puuid_mappings (dev_key_prefix, encrypted_puuid, player_id) VALUES ($1, $2, $3);", &stmt) != DuckDBError) {
            duckdb_bind_varchar(stmt, 1, dev_key_prefix.c_str());
            duckdb_bind_varchar(stmt, 2, puuid.c_str());
            duckdb_bind_varchar(stmt, 3, player_id.c_str());
            duckdb_result res;
            duckdb_execute_prepared(stmt, &res);
            duckdb_destroy_result(&res);
            duckdb_destroy_prepare(&stmt);
        }
    }

    return player_id;
}

bool DatabaseManager::SaveAccount(const std::string& dev_key_prefix, const std::string& puuid, const std::string& game_name, const std::string& tag_line) {
    std::string pid = ResolveOrCreatePlayer(dev_key_prefix, puuid, game_name, tag_line);
    if (pid.empty()) return false;

    // Legacy table insert
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return false;

    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, "INSERT OR REPLACE INTO accounts VALUES ($1, $2, $3, $4);", &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return false;
    }
    duckdb_bind_varchar(stmt, 1, puuid.c_str());
    duckdb_bind_varchar(stmt, 2, game_name.c_str());
    duckdb_bind_varchar(stmt, 3, tag_line.c_str());
    duckdb_bind_int64(stmt, 4, now);

    duckdb_result res;
    bool success = (duckdb_execute_prepared(stmt, &res) != DuckDBError);
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return success;
}

bool DatabaseManager::SaveAccount(const AccountRecord& account) {
    return SaveAccount("DEFAULT_DEV", account.puuid, account.game_name, account.tag_line);
}

bool DatabaseManager::MergePlayers(const std::string& target_player_id, const std::string& source_player_id) {
    if (target_player_id == source_player_id || source_player_id.empty() || target_player_id.empty()) {
        return true;
    }
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return false;

    duckdb_result res;
    // 1. Delete overlapping aliases on source before re-pointing remaining player_aliases
    duckdb_prepared_statement stmt0;
    if (duckdb_prepare(conn_, R"(
        DELETE FROM player_aliases
        WHERE player_id = $2
          AND EXISTS (
              SELECT 1 FROM player_aliases t
              WHERE t.player_id = $1
                AND t.game_name = player_aliases.game_name
                AND t.tag_line = player_aliases.tag_line
          );
    )", &stmt0) != DuckDBError) {
        duckdb_bind_varchar(stmt0, 1, target_player_id.c_str());
        duckdb_bind_varchar(stmt0, 2, source_player_id.c_str());
        duckdb_execute_prepared(stmt0, &res);
        duckdb_destroy_result(&res);
        duckdb_destroy_prepare(&stmt0);
    }

    duckdb_prepared_statement stmt1;
    if (duckdb_prepare(conn_, "UPDATE player_aliases SET player_id = $1 WHERE player_id = $2;", &stmt1) != DuckDBError) {
        duckdb_bind_varchar(stmt1, 1, target_player_id.c_str());
        duckdb_bind_varchar(stmt1, 2, source_player_id.c_str());
        duckdb_execute_prepared(stmt1, &res);
        duckdb_destroy_result(&res);
        duckdb_destroy_prepare(&stmt1);
    }

    // 2. Re-point dev_puuid_mappings
    duckdb_prepared_statement stmt2;
    if (duckdb_prepare(conn_, "UPDATE dev_puuid_mappings SET player_id = $1 WHERE player_id = $2;", &stmt2) != DuckDBError) {
        duckdb_bind_varchar(stmt2, 1, target_player_id.c_str());
        duckdb_bind_varchar(stmt2, 2, source_player_id.c_str());
        duckdb_execute_prepared(stmt2, &res);
        duckdb_destroy_result(&res);
        duckdb_destroy_prepare(&stmt2);
    }

    // 3. Re-point match_participants
    duckdb_prepared_statement stmt3;
    if (duckdb_prepare(conn_, "UPDATE match_participants SET player_id = $1 WHERE player_id = $2;", &stmt3) != DuckDBError) {
        duckdb_bind_varchar(stmt3, 1, target_player_id.c_str());
        duckdb_bind_varchar(stmt3, 2, source_player_id.c_str());
        duckdb_execute_prepared(stmt3, &res);
        duckdb_destroy_result(&res);
        duckdb_destroy_prepare(&stmt3);
    }

    // 4. Delete old source player
    duckdb_prepared_statement stmt4;
    if (duckdb_prepare(conn_, "DELETE FROM players WHERE player_id = $1;", &stmt4) != DuckDBError) {
        duckdb_bind_varchar(stmt4, 1, source_player_id.c_str());
        duckdb_execute_prepared(stmt4, &res);
        duckdb_destroy_result(&res);
        duckdb_destroy_prepare(&stmt4);
    }

    std::cout << "[DatabaseManager] Auto-merged player " << source_player_id << " into " << target_player_id << std::endl;
    return true;
}

bool DatabaseManager::SaveMatch(const std::string& match_id, const std::string& raw_json_str, const std::string& primary_player_id) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_ || raw_json_str.empty()) return false;

    int64_t game_creation = 0;
    int64_t game_duration = 0;
    std::string game_version = "";
    int32_t queue_id = 0;

    struct ParticipantRow {
        int participant_id{0};
        std::string puuid;
        std::string player_id;
        std::string game_name;
        std::string tag_line;
        std::string champion_name;
        int champion_id{0};
        int team_id{0};
        int kills{0};
        int deaths{0};
        int assists{0};
        bool win{false};
    };
    std::string actual_match_id = match_id;
    std::vector<ParticipantRow> participants;

    try {
        simdjson::padded_string padded(raw_json_str.data(), raw_json_str.size());
        simdjson::dom::parser parser;
        simdjson::dom::element doc;
        if (parser.parse(padded).get(doc) != simdjson::SUCCESS) {
            return false;
        }
        // Reject error responses
        if (doc["status"].error() == simdjson::SUCCESS) {
            return false;
        }
        simdjson::dom::element info;
        if (doc["info"].get(info) != simdjson::SUCCESS) {
            return false;
        }

        std::string_view mid_sv;
        if (doc["metadata"]["matchId"].get(mid_sv) == simdjson::SUCCESS && !mid_sv.empty()) {
            actual_match_id = std::string(mid_sv);
        }

        int64_t val_int = 0;
        if (info["gameCreation"].get(val_int) == simdjson::SUCCESS) {
            game_creation = val_int;
        }
        if (info["gameDuration"].get(val_int) == simdjson::SUCCESS) {
            game_duration = val_int;
        }
        std::string_view gv_sv;
        if (info["gameVersion"].get(gv_sv) == simdjson::SUCCESS) {
            game_version = std::string(gv_sv);
        }
        if (info["queueId"].get(val_int) == simdjson::SUCCESS) {
            queue_id = static_cast<int32_t>(val_int);
        }

        // Parse participants
        simdjson::dom::array parts_arr;
        if (info["participants"].get(parts_arr) == simdjson::SUCCESS) {
            for (simdjson::dom::element p : parts_arr) {
                ParticipantRow row;
                int64_t pid_val = 0;
                if (p["participantId"].get(pid_val) == simdjson::SUCCESS) {
                    row.participant_id = static_cast<int>(pid_val);
                }
                std::string_view sv;
                if (p["puuid"].get(sv) == simdjson::SUCCESS) {
                    row.puuid = std::string(sv);
                }
                if (p["riotIdGameName"].get(sv) == simdjson::SUCCESS) {
                    row.game_name = std::string(sv);
                } else if (p["summonerName"].get(sv) == simdjson::SUCCESS) {
                    row.game_name = std::string(sv);
                }
                if (p["riotIdTagline"].get(sv) == simdjson::SUCCESS) {
                    row.tag_line = std::string(sv);
                }
                if (p["championName"].get(sv) == simdjson::SUCCESS) {
                    row.champion_name = std::string(sv);
                }
                if (p["championId"].get(pid_val) == simdjson::SUCCESS) {
                    row.champion_id = static_cast<int>(pid_val);
                }
                if (p["teamId"].get(pid_val) == simdjson::SUCCESS) {
                    row.team_id = static_cast<int>(pid_val);
                }
                if (p["kills"].get(pid_val) == simdjson::SUCCESS) {
                    row.kills = static_cast<int>(pid_val);
                }
                if (p["deaths"].get(pid_val) == simdjson::SUCCESS) {
                    row.deaths = static_cast<int>(pid_val);
                }
                if (p["assists"].get(pid_val) == simdjson::SUCCESS) {
                    row.assists = static_cast<int>(pid_val);
                }
                bool b_val = false;
                if (p["win"].get(b_val) == simdjson::SUCCESS) {
                    row.win = b_val;
                }
                participants.push_back(std::move(row));
            }
        }
    } catch (...) {
        return false;
    }

    // 1. Insert into matches table
    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, R"(
        INSERT OR REPLACE INTO matches (match_id, game_creation, game_duration, game_version, queue_id, raw_json)
        VALUES ($1, $2, $3, $4, $5, $6);
    )", &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return false;
    }

    duckdb_bind_varchar(stmt, 1, actual_match_id.c_str());
    duckdb_bind_int64(stmt, 2, game_creation);
    duckdb_bind_int64(stmt, 3, game_duration);
    duckdb_bind_varchar(stmt, 4, game_version.c_str());
    duckdb_bind_int32(stmt, 5, queue_id);
    duckdb_bind_varchar(stmt, 6, raw_json_str.c_str());

    duckdb_result res;
    bool success = (duckdb_execute_prepared(stmt, &res) != DuckDBError);
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);

    if (!success) return false;

    // 2. Insert participant rows, resolving known player_id via PUUID or Riot ID alias
    for (const auto& p : participants) {
        std::string p_player_id = p.player_id;

        if (p_player_id.empty() && !p.puuid.empty()) {
            duckdb_prepared_statement lstmt;
            if (duckdb_prepare(conn_, "SELECT player_id FROM dev_puuid_mappings WHERE encrypted_puuid = $1 LIMIT 1;", &lstmt) != DuckDBError) {
                duckdb_bind_varchar(lstmt, 1, p.puuid.c_str());
                duckdb_result lres;
                if (duckdb_execute_prepared(lstmt, &lres) != DuckDBError && duckdb_row_count(&lres) > 0) {
                    char* val = duckdb_value_varchar(&lres, 0, 0);
                    if (val) {
                        p_player_id = std::string(val);
                        duckdb_free(val);
                    }
                }
                duckdb_destroy_result(&lres);
                duckdb_destroy_prepare(&lstmt);
            }
        }

        if (p_player_id.empty() && !p.game_name.empty() && !p.tag_line.empty()) {
            duckdb_prepared_statement lstmt;
            if (duckdb_prepare(conn_, "SELECT player_id FROM player_aliases WHERE lower(game_name) = lower($1) AND lower(tag_line) = lower($2) ORDER BY last_seen DESC LIMIT 1;", &lstmt) != DuckDBError) {
                duckdb_bind_varchar(lstmt, 1, p.game_name.c_str());
                duckdb_bind_varchar(lstmt, 2, p.tag_line.c_str());
                duckdb_result lres;
                if (duckdb_execute_prepared(lstmt, &lres) != DuckDBError && duckdb_row_count(&lres) > 0) {
                    char* val = duckdb_value_varchar(&lres, 0, 0);
                    if (val) {
                        p_player_id = std::string(val);
                        duckdb_free(val);
                    }
                }
                duckdb_destroy_result(&lres);
                duckdb_destroy_prepare(&lstmt);
            }
        }

        if (p_player_id.empty() && !primary_player_id.empty() && (p.participant_id == 1)) {
            p_player_id = primary_player_id;
        }

        duckdb_prepared_statement pstmt;
        if (duckdb_prepare(conn_, R"(
            INSERT OR REPLACE INTO match_participants 
            (match_id, participant_id, player_id, game_name, tag_line, champion_name, champion_id, team_id, kills, deaths, assists, win)
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12);
        )", &pstmt) != DuckDBError) {
            duckdb_bind_varchar(pstmt, 1, actual_match_id.c_str());
            duckdb_bind_int32(pstmt, 2, p.participant_id);
            duckdb_bind_varchar(pstmt, 3, p_player_id.c_str());
            duckdb_bind_varchar(pstmt, 4, p.game_name.c_str());
            duckdb_bind_varchar(pstmt, 5, p.tag_line.c_str());
            duckdb_bind_varchar(pstmt, 6, p.champion_name.c_str());
            duckdb_bind_int32(pstmt, 7, p.champion_id);
            duckdb_bind_int32(pstmt, 8, p.team_id);
            duckdb_bind_int32(pstmt, 9, p.kills);
            duckdb_bind_int32(pstmt, 10, p.deaths);
            duckdb_bind_int32(pstmt, 11, p.assists);
            duckdb_bind_boolean(pstmt, 12, p.win);

            duckdb_result pres;
            duckdb_execute_prepared(pstmt, &pres);
            duckdb_destroy_result(&pres);
            duckdb_destroy_prepare(&pstmt);
        }
    }

    return true;
}

bool DatabaseManager::SaveTimeline(const std::string& match_id, const std::string& raw_json_str) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_ || raw_json_str.empty()) return false;

    std::string actual_match_id = match_id;
    int32_t frame_interval = 0;
    simdjson::dom::parser parser;
    simdjson::padded_string padded(raw_json_str.data(), raw_json_str.size());
    simdjson::dom::element doc;

    try {
        if (parser.parse(padded).get(doc) != simdjson::SUCCESS) {
            std::cerr << "[DatabaseManager] SaveTimeline parse failed" << std::endl;
            return false;
        }
        if (doc["status"].error() == simdjson::SUCCESS) {
            std::cerr << "[DatabaseManager] SaveTimeline doc status error" << std::endl;
            return false;
        }
        simdjson::dom::element info;
        if (doc["info"].get(info) != simdjson::SUCCESS) {
            std::cerr << "[DatabaseManager] SaveTimeline no info" << std::endl;
            return false;
        }
        std::string_view mid_sv;
        if (doc["metadata"]["matchId"].get(mid_sv) == simdjson::SUCCESS && !mid_sv.empty()) {
            actual_match_id = std::string(mid_sv);
        }
        int64_t val = 0;
        if (info["frameInterval"].get(val) == simdjson::SUCCESS) {
            frame_interval = static_cast<int32_t>(val);
        }
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] SaveTimeline json exception: " << e.what() << std::endl;
        return false;
    }

    if (actual_match_id.empty()) {
        std::cerr << "[DatabaseManager] SaveTimeline actual_match_id empty" << std::endl;
        return false;
    }

    simdjson::dom::array frames;
    bool has_frames = (doc["info"]["frames"].get(frames) == simdjson::SUCCESS && frames.size() > 0);

    if (has_frames) {
        // Idempotency: delete old frames and events for this match before transaction
        duckdb_prepared_statement del_stmt;
        if (duckdb_prepare(conn_, "DELETE FROM timeline_participant_frames WHERE match_id = $1;", &del_stmt) != DuckDBError) {
            duckdb_bind_varchar(del_stmt, 1, actual_match_id.c_str());
            duckdb_result del_res;
            duckdb_execute_prepared(del_stmt, &del_res);
            duckdb_destroy_result(&del_res);
            duckdb_destroy_prepare(&del_stmt);
        }
        if (duckdb_prepare(conn_, "DELETE FROM timeline_events WHERE match_id = $1;", &del_stmt) != DuckDBError) {
            duckdb_bind_varchar(del_stmt, 1, actual_match_id.c_str());
            duckdb_result del_res;
            duckdb_execute_prepared(del_stmt, &del_res);
            duckdb_destroy_result(&del_res);
            duckdb_destroy_prepare(&del_stmt);
        }
    }

    duckdb_result res;
    if (duckdb_query(conn_, "BEGIN TRANSACTION;", &res) == DuckDBError) {
        std::cerr << "[DatabaseManager] SaveTimeline BEGIN TRANSACTION failed: " << duckdb_result_error(&res) << std::endl;
        duckdb_destroy_result(&res);
        return false;
    }
    duckdb_destroy_result(&res);

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, R"(
        INSERT OR REPLACE INTO match_timelines (match_id, frame_interval, raw_json) 
        VALUES ($1, $2, $3);
    )", &stmt) == DuckDBError) {
        std::cerr << "[DatabaseManager] Prepare error in SaveTimeline: " << duckdb_prepare_error(stmt) << std::endl;
        duckdb_destroy_prepare(&stmt);
        duckdb_query(conn_, "ROLLBACK;", nullptr);
        return false;
    }

    duckdb_bind_varchar(stmt, 1, actual_match_id.c_str());
    duckdb_bind_int32(stmt, 2, frame_interval);
    duckdb_bind_varchar(stmt, 3, raw_json_str.c_str());

    bool success = (duckdb_execute_prepared(stmt, &res) != DuckDBError);
    if (!success) {
        std::cerr << "[DatabaseManager] SaveTimeline execute error: " << duckdb_result_error(&res) << std::endl;
        duckdb_destroy_result(&res);
        duckdb_destroy_prepare(&stmt);
        duckdb_query(conn_, "ROLLBACK;", nullptr);
        return false;
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);

    if (has_frames) {

        duckdb_appender pf_appender = nullptr;
        if (duckdb_appender_create(conn_, nullptr, "timeline_participant_frames", &pf_appender) == DuckDBError) {
            std::cerr << "[DatabaseManager] Failed to create pf_appender: " << duckdb_appender_error(pf_appender) << std::endl;
            duckdb_appender_destroy(&pf_appender);
            duckdb_query(conn_, "ROLLBACK;", nullptr);
            return false;
        }

        duckdb_appender ev_appender = nullptr;
        if (duckdb_appender_create(conn_, nullptr, "timeline_events", &ev_appender) == DuckDBError) {
            std::cerr << "[DatabaseManager] Failed to create ev_appender: " << duckdb_appender_error(ev_appender) << std::endl;
            duckdb_appender_destroy(&ev_appender);
            duckdb_appender_destroy(&pf_appender);
            duckdb_query(conn_, "ROLLBACK;", nullptr);
            return false;
        }

        try {
            int32_t frame_index = 0;
            for (auto frame_val : frames) {
                simdjson::dom::element frame;
                if (frame_val.get(frame) != simdjson::SUCCESS) {
                    frame_index++;
                    continue;
                }

                int64_t frame_timestamp = 0;
                int64_t ts_val = 0;
                if (frame["timestamp"].get(ts_val) == simdjson::SUCCESS) {
                    frame_timestamp = ts_val;
                }

                // Participant Frames
                simdjson::dom::element pf_elem;
                if (frame["participantFrames"].get(pf_elem) == simdjson::SUCCESS) {
                    simdjson::dom::object pf_obj;
                    if (pf_elem.get(pf_obj) == simdjson::SUCCESS) {
                        for (auto field : pf_obj) {
                            simdjson::dom::element pframe = field.value;
                            int32_t participant_id = 0;
                            int64_t pid_val = 0;
                            if (pframe["participantId"].get(pid_val) == simdjson::SUCCESS) {
                                participant_id = static_cast<int32_t>(pid_val);
                            } else {
                                try {
                                    participant_id = std::stoi(std::string(field.key));
                                } catch (...) {
                                    participant_id = 0;
                                }
                            }

                            int32_t total_gold = 0;
                            int64_t tg = 0;
                            if (pframe["totalGold"].get(tg) == simdjson::SUCCESS) total_gold = static_cast<int32_t>(tg);

                            int32_t current_gold = 0;
                            int64_t cg = 0;
                            if (pframe["currentGold"].get(cg) == simdjson::SUCCESS) current_gold = static_cast<int32_t>(cg);

                            int32_t xp = 0;
                            int64_t xp_val = 0;
                            if (pframe["xp"].get(xp_val) == simdjson::SUCCESS) xp = static_cast<int32_t>(xp_val);

                            int32_t level = 0;
                            int64_t lvl = 0;
                            if (pframe["level"].get(lvl) == simdjson::SUCCESS) level = static_cast<int32_t>(lvl);

                            int32_t minions_killed = 0;
                            int64_t mk = 0;
                            if (pframe["minionsKilled"].get(mk) == simdjson::SUCCESS) minions_killed = static_cast<int32_t>(mk);

                            int32_t jungle_minions_killed = 0;
                            int64_t jmk = 0;
                            if (pframe["jungleMinionsKilled"].get(jmk) == simdjson::SUCCESS) jungle_minions_killed = static_cast<int32_t>(jmk);

                            int32_t pos_x = 0;
                            int32_t pos_y = 0;
                            simdjson::dom::element pos;
                            if (pframe["position"].get(pos) == simdjson::SUCCESS) {
                                int64_t x_val = 0, y_val = 0;
                                if (pos["x"].get(x_val) == simdjson::SUCCESS) pos_x = static_cast<int32_t>(x_val);
                                if (pos["y"].get(y_val) == simdjson::SUCCESS) pos_y = static_cast<int32_t>(y_val);
                            }

                            int32_t total_damage = 0;
                            simdjson::dom::element dmg;
                            if (pframe["damageStats"].get(dmg) == simdjson::SUCCESS) {
                                int64_t dmg_val = 0;
                                if (dmg["totalDamageDoneToChampions"].get(dmg_val) == simdjson::SUCCESS) {
                                    total_damage = static_cast<int32_t>(dmg_val);
                                }
                            }

                            duckdb_append_varchar(pf_appender, actual_match_id.c_str());
                            duckdb_append_int32(pf_appender, frame_index);
                            duckdb_append_int32(pf_appender, participant_id);
                            duckdb_append_int64(pf_appender, frame_timestamp);
                            duckdb_append_int32(pf_appender, total_gold);
                            duckdb_append_int32(pf_appender, current_gold);
                            duckdb_append_int32(pf_appender, xp);
                            duckdb_append_int32(pf_appender, level);
                            duckdb_append_int32(pf_appender, minions_killed);
                            duckdb_append_int32(pf_appender, jungle_minions_killed);
                            duckdb_append_int32(pf_appender, pos_x);
                            duckdb_append_int32(pf_appender, pos_y);
                            duckdb_append_int32(pf_appender, total_damage);
                            duckdb_appender_end_row(pf_appender);
                        }
                    }
                }

                // Events
                simdjson::dom::element ev_elem;
                if (frame["events"].get(ev_elem) == simdjson::SUCCESS) {
                    simdjson::dom::array ev_arr;
                    if (ev_elem.get(ev_arr) == simdjson::SUCCESS) {
                        int32_t event_index = 0;
                        for (auto event_val : ev_arr) {
                            simdjson::dom::element ev;
                            if (event_val.get(ev) != simdjson::SUCCESS) continue;

                            std::string_view type_sv;
                            if (ev["type"].get(type_sv) != simdjson::SUCCESS || type_sv.empty()) {
                                continue;
                            }
                            std::string event_type(type_sv);

                            int64_t ev_timestamp = frame_timestamp;
                            int64_t ev_ts = 0;
                            if (ev["timestamp"].get(ev_ts) == simdjson::SUCCESS) {
                                ev_timestamp = ev_ts;
                            }

                            int32_t killer_id = 0;
                            int64_t kid = 0;
                            if (ev["killerId"].get(kid) == simdjson::SUCCESS) killer_id = static_cast<int32_t>(kid);

                            int32_t victim_id = 0;
                            int64_t vid = 0;
                            if (ev["victimId"].get(vid) == simdjson::SUCCESS) victim_id = static_cast<int32_t>(vid);

                            int32_t creator_id = 0;
                            int64_t cid = 0;
                            if (ev["creatorId"].get(cid) == simdjson::SUCCESS) creator_id = static_cast<int32_t>(cid);

                            int32_t participant_id = 0;
                            int64_t pid = 0;
                            if (ev["participantId"].get(pid) == simdjson::SUCCESS) participant_id = static_cast<int32_t>(pid);

                            int32_t item_id = 0;
                            int64_t iid = 0;
                            if (ev["itemId"].get(iid) == simdjson::SUCCESS) {
                                item_id = static_cast<int32_t>(iid);
                            } else if (ev["afterId"].get(iid) == simdjson::SUCCESS) {
                                item_id = static_cast<int32_t>(iid);
                            } else if (ev["beforeId"].get(iid) == simdjson::SUCCESS) {
                                item_id = static_cast<int32_t>(iid);
                            }

                            std::string monster_type = "";
                            std::string_view mt_sv;
                            if (ev["monsterType"].get(mt_sv) == simdjson::SUCCESS) monster_type = std::string(mt_sv);

                            std::string monster_sub_type = "";
                            std::string_view mst_sv;
                            if (ev["monsterSubType"].get(mst_sv) == simdjson::SUCCESS) monster_sub_type = std::string(mst_sv);

                            std::string building_type = "";
                            std::string_view bt_sv;
                            if (ev["buildingType"].get(bt_sv) == simdjson::SUCCESS) building_type = std::string(bt_sv);

                            std::string lane_type = "";
                            std::string_view lt_sv;
                            if (ev["laneType"].get(lt_sv) == simdjson::SUCCESS) lane_type = std::string(lt_sv);

                            int32_t team_id = 0;
                            int64_t tid = 0;
                            if (ev["teamId"].get(tid) == simdjson::SUCCESS) {
                                team_id = static_cast<int32_t>(tid);
                            } else if (ev["killerTeamId"].get(tid) == simdjson::SUCCESS) {
                                team_id = static_cast<int32_t>(tid);
                            }

                            int32_t pos_x = 0;
                            int32_t pos_y = 0;
                            simdjson::dom::element pos;
                            if (ev["position"].get(pos) == simdjson::SUCCESS) {
                                int64_t x_val = 0, y_val = 0;
                                if (pos["x"].get(x_val) == simdjson::SUCCESS) pos_x = static_cast<int32_t>(x_val);
                                if (pos["y"].get(y_val) == simdjson::SUCCESS) pos_y = static_cast<int32_t>(y_val);
                            }

                            std::string assisting_participant_ids = "";
                            simdjson::dom::element assist_elem;
                            if (ev["assistingParticipantIds"].get(assist_elem) == simdjson::SUCCESS) {
                                simdjson::dom::array assist_arr;
                                if (assist_elem.get(assist_arr) == simdjson::SUCCESS) {
                                    bool first = true;
                                    for (auto a_val : assist_arr) {
                                        int64_t aid = 0;
                                        if (a_val.get(aid) == simdjson::SUCCESS) {
                                            if (!first) assisting_participant_ids += ",";
                                            assisting_participant_ids += std::to_string(aid);
                                            first = false;
                                        }
                                    }
                                }
                            }

                            duckdb_append_varchar(ev_appender, actual_match_id.c_str());
                            duckdb_append_int32(ev_appender, frame_index);
                            duckdb_append_int32(ev_appender, event_index++);
                            duckdb_append_int64(ev_appender, ev_timestamp);
                            duckdb_append_varchar(ev_appender, event_type.c_str());
                            duckdb_append_int32(ev_appender, killer_id);
                            duckdb_append_int32(ev_appender, victim_id);
                            duckdb_append_int32(ev_appender, creator_id);
                            duckdb_append_int32(ev_appender, participant_id);
                            duckdb_append_int32(ev_appender, item_id);
                            duckdb_append_varchar(ev_appender, monster_type.c_str());
                            duckdb_append_varchar(ev_appender, monster_sub_type.c_str());
                            duckdb_append_varchar(ev_appender, building_type.c_str());
                            duckdb_append_varchar(ev_appender, lane_type.c_str());
                            duckdb_append_int32(ev_appender, team_id);
                            duckdb_append_int32(ev_appender, pos_x);
                            duckdb_append_int32(ev_appender, pos_y);
                            duckdb_append_varchar(ev_appender, assisting_participant_ids.c_str());
                            duckdb_appender_end_row(ev_appender);
                        }
                    }
                }

                frame_index++;
            }
        } catch (const std::exception& ex) {
            std::cerr << "[DatabaseManager] Exception while unnesting timeline: " << ex.what() << std::endl;
            duckdb_appender_destroy(&pf_appender);
            duckdb_appender_destroy(&ev_appender);
            duckdb_query(conn_, "ROLLBACK;", nullptr);
            return false;
        }

        bool pf_close_ok = (duckdb_appender_close(pf_appender) != DuckDBError);
        if (!pf_close_ok) {
            std::cerr << "[DatabaseManager] pf_appender close error: " << duckdb_appender_error(pf_appender) << std::endl;
        }
        duckdb_appender_destroy(&pf_appender);

        bool ev_close_ok = (duckdb_appender_close(ev_appender) != DuckDBError);
        if (!ev_close_ok) {
            std::cerr << "[DatabaseManager] ev_appender close error: " << duckdb_appender_error(ev_appender) << std::endl;
        }
        duckdb_appender_destroy(&ev_appender);

        if (!pf_close_ok || !ev_close_ok) {
            duckdb_query(conn_, "ROLLBACK;", nullptr);
            return false;
        }
    }

    if (duckdb_query(conn_, "COMMIT;", &res) == DuckDBError) {
        std::cerr << "[DatabaseManager] Commit error in SaveTimeline: " << duckdb_result_error(&res) << std::endl;
        duckdb_destroy_result(&res);
        duckdb_query(conn_, "ROLLBACK;", nullptr);
        return false;
    }
    duckdb_destroy_result(&res);

    return true;
}

bool DatabaseManager::HasPlayerPuuid(const std::string& puuid) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_ || puuid.empty()) return false;

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, "SELECT COUNT(*) FROM dev_puuid_mappings WHERE encrypted_puuid = $1;", &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return false;
    }
    duckdb_bind_varchar(stmt, 1, puuid.c_str());

    duckdb_result res;
    bool found = false;
    if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            int64_t count = duckdb_value_int64(&res, 0, 0);
            found = (count > 0);
        }
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return found;
}

bool DatabaseManager::HasMatch(const std::string& match_id) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return false;

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, "SELECT COUNT(*) FROM matches WHERE match_id = $1;", &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return false;
    }
    duckdb_bind_varchar(stmt, 1, match_id.c_str());

    duckdb_result res;
    bool found = false;
    if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            int64_t count = duckdb_value_int64(&res, 0, 0);
            found = (count > 0);
        }
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return found;
}

bool DatabaseManager::HasTimeline(const std::string& match_id) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return false;

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, "SELECT COUNT(*) FROM match_timelines WHERE match_id = $1;", &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return false;
    }
    duckdb_bind_varchar(stmt, 1, match_id.c_str());

    duckdb_result res;
    bool found = false;
    if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            int64_t count = duckdb_value_int64(&res, 0, 0);
            found = (count > 0);
        }
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return found;
}

int64_t DatabaseManager::GetMatchCount() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;

    duckdb_result res;
    int64_t count = 0;
    if (duckdb_query(conn_, "SELECT COUNT(*) FROM matches;", &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            count = duckdb_value_int64(&res, 0, 0);
        }
    }
    duckdb_destroy_result(&res);
    return count;
}

int64_t DatabaseManager::GetTimelineCount() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;

    duckdb_result res;
    int64_t count = 0;
    if (duckdb_query(conn_, "SELECT COUNT(*) FROM match_timelines;", &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            count = duckdb_value_int64(&res, 0, 0);
        }
    }
    duckdb_destroy_result(&res);
    return count;
}

int64_t DatabaseManager::GetMatchParticipantCount(const std::string& match_id) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, "SELECT COUNT(*) FROM match_participants WHERE match_id = $1;", &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return 0;
    }
    duckdb_bind_varchar(stmt, 1, match_id.c_str());

    duckdb_result res;
    int64_t count = 0;
    if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            count = duckdb_value_int64(&res, 0, 0);
        }
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return count;
}

std::string DatabaseManager::GetParticipantPlayerId(const std::string& match_id, int participant_id) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return "";

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, "SELECT player_id FROM match_participants WHERE match_id = $1 AND participant_id = $2;", &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return "";
    }
    duckdb_bind_varchar(stmt, 1, match_id.c_str());
    duckdb_bind_int32(stmt, 2, participant_id);

    duckdb_result res;
    std::string pid = "";
    if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            char* val = duckdb_value_varchar(&res, 0, 0);
            if (val) {
                pid = std::string(val);
                duckdb_free(val);
            }
        }
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return pid;
}

int64_t DatabaseManager::GetPlayerCount() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;

    duckdb_result res;
    int64_t count = 0;
    if (duckdb_query(conn_, "SELECT COUNT(*) FROM players;", &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            count = duckdb_value_int64(&res, 0, 0);
        }
    }
    duckdb_destroy_result(&res);
    return count;
}

int64_t DatabaseManager::GetAccountCount() {
    return GetPlayerCount();
}

bool DatabaseManager::HasMatchesForPlayerPuuid(const std::string& puuid, int min_matches) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_ || puuid.empty()) return false;
    if (min_matches <= 0) return true;

    duckdb_prepared_statement stmt;
    const char* sql = R"(
        SELECT COUNT(DISTINCT mp.match_id)
        FROM match_participants mp
        WHERE mp.player_id = $1
           OR mp.player_id IN (
               SELECT player_id FROM dev_puuid_mappings WHERE encrypted_puuid = $2
           );
    )";
    if (duckdb_prepare(conn_, sql, &stmt) == DuckDBError) {
        duckdb_destroy_prepare(&stmt);
        return false;
    }
    duckdb_bind_varchar(stmt, 1, puuid.c_str());
    duckdb_bind_varchar(stmt, 2, puuid.c_str());

    duckdb_result res;
    bool has_matches = false;
    if (duckdb_execute_prepared(stmt, &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            int64_t count = duckdb_value_int64(&res, 0, 0);
            has_matches = (count >= min_matches);
        }
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return has_matches;
}

bool DatabaseManager::InitTables() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return false;
    return CreateTables();
}

int64_t DatabaseManager::GetTimelineParticipantFrameCount() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;

    duckdb_result res;
    int64_t count = 0;
    if (duckdb_query(conn_, "SELECT COUNT(*) FROM timeline_participant_frames;", &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            count = duckdb_value_int64(&res, 0, 0);
        }
    }
    duckdb_destroy_result(&res);
    return count;
}

int64_t DatabaseManager::GetTimelineEventCount() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;

    duckdb_result res;
    int64_t count = 0;
    if (duckdb_query(conn_, "SELECT COUNT(*) FROM timeline_events;", &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) {
            count = duckdb_value_int64(&res, 0, 0);
        }
    }
    duckdb_destroy_result(&res);
    return count;
}

int64_t DatabaseManager::BackfillUnnestedTimelines() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;

    const char* sql = R"(
        SELECT mt.match_id, mt.raw_json
        FROM match_timelines mt
        LEFT JOIN (SELECT DISTINCT match_id FROM timeline_participant_frames) pf 
            ON mt.match_id = pf.match_id
        WHERE pf.match_id IS NULL;
    )";

    std::vector<std::pair<std::string, std::string>> candidates;
    std::vector<std::string> seen_mids;

    auto collect_candidates = [&](duckdb_connection c) {
        duckdb_result res;
        if (duckdb_query(c, sql, &res) != DuckDBError) {
            idx_t row_count = duckdb_row_count(&res);
            for (idx_t i = 0; i < row_count; ++i) {
                char* mid_c = duckdb_value_varchar(&res, 0, i);
                char* raw_c = duckdb_value_varchar(&res, 1, i);
                std::string mid = mid_c ? mid_c : "";
                std::string raw = raw_c ? raw_c : "";
                if (mid_c) duckdb_free(mid_c);
                if (raw_c) duckdb_free(raw_c);
                if (!mid.empty() && !raw.empty()) {
                    bool already_seen = false;
                    for (const auto& s : seen_mids) {
                        if (s == mid) { already_seen = true; break; }
                    }
                    if (!already_seen) {
                        seen_mids.push_back(mid);
                        candidates.emplace_back(std::move(mid), std::move(raw));
                    }
                }
            }
            duckdb_destroy_result(&res);
        }
    };

    // 1. Collect from active connection
    collect_candidates(conn_);

    // 2. Also collect from disk if external writers inserted rows (e.g. legacy migrations)
    if (!current_db_path_.empty()) {
        duckdb_database ext_db;
        duckdb_config cfg;
        duckdb_create_config(&cfg);
        duckdb_set_config(cfg, "access_mode", "READ_ONLY");
        if (duckdb_open_ext(current_db_path_.c_str(), &ext_db, cfg, nullptr) != DuckDBError) {
            duckdb_connection ext_conn;
            if (duckdb_connect(ext_db, &ext_conn) != DuckDBError) {
                collect_candidates(ext_conn);
                duckdb_disconnect(&ext_conn);
            }
            duckdb_close(&ext_db);
        }
        duckdb_destroy_config(&cfg);
    }

    int64_t backfilled_count = 0;
    simdjson::dom::parser parser;
    for (const auto& [mid, raw] : candidates) {
        // Check if conn_ already has participant frames for this match
        duckdb_prepared_statement check_stmt;
        if (duckdb_prepare(conn_, "SELECT COUNT(*) FROM timeline_participant_frames WHERE match_id = $1;", &check_stmt) == DuckDBError) {
            duckdb_destroy_prepare(&check_stmt);
            continue;
        }
        duckdb_bind_varchar(check_stmt, 1, mid.c_str());
        duckdb_result chk_res;
        bool already_has_pf = false;
        if (duckdb_execute_prepared(check_stmt, &chk_res) != DuckDBError) {
            if (duckdb_row_count(&chk_res) > 0 && duckdb_value_int64(&chk_res, 0, 0) > 0) {
                already_has_pf = true;
            }
        }
        duckdb_destroy_result(&chk_res);
        duckdb_destroy_prepare(&check_stmt);
        if (already_has_pf) continue;

        // Check that raw JSON actually has frames to unnest
        try {
            simdjson::padded_string padded(raw.data(), raw.size());
            simdjson::dom::element doc;
            if (parser.parse(padded).get(doc) != simdjson::SUCCESS) continue;
            simdjson::dom::array frames;
            if (doc["info"]["frames"].get(frames) != simdjson::SUCCESS || frames.size() == 0) {
                continue;
            }
        } catch (...) {
            continue;
        }

        if (SaveTimeline(mid, raw)) {
            backfilled_count++;
        }
    }

    return backfilled_count;
}

static bool IsReadOnlyQuery(const std::string& sql, std::string& out_error) {
    std::string trimmed = sql;
    if (trimmed.size() >= 3 && (unsigned char)trimmed[0] == 0xEF && (unsigned char)trimmed[1] == 0xBB && (unsigned char)trimmed[2] == 0xBF) {
        trimmed = trimmed.substr(3);
    }

    std::vector<std::string> statements;
    std::string current_stmt;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool in_line_comment = false;
    bool in_block_comment = false;

    for (size_t i = 0; i < trimmed.size(); ++i) {
        char c = trimmed[i];
        char next = (i + 1 < trimmed.size()) ? trimmed[i + 1] : '\0';

        if (in_line_comment) {
            if (c == '\n' || c == '\r') {
                in_line_comment = false;
            }
            continue;
        }
        if (in_block_comment) {
            if (c == '*' && next == '/') {
                in_block_comment = false;
                ++i;
            }
            continue;
        }
        if (in_single_quote) {
            if (c == '\'') {
                if (next == '\'') {
                    ++i; // escaped quote
                } else {
                    in_single_quote = false;
                }
            }
            continue;
        }
        if (in_double_quote) {
            if (c == '"') {
                if (next == '"') {
                    ++i; // escaped quote
                } else {
                    in_double_quote = false;
                }
            }
            continue;
        }

        // Check comment start
        if (c == '-' && next == '-') {
            in_line_comment = true;
            ++i;
            continue;
        }
        if (c == '/' && next == '*') {
            in_block_comment = true;
            ++i;
            continue;
        }
        if (c == '\'') {
            in_single_quote = true;
            continue;
        }
        if (c == '"') {
            in_double_quote = true;
            continue;
        }

        if (c == ';') {
            statements.push_back(current_stmt);
            current_stmt.clear();
        } else {
            current_stmt += c;
        }
    }
    if (!current_stmt.empty()) {
        statements.push_back(current_stmt);
    }

    std::vector<std::vector<std::string>> parsed_stmts;
    for (const auto& stmt_text : statements) {
        std::vector<std::string> tokens;
        std::string cur_tok;
        for (char ch : stmt_text) {
            if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
                cur_tok += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            } else {
                if (!cur_tok.empty()) {
                    tokens.push_back(cur_tok);
                    cur_tok.clear();
                }
            }
        }
        if (!cur_tok.empty()) {
            tokens.push_back(cur_tok);
        }
        if (!tokens.empty()) {
            parsed_stmts.push_back(std::move(tokens));
        }
    }

    if (parsed_stmts.empty()) {
        out_error = "Empty SQL query.";
        return false;
    }

    static const std::vector<std::string> allowed_starters = {
        "SELECT", "WITH", "PRAGMA", "EXPLAIN"
    };

    static const std::vector<std::string> forbidden_keywords = {
        "INSERT", "UPDATE", "DELETE", "DROP", "ALTER", "CREATE",
        "ATTACH", "DETACH", "TRUNCATE", "REPLACE", "GRANT", "REVOKE", "VACUUM"
    };

    for (const auto& tokens : parsed_stmts) {
        const std::string& first = tokens[0];
        bool starter_ok = false;
        for (const auto& st : allowed_starters) {
            if (first == st) {
                starter_ok = true;
                break;
            }
        }
        if (!starter_ok) {
            out_error = "Read-only query violation: Only SELECT, WITH, PRAGMA, and EXPLAIN queries are permitted (found: " + first + ").";
            return false;
        }

        for (const auto& tok : tokens) {
            for (const auto& fk : forbidden_keywords) {
                if (tok == fk) {
                    out_error = "Read-only query violation: Mutating statement keyword detected: " + tok;
                    return false;
                }
            }
        }
    }

    return true;
}

QueryResult DatabaseManager::ExecuteQuery(const std::string& sql) {
    QueryResult res;
    std::string err;
    if (!IsReadOnlyQuery(sql, err)) {
        res.success = false;
        res.error_message = err;
        return res;
    }

    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) {
        res.success = false;
        res.error_message = "Database is not initialized.";
        return res;
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    duckdb_result result;
    if (duckdb_query(conn_, sql.c_str(), &result) == DuckDBError) {
        auto end_time = std::chrono::high_resolution_clock::now();
        res.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        res.success = false;
        const char* d_err = duckdb_result_error(&result);
        res.error_message = d_err ? d_err : "Query execution failed.";
        duckdb_destroy_result(&result);
        return res;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    res.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    idx_t col_count = duckdb_column_count(&result);
    res.column_names.reserve(col_count);
    for (idx_t c = 0; c < col_count; ++c) {
        const char* c_name = duckdb_column_name(&result, c);
        res.column_names.push_back(c_name ? c_name : ("col_" + std::to_string(c)));
    }

    idx_t row_count = duckdb_row_count(&result);
    res.rows.reserve(row_count);
    for (idx_t r = 0; r < row_count; ++r) {
        std::vector<std::string> row;
        row.reserve(col_count);
        for (idx_t c = 0; c < col_count; ++c) {
            if (duckdb_value_is_null(&result, c, r)) {
                row.push_back("NULL");
            } else {
                char* val_c = duckdb_value_varchar(&result, c, r);
                row.push_back(val_c ? val_c : "");
                if (val_c) {
                    duckdb_free(val_c);
                }
            }
        }
        res.rows.push_back(std::move(row));
    }

    res.row_count = static_cast<int64_t>(row_count);
    res.success = true;
    duckdb_destroy_result(&result);
    return res;
}

TableCounts DatabaseManager::GetTableCounts() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    TableCounts tc;
    if (!initialized_ || !conn_) {
        return tc;
    }

    auto count_table = [&](const char* table_name) -> int64_t {
        std::string q = std::string("SELECT COUNT(*) FROM ") + table_name + ";";
        duckdb_result res;
        int64_t count = 0;
        if (duckdb_query(conn_, q.c_str(), &res) != DuckDBError) {
            if (duckdb_row_count(&res) > 0) {
                count = duckdb_value_int64(&res, 0, 0);
            }
        }
        duckdb_destroy_result(&res);
        return count;
    };

    tc.players = count_table("players");
    tc.matches = count_table("matches");
    tc.match_participants = count_table("match_participants");
    tc.match_timelines = count_table("match_timelines");
    tc.timeline_participant_frames = count_table("timeline_participant_frames");
    tc.timeline_events = count_table("timeline_events");
    return tc;
}

int64_t DatabaseManager::GetParticipantCount() {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) return 0;
    duckdb_result res;
    int64_t count = 0;
    if (duckdb_query(conn_, "SELECT COUNT(*) FROM match_participants;", &res) != DuckDBError) {
        if (duckdb_row_count(&res) > 0) count = duckdb_value_int64(&res, 0, 0);
    }
    duckdb_destroy_result(&res);
    return count;
}

int64_t DatabaseManager::GetTimelineFrameCount() {
    return GetTimelineParticipantFrameCount();
}

QueryResult DatabaseManager::GetChampionPerformanceSummary(int limit) {
    if (limit <= 0) limit = 50;
    std::string sql = R"(
        SELECT
            champion_name AS "Champion",
            COUNT(*) AS "Picks",
            SUM(CASE WHEN win THEN 1 ELSE 0 END) AS "Wins",
            ROUND(100.0 * SUM(CASE WHEN win THEN 1 ELSE 0 END) / COUNT(*), 1) AS "Winrate %",
            ROUND(AVG(kills), 1) AS "Avg K",
            ROUND(AVG(deaths), 1) AS "Avg D",
            ROUND(AVG(assists), 1) AS "Avg A",
            ROUND(CASE WHEN SUM(deaths) = 0 THEN (SUM(kills) + SUM(assists)) * 1.0 ELSE (SUM(kills) + SUM(assists)) * 1.0 / SUM(deaths) END, 2) AS "KDA"
        FROM match_participants
        WHERE champion_name IS NOT NULL AND champion_name != ''
        GROUP BY champion_name
        ORDER BY "Picks" DESC, "Wins" DESC
        LIMIT )" + std::to_string(limit) + ";";
    return ExecuteQuery(sql);
}

QueryResult DatabaseManager::GetTopTrackedPlayersSummary(int limit) {
    if (limit <= 0) limit = 50;
    std::string sql = R"(
        SELECT
            COALESCE(
                NULLIF(TRIM(p.current_game_name || '#' || p.current_tag_line), '#'),
                NULLIF(TRIM(mp.game_name || '#' || mp.tag_line), '#'),
                mp.player_id,
                'Unknown'
            ) AS "Riot ID",
            COUNT(DISTINCT mp.match_id) AS "Matches in DB",
            SUM(CASE WHEN mp.win THEN 1 ELSE 0 END) AS "Wins",
            ROUND(100.0 * SUM(CASE WHEN mp.win THEN 1 ELSE 0 END) / COUNT(DISTINCT mp.match_id), 1) AS "Winrate %",
            ROUND(AVG(mp.kills), 1) AS "Avg K",
            ROUND(AVG(mp.deaths), 1) AS "Avg D",
            ROUND(AVG(mp.assists), 1) AS "Avg A"
        FROM match_participants mp
        LEFT JOIN players p ON mp.player_id = p.player_id
        WHERE (mp.player_id IS NOT NULL AND mp.player_id != '') OR (mp.game_name IS NOT NULL AND mp.game_name != '')
        GROUP BY 1
        ORDER BY "Matches in DB" DESC, "Wins" DESC
        LIMIT )" + std::to_string(limit) + ";";
    return ExecuteQuery(sql);
}

QueryResult DatabaseManager::GetTimelineFramesSummary() {
    std::string sql = R"(
        SELECT
            frame_index AS "Minute",
            ROUND(AVG(total_gold), 0) AS "Avg Gold",
            ROUND(AVG(xp), 0) AS "Avg XP",
            ROUND(AVG(minions_killed + jungle_minions_killed), 1) AS "Avg CS",
            ROUND(AVG(level), 1) AS "Avg Level",
            COUNT(DISTINCT match_id) AS "Matches"
        FROM timeline_participant_frames
        GROUP BY frame_index
        ORDER BY frame_index ASC
        LIMIT 35;
    )";
    return ExecuteQuery(sql);
}

QueryResult DatabaseManager::GetTimelineEventsSummary() {
    std::string sql = R"(
        SELECT
            event_type AS "Event Type",
            COUNT(*) AS "Count",
            ROUND(AVG(timestamp) / 60000.0, 1) AS "Avg Minute",
            COUNT(DISTINCT match_id) AS "Matches"
        FROM timeline_events
        WHERE event_type IS NOT NULL AND event_type != ''
        GROUP BY event_type
        ORDER BY "Count" DESC;
    )";
    return ExecuteQuery(sql);
}

static std::string EscapeCsvCell(const std::string& cell) {
    bool needs_quotes = false;
    for (char c : cell) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needs_quotes = true;
            break;
        }
    }
    if (!needs_quotes) {
        return cell;
    }
    std::string res = "\"";
    for (char c : cell) {
        if (c == '"') {
            res += "\"\"";
        } else {
            res += c;
        }
    }
    res += "\"";
    return res;
}

bool DatabaseManager::ExportQueryToCSV(const std::string& sql, const std::string& output_csv_path) {
    std::lock_guard<std::recursive_mutex> lock(db_mutex_);
    if (!initialized_ || !conn_) {
        return false;
    }

    try {
        std::filesystem::path p(output_csv_path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {
        return false;
    }

    QueryResult qr = ExecuteQuery(sql);
    if (!qr.success) {
        return false;
    }

    std::ofstream out(output_csv_path);
    if (!out.is_open()) {
        return false;
    }

    // Write header
    for (size_t i = 0; i < qr.column_names.size(); ++i) {
        out << EscapeCsvCell(qr.column_names[i]);
        if (i + 1 < qr.column_names.size()) {
            out << ",";
        }
    }
    out << "\n";

    // Write rows
    for (const auto& row : qr.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            out << EscapeCsvCell(row[i]);
            if (i + 1 < row.size()) {
                out << ",";
            }
        }
        out << "\n";
    }

    out.flush();
    return out.good();
}

bool DatabaseManager::ExportTableToCSV(const std::string& table_name, const std::string& output_csv_path) {
    if (table_name.empty()) {
        return false;
    }
    for (char c : table_name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    std::string sql = "SELECT * FROM " + table_name + ";";
    return ExportQueryToCSV(sql, output_csv_path);
}

} // namespace restfulEz

