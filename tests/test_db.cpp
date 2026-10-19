#include "db/DatabaseManager.h"
#include <iostream>
#include <filesystem>
#include <cassert>

int main() {
    std::cout << "[Test] Running DatabaseManager DuckDB universal integration tests..." << std::endl;

    std::string test_db_path = "./test_output/test_ezreal.duckdb";
    std::string test_db_reopen_path = "./test_output/reopen_ezreal.duckdb";
    if (std::filesystem::exists("./test_output")) {
        std::filesystem::remove_all("./test_output");
    }

    restfulEz::DatabaseManager& db = restfulEz::DatabaseManager::GetInstance();
    
    // 1. Test Initialization
    bool init_ok = db.Init(test_db_path);
    assert(init_ok);
    assert(db.IsInitialized());
    assert(db.GetDatabasePath() == test_db_path);
    std::cout << "  [PASS] Database Init" << std::endl;

    // 2. Test Multi-Developer Player Resolution & Aliases
    // Dev A queries Faker#KR1
    std::string p1 = db.ResolveOrCreatePlayer("DEV_A", "PUUID_A_FAKER", "Faker", "KR1");
    assert(!p1.empty());
    assert(db.GetPlayerCount() == 1);

    // Dev A queries Faker#KR1 again -> returns same p1 instantly
    std::string p1_again = db.ResolveOrCreatePlayer("DEV_A", "PUUID_A_FAKER", "Faker", "KR1");
    assert(p1_again == p1);
    assert(db.GetPlayerCount() == 1);

    // Player renames to "Hide on bush#KR1" -> Dev A queries with same PUUID
    std::string p1_renamed = db.ResolveOrCreatePlayer("DEV_A", "PUUID_A_FAKER", "Hide on bush", "KR1");
    assert(p1_renamed == p1);
    assert(db.GetPlayerCount() == 1);

    // Dev B queries "hide on bush#kr1" (case-insensitive alias match) with Dev B's key -> finds existing player and maps Dev B's PUUID
    std::string p1_dev_b = db.ResolveOrCreatePlayer("DEV_B", "PUUID_B_FAKER", "hide on bush", "kr1");
    assert(p1_dev_b == p1);
    assert(db.GetPlayerCount() == 1);
    std::cout << "  [PASS] Multi-Developer Player Identity & Case-Insensitive Alias Resolution" << std::endl;

    // 3. Test SaveAccount (Legacy & Universal)
    bool acc_ok = db.SaveAccount("DEV_A", "PUUID_A_EZREAL", "Ezreal", "NA1");
    assert(acc_ok);
    assert(db.GetPlayerCount() == 2);
    assert(db.GetAccountCount() == 2);

    restfulEz::AccountRecord legacy_acc{"PUUID_LEGACY_LUX", "Lux", "DEMACIA", 0};
    assert(db.SaveAccount(legacy_acc));
    assert(db.GetPlayerCount() == 3);
    std::cout << "  [PASS] SaveAccount (Universal & Legacy)" << std::endl;

    // 4. Test MergePlayers (including overlapping aliases)
    std::string p_temp = db.ResolveOrCreatePlayer("DEV_C", "PUUID_C_TEMP", "TempPlayer", "EUW1");
    assert(db.GetPlayerCount() == 4);
    bool merge_ok = db.MergePlayers(p1, p_temp);
    assert(merge_ok);
    assert(db.GetPlayerCount() == 3);
    // Verify DEV_C's PUUID now resolves to p1
    assert(db.ResolveOrCreatePlayer("DEV_C", "PUUID_C_TEMP", "", "") == p1);
    std::cout << "  [PASS] MergePlayers" << std::endl;

    // 5. Test SaveMatch, Participant Linking, Idempotency & Error Rejection
    std::string match_id = "KR_6987654321";
    std::string sample_match_json = R"({
        "metadata": {
            "matchId": "KR_6987654321"
        },
        "info": {
            "gameCreation": 1650000000000,
            "gameDuration": 1850,
            "gameVersion": "14.10.1",
            "queueId": 420,
            "participants": [
                { "participantId": 1, "puuid": "OTHER_PUUID", "riotIdGameName": "Player1", "riotIdTagline": "KR1", "championName": "LeeSin", "championId": 64, "teamId": 100, "kills": 3, "deaths": 4, "assists": 12, "win": true },
                { "participantId": 2, "puuid": "PUUID_A_FAKER", "riotIdGameName": "Hide on bush", "riotIdTagline": "KR1", "championName": "Ahri", "championId": 103, "teamId": 100, "kills": 10, "deaths": 2, "assists": 8, "win": true }
            ]
        }
    })";

    // Call SaveMatch without primary_player_id (as RequestSender::write_response_file does)
    bool match_ok = db.SaveMatch(match_id, sample_match_json);
    assert(match_ok);
    assert(db.HasMatch(match_id));
    assert(!db.HasMatch("NON_EXISTENT_MATCH"));
    assert(db.GetMatchCount() == 1);
    assert(db.GetMatchParticipantCount(match_id) == 2);
    // Participant 2 has PUUID_A_FAKER -> must automatically link to canonical player p1
    assert(db.GetParticipantPlayerId(match_id, 2) == p1);

    // Idempotent upsert of same match_id should not duplicate match or participant rows
    assert(db.SaveMatch(match_id, sample_match_json));
    assert(db.GetMatchCount() == 1);
    assert(db.GetMatchParticipantCount(match_id) == 2);

    // Reject Riot API error responses & malformed JSON
    std::string error_json = R"({"status":{"message":"Data not found - match file not found","status_code":404}})";
    assert(!db.SaveMatch("KR_ERROR_404", error_json));
    assert(!db.HasMatch("KR_ERROR_404"));
    assert(!db.SaveMatch("KR_MALFORMED", "{not valid json"));
    assert(!db.SaveMatch("KR_EMPTY_OBJ", "{}"));
    assert(db.GetMatchCount() == 1);
    std::cout << "  [PASS] SaveMatch, Participant Resolution, Idempotency & Error Rejection" << std::endl;

    // 6. Test SaveTimeline, HasTimeline, GetTimelineCount & Error Rejection
    std::string sample_timeline_json = R"({
        "metadata": {
            "matchId": "KR_6987654321"
        },
        "info": {
            "frameInterval": 60000
        }
    })";

    bool timeline_ok = db.SaveTimeline(match_id, sample_timeline_json);
    assert(timeline_ok);
    assert(db.HasTimeline(match_id));
    assert(!db.HasTimeline("NON_EXISTENT_MATCH"));
    assert(db.GetTimelineCount() == 1);

    // Upsert same timeline
    assert(db.SaveTimeline(match_id, sample_timeline_json));
    assert(db.GetTimelineCount() == 1);

    // Reject error and malformed timeline payloads
    // Reject error and malformed timeline payloads
    assert(!db.SaveTimeline("KR_ERROR_404", error_json));
    assert(!db.SaveTimeline("KR_MALFORMED", "invalid_json"));
    assert(db.GetTimelineCount() == 1);
    std::cout << "  [PASS] SaveTimeline, HasTimeline & Error Rejection" << std::endl;

    // 6b. Test InitTables() public method
    assert(db.InitTables());
    std::cout << "  [PASS] InitTables() public method" << std::endl;

    // 6c. Test Rich Timeline Unnesting, Participant Frames, Events, Idempotency & Backfill
    std::string rich_match_id = "KR_RICH_001";
    std::string rich_timeline_json = R"({
        "metadata": {
            "matchId": "KR_RICH_001"
        },
        "info": {
            "frameInterval": 60000,
            "frames": [
                {
                    "timestamp": 0,
                    "participantFrames": {
                        "1": {
                            "participantId": 1,
                            "totalGold": 500,
                            "currentGold": 500,
                            "xp": 0,
                            "level": 1,
                            "minionsKilled": 0,
                            "jungleMinionsKilled": 0,
                            "position": { "x": 1000, "y": 1000 },
                            "damageStats": { "totalDamageDoneToChampions": 0 }
                        },
                        "2": {
                            "participantId": 2,
                            "totalGold": 500,
                            "currentGold": 500,
                            "xp": 0,
                            "level": 1,
                            "minionsKilled": 0,
                            "jungleMinionsKilled": 0,
                            "position": { "x": 2000, "y": 2000 },
                            "damageStats": { "totalDamageDoneToChampions": 0 }
                        }
                    },
                    "events": [
                        { "timestamp": 1000, "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1055 },
                        { "timestamp": 1500, "type": "WARD_PLACED", "creatorId": 2 }
                    ]
                },
                {
                    "timestamp": 60000,
                    "participantFrames": {
                        "1": {
                            "participantId": 1,
                            "totalGold": 950,
                            "currentGold": 350,
                            "xp": 550,
                            "level": 2,
                            "minionsKilled": 12,
                            "jungleMinionsKilled": 2,
                            "position": { "x": 3500, "y": 4200 },
                            "damageStats": { "totalDamageDoneToChampions": 450 }
                        },
                        "2": {
                            "participantId": 2,
                            "totalGold": 800,
                            "currentGold": 300,
                            "xp": 480,
                            "level": 2,
                            "minionsKilled": 8,
                            "jungleMinionsKilled": 0,
                            "position": { "x": 5000, "y": 5000 },
                            "damageStats": { "totalDamageDoneToChampions": 200 }
                        }
                    },
                    "events": [
                        {
                            "timestamp": 45000,
                            "type": "CHAMPION_KILL",
                            "killerId": 1,
                            "victimId": 2,
                            "position": { "x": 4000, "y": 4100 },
                            "assistingParticipantIds": [3, 4]
                        },
                        {
                            "timestamp": 55000,
                            "type": "ELITE_MONSTER_KILL",
                            "killerId": 1,
                            "teamId": 100,
                            "monsterType": "DRAGON",
                            "monsterSubType": "WATER_DRAGON",
                            "position": { "x": 9800, "y": 4400 }
                        },
                        {
                            "timestamp": 58000,
                            "type": "BUILDING_KILL",
                            "killerId": 1,
                            "buildingType": "TOWER_BUILDING",
                            "laneType": "MID_LANE",
                            "teamId": 200,
                            "position": { "x": 6000, "y": 6000 }
                        }
                    ]
                }
            ]
        }
    })";

    bool rich_ok = db.SaveTimeline(rich_match_id, rich_timeline_json);
    assert(rich_ok);
    assert(db.HasTimeline(rich_match_id));
    assert(db.GetTimelineCount() == 2); // 1 sample + 1 rich

    // Verify participant frame and event counts
    assert(db.GetTimelineParticipantFrameCount() == 4); // 2 frames * 2 participants
    assert(db.GetTimelineEventCount() == 5);            // 2 events frame 0 + 3 events frame 1

    // Verify timeline idempotency (re-saving same timeline must not duplicate rows)
    assert(db.SaveTimeline(rich_match_id, rich_timeline_json));
    assert(db.GetTimelineParticipantFrameCount() == 4);
    assert(db.GetTimelineEventCount() == 5);
    std::cout << "  [PASS] Rich Timeline Unnesting, Frame/Event Counts & Idempotency" << std::endl;

    // Verify column values in DuckDB
    duckdb_database test_raw_db;
    duckdb_connection test_raw_conn;
    assert(duckdb_open(test_db_path.c_str(), &test_raw_db) != DuckDBError);
    assert(duckdb_connect(test_raw_db, &test_raw_conn) != DuckDBError);

    duckdb_result pf_check_res;
    const char* pf_sql = R"(
        SELECT total_gold, current_gold, xp, level, minions_killed, jungle_minions_killed, pos_x, pos_y, total_damage_done_to_champions
        FROM timeline_participant_frames
        WHERE match_id = 'KR_RICH_001' AND frame_index = 1 AND participant_id = 1;
    )";
    assert(duckdb_query(test_raw_conn, pf_sql, &pf_check_res) != DuckDBError);
    assert(duckdb_row_count(&pf_check_res) == 1);
    assert(duckdb_value_int32(&pf_check_res, 0, 0) == 950);
    assert(duckdb_value_int32(&pf_check_res, 1, 0) == 350);
    assert(duckdb_value_int32(&pf_check_res, 2, 0) == 550);
    assert(duckdb_value_int32(&pf_check_res, 3, 0) == 2);
    assert(duckdb_value_int32(&pf_check_res, 4, 0) == 12);
    assert(duckdb_value_int32(&pf_check_res, 5, 0) == 2);
    assert(duckdb_value_int32(&pf_check_res, 6, 0) == 3500);
    assert(duckdb_value_int32(&pf_check_res, 7, 0) == 4200);
    assert(duckdb_value_int32(&pf_check_res, 8, 0) == 450);
    duckdb_destroy_result(&pf_check_res);

    duckdb_result ev_check_res;
    const char* ev_sql = R"(
        SELECT event_type, killer_id, victim_id, assisting_participant_ids, monster_type, monster_sub_type, building_type, lane_type, team_id
        FROM timeline_events
        WHERE match_id = 'KR_RICH_001'
        ORDER BY frame_index, event_index;
    )";
    assert(duckdb_query(test_raw_conn, ev_sql, &ev_check_res) != DuckDBError);
    assert(duckdb_row_count(&ev_check_res) == 5);
    // Event 2 is CHAMPION_KILL
    char* ev2_type = duckdb_value_varchar(&ev_check_res, 0, 2);
    char* ev2_assists = duckdb_value_varchar(&ev_check_res, 3, 2);
    assert(std::string(ev2_type) == "CHAMPION_KILL");
    assert(duckdb_value_int32(&ev_check_res, 1, 2) == 1);
    assert(duckdb_value_int32(&ev_check_res, 2, 2) == 2);
    assert(std::string(ev2_assists) == "3,4");
    duckdb_free(ev2_type);
    duckdb_free(ev2_assists);

    // Event 3 is ELITE_MONSTER_KILL
    char* ev3_mtype = duckdb_value_varchar(&ev_check_res, 4, 3);
    char* ev3_msubtype = duckdb_value_varchar(&ev_check_res, 5, 3);
    assert(std::string(ev3_mtype) == "DRAGON");
    assert(std::string(ev3_msubtype) == "WATER_DRAGON");
    duckdb_free(ev3_mtype);
    duckdb_free(ev3_msubtype);

    // Event 4 is BUILDING_KILL
    char* ev4_btype = duckdb_value_varchar(&ev_check_res, 6, 4);
    char* ev4_ltype = duckdb_value_varchar(&ev_check_res, 7, 4);
    assert(std::string(ev4_btype) == "TOWER_BUILDING");
    assert(std::string(ev4_ltype) == "MID_LANE");
    assert(duckdb_value_int32(&ev_check_res, 8, 4) == 200);
    duckdb_free(ev4_btype);
    duckdb_free(ev4_ltype);
    duckdb_destroy_result(&ev_check_res);

    // 6d. Test BackfillUnnestedTimelines()
    // Insert a legacy timeline directly into match_timelines without participant frames
    std::string legacy_match_id = "KR_LEGACY_099";
    std::string legacy_timeline_json = R"({
        "metadata": { "matchId": "KR_LEGACY_099" },
        "info": {
            "frameInterval": 60000,
            "frames": [
                {
                    "timestamp": 0,
                    "participantFrames": {
                        "1": { "participantId": 1, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1, "minionsKilled": 0, "jungleMinionsKilled": 0 }
                    },
                    "events": [
                        { "timestamp": 500, "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1054 }
                    ]
                }
            ]
        }
    })";

    duckdb_prepared_statement legacy_stmt;
    assert(duckdb_prepare(test_raw_conn, "INSERT OR REPLACE INTO match_timelines (match_id, frame_interval, raw_json) VALUES ($1, $2, $3);", &legacy_stmt) != DuckDBError);
    duckdb_bind_varchar(legacy_stmt, 1, legacy_match_id.c_str());
    duckdb_bind_int32(legacy_stmt, 2, 60000);
    duckdb_bind_varchar(legacy_stmt, 3, legacy_timeline_json.c_str());
    duckdb_result legacy_res;
    assert(duckdb_execute_prepared(legacy_stmt, &legacy_res) != DuckDBError);
    duckdb_destroy_result(&legacy_res);
    duckdb_destroy_prepare(&legacy_stmt);

    duckdb_disconnect(&test_raw_conn);
    duckdb_close(&test_raw_db);

    int64_t backfilled = db.BackfillUnnestedTimelines();
    std::cout << "backfilled=" << backfilled 
              << " pf_count=" << db.GetTimelineParticipantFrameCount() 
              << " ev_count=" << db.GetTimelineEventCount() << std::endl;
    assert(backfilled == 1);
    assert(db.GetTimelineParticipantFrameCount() == 5); // 4 from rich + 1 from legacy
    assert(db.GetTimelineEventCount() == 6);            // 5 from rich + 1 from legacy

    // Subsequent call must return 0
    assert(db.BackfillUnnestedTimelines() == 0);
    std::cout << "  [PASS] Timeline Column Value Verification & BackfillUnnestedTimelines" << std::endl;

    // 6e. Test HasMatchesForPlayerPuuid
    assert(db.HasMatchesForPlayerPuuid("PUUID_A_FAKER", 1));
    assert(db.HasMatchesForPlayerPuuid("PUUID_B_FAKER", 1)); // Dev B mapped to p1
    assert(db.HasMatchesForPlayerPuuid("PUUID_C_TEMP", 1));  // Merged into p1
    assert(db.HasMatchesForPlayerPuuid(p1, 1));               // Canonical player ID directly
    assert(!db.HasMatchesForPlayerPuuid("PUUID_A_FAKER", 2)); // Faker only has 1 match in DB
    assert(!db.HasMatchesForPlayerPuuid("PUUID_A_EZREAL", 1)); // Ezreal account saved, but 0 matches
    assert(!db.HasMatchesForPlayerPuuid("NON_EXISTENT_PUUID", 1));
    assert(!db.HasMatchesForPlayerPuuid("", 1));
    assert(db.HasMatchesForPlayerPuuid("PUUID_A_FAKER", 0));  // min_matches <= 0 returns true
    assert(db.HasMatchesForPlayerPuuid("NON_EXISTENT_PUUID", 0)); // min_matches <= 0 returns true for non-empty
    assert(!db.HasMatchesForPlayerPuuid("", 0));              // empty puuid returns false even if min_matches <= 0
    std::cout << "  [PASS] HasMatchesForPlayerPuuid (mappings, counts, bounds & thresholds)" << std::endl;

    // 6f. Test Analytical Query Engine, TableCounts, Summaries, and CSV Export
    auto tc = db.GetTableCounts();
    assert(tc.players >= 3);
    assert(tc.matches >= 1);
    assert(tc.match_participants >= 2);
    assert(tc.match_timelines >= 1);
    assert(tc.timeline_participant_frames >= 5);
    assert(tc.timeline_events >= 6);
    assert(db.GetParticipantCount() >= 2);
    assert(db.GetTimelineFrameCount() >= 5);
    std::cout << "  [PASS] GetTableCounts & GetParticipantCount / GetTimelineFrameCount" << std::endl;

    // ExecuteQuery validation: Read-Only enforcement & valid execution
    auto valid_q = db.ExecuteQuery("SELECT player_id, current_game_name FROM players;");
    assert(valid_q.success);
    assert(valid_q.row_count >= 3);
    assert(valid_q.column_names.size() == 2);
    assert(valid_q.column_names[0] == "player_id");
    assert(valid_q.column_names[1] == "current_game_name");
    assert(valid_q.execution_time_ms >= 0.0);

    // Read-only violations
    auto ro_drop = db.ExecuteQuery("DROP TABLE players;");
    assert(!ro_drop.success);
    assert(ro_drop.error_message.find("Read-only") != std::string::npos);

    auto ro_insert = db.ExecuteQuery("INSERT INTO players VALUES ('p_malicious', 'Hacker', 'NA1', 0, 0);");
    assert(!ro_insert.success);
    assert(ro_insert.error_message.find("Read-only") != std::string::npos);

    auto ro_delete = db.ExecuteQuery("DELETE FROM players;");
    assert(!ro_delete.success);
    assert(ro_delete.error_message.find("Read-only") != std::string::npos);

    auto ro_multi = db.ExecuteQuery("SELECT * FROM players; DROP TABLE players;");
    assert(!ro_multi.success);

    // Summaries
    auto champ_summary = db.GetChampionPerformanceSummary(10);
    assert(champ_summary.success);
    assert(champ_summary.row_count >= 1);
    assert(champ_summary.column_names[0] == "Champion");

    auto player_summary = db.GetTopTrackedPlayersSummary(10);
    assert(player_summary.success);
    assert(player_summary.row_count >= 1);
    assert(player_summary.column_names[0] == "Riot ID");

    auto tf_summary = db.GetTimelineFramesSummary();
    assert(tf_summary.success);
    assert(tf_summary.row_count >= 1);
    assert(tf_summary.column_names[0] == "Minute");

    auto ev_summary = db.GetTimelineEventsSummary();
    assert(ev_summary.success);
    assert(ev_summary.row_count >= 1);
    assert(ev_summary.column_names[0] == "Event Type");

    // CSV Exports
    std::string test_csv_dir = "./test_output/exports";
    std::string test_q_csv = test_csv_dir + "/query_export.csv";
    std::string test_tbl_csv = test_csv_dir + "/matches_export.csv";

    assert(db.ExportQueryToCSV("SELECT champion_name, kills, deaths, assists FROM match_participants;", test_q_csv));
    assert(std::filesystem::exists(test_q_csv));
    assert(std::filesystem::file_size(test_q_csv) > 0);

    assert(db.ExportTableToCSV("matches", test_tbl_csv));
    assert(std::filesystem::exists(test_tbl_csv));
    assert(std::filesystem::file_size(test_tbl_csv) > 0);

    // Invalid table name rejection
    assert(!db.ExportTableToCSV("matches; DROP TABLE players;", test_csv_dir + "/bad.csv"));
    assert(!db.ExportTableToCSV("", test_csv_dir + "/empty.csv"));

    std::cout << "  [PASS] ExecuteQuery, Summaries, and CSV Exporters" << std::endl;

    // 7. Test Reopen
    bool reopen_ok = db.Reopen(test_db_reopen_path);
    assert(reopen_ok);
    assert(db.IsInitialized());
    assert(db.GetDatabasePath() == test_db_reopen_path);
    assert(db.GetMatchCount() == 0); // New empty DB
    assert(db.GetTimelineCount() == 0);
    assert(db.GetTimelineParticipantFrameCount() == 0);
    assert(db.GetTimelineEventCount() == 0);
    assert(!db.HasMatchesForPlayerPuuid("PUUID_A_FAKER", 1)); // Fresh DB has 0 matches
    std::cout << "  [PASS] Database Reopen" << std::endl;

    // 8. Cleanup
    db.Close();
    assert(!db.IsInitialized());
    assert(!db.HasMatchesForPlayerPuuid("PUUID_A_FAKER", 1)); // Uninitialized returns false
    assert(!db.HasMatchesForPlayerPuuid("PUUID_A_FAKER", 0)); // Uninitialized returns false
    assert(db.GetTimelineParticipantFrameCount() == 0);
    assert(db.GetTimelineEventCount() == 0);
    assert(db.BackfillUnnestedTimelines() == 0);
    assert(!db.InitTables());
    std::cout << "  [PASS] Close Database" << std::endl;

    std::filesystem::remove_all("./test_output");
    std::cout << "[Test] All DuckDB universal integration tests passed successfully!" << std::endl;
    return 0;
}
