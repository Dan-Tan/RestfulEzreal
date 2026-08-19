#include "DatabaseManager.h"
#include "simdjson.h"
#include <filesystem>
#include <iostream>
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
                    player_id = std::string(duckdb_value_varchar(&res, 0, 0));
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
                    player_id = std::string(duckdb_value_varchar(&res, 0, 0));
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
    // 1. Re-point player_aliases
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
    if (!initialized_ || !conn_) return false;

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
        if (parser.parse(padded).get(doc) == simdjson::SUCCESS) {
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
        }
    } catch (...) {
        // Fallback for minimal/malformed json
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

    // 2. Insert participant rows
    for (const auto& p : participants) {
        std::string p_player_id = p.player_id;
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
    if (!initialized_ || !conn_) return false;

    int32_t frame_interval = 0;
    try {
        simdjson::dom::parser parser;
        simdjson::dom::element doc;
        if (parser.parse(raw_json_str.data(), raw_json_str.size()).get(doc) == simdjson::SUCCESS) {
            int64_t val = 0;
            if (doc["info"]["frameInterval"].get(val) == simdjson::SUCCESS) {
                frame_interval = static_cast<int32_t>(val);
            }
        }
    } catch (...) {
        // Fallback for minimal json
    }

    duckdb_prepared_statement stmt;
    if (duckdb_prepare(conn_, R"(
        INSERT OR REPLACE INTO match_timelines (match_id, frame_interval, raw_json) 
        VALUES ($1, $2, $3);
    )", &stmt) == DuckDBError) {
        std::cerr << "[DatabaseManager] Prepare error in SaveTimeline: " << duckdb_prepare_error(stmt) << std::endl;
        duckdb_destroy_prepare(&stmt);
        return false;
    }

    duckdb_bind_varchar(stmt, 1, match_id.c_str());
    duckdb_bind_int32(stmt, 2, frame_interval);
    duckdb_bind_varchar(stmt, 3, raw_json_str.c_str());

    duckdb_result res;
    bool success = (duckdb_execute_prepared(stmt, &res) != DuckDBError);
    if (!success) {
        std::cerr << "[DatabaseManager] SaveTimeline execute error: " << duckdb_result_error(&res) << std::endl;
    }
    duckdb_destroy_result(&res);
    duckdb_destroy_prepare(&stmt);
    return success;
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

} // namespace restfulEz

