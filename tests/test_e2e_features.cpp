#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <concepts>
#include <functional>
#include <cmath>

#define private public
#define protected public
#include "imgui.h"
#include "db/DatabaseManager.h"
#include "BatchRequests.h"
#include "QueryForm.h"
#include "TemplateManager.h"
#include "FileManager.h"
#include "duckdb.h"
#undef private
#undef protected

using namespace restfulEz;

// =============================================================================
// C++20 Concept Checks for Progressive Testability
// =============================================================================
template <typename T>
concept HasHasMatchesForPlayerPuuid = requires(T& t, const std::string& p, int m) {
    { t.HasMatchesForPlayerPuuid(p, m) } -> std::convertible_to<bool>;
};

template <typename T>
concept HasSkipPlayersWithMatchesInDB = requires(int m) {
    { T::SkipPlayersWithMatchesInDB(m) } -> std::convertible_to<std::function<bool(const std::string&)>>;
};

template <typename T>
concept HasBackfillUnnestedTimelines = requires(T& t) {
    { t.BackfillUnnestedTimelines() } -> std::convertible_to<int64_t>;
};

template <typename T>
concept HasExecuteQuery = requires(T& t, const std::string& sql) {
    t.ExecuteQuery(sql);
};

template <typename T>
concept HasGetTableCounts = requires(T& t) {
    t.GetTableCounts();
};

template <typename T>
concept HasGetChampionPerformanceSummary = requires(T& t, int limit) {
    t.GetChampionPerformanceSummary(limit);
};

template <typename T>
concept HasGetTopTrackedPlayersSummary = requires(T& t, int limit) {
    t.GetTopTrackedPlayersSummary(limit);
};

template <typename T>
concept HasExportQueryToCSV = requires(T& t, const std::string& sql, const std::string& path) {
    { t.ExportQueryToCSV(sql, path) } -> std::convertible_to<bool>;
};

template <typename T>
concept HasExportTableToCSV = requires(T& t, const std::string& tbl, const std::string& path) {
    { t.ExportTableToCSV(tbl, path) } -> std::convertible_to<bool>;
};

// =============================================================================
// Fixtures and Helpers
// =============================================================================
static const std::string TEST_WS_DIR = "./test_e2e_ws";
static const std::string TEST_DB_PATH = "./test_e2e_ws/db/ezreal_data.duckdb";
static const std::string TEST_EXPORTS_DIR = "./test_e2e_ws/exports";

static void ResetTestWorkspace() {
    std::filesystem::remove_all(TEST_WS_DIR);
    std::filesystem::create_directories(TEST_WS_DIR + "/db");
    std::filesystem::create_directories(TEST_EXPORTS_DIR);
}

static std::string CreateSampleMatchJson(
    const std::string& match_id,
    const std::vector<std::pair<std::string, std::string>>& participants // {puuid, champion}
) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"metadata\": {\"matchId\": \"" << match_id << "\"},\n"
       << "  \"info\": {\n"
       << "    \"gameCreation\": 1700000000000,\n"
       << "    \"gameDuration\": 1800,\n"
       << "    \"gameVersion\": \"14.10.1\",\n"
       << "    \"queueId\": 420,\n"
       << "    \"participants\": [\n";

    for (size_t i = 0; i < participants.size(); ++i) {
        ss << "      {\n"
           << "        \"participantId\": " << (i + 1) << ",\n"
           << "        \"puuid\": \"" << participants[i].first << "\",\n"
           << "        \"riotIdGameName\": \"Player_" << (i + 1) << "\",\n"
           << "        \"riotIdTagline\": \"KR1\",\n"
           << "        \"championName\": \"" << participants[i].second << "\",\n"
           << "        \"championId\": " << (100 + i) << ",\n"
           << "        \"teamId\": " << ((i < participants.size() / 2) ? 100 : 200) << ",\n"
           << "        \"kills\": " << (i * 2 + 1) << ",\n"
           << "        \"deaths\": " << (i + 1) << ",\n"
           << "        \"assists\": " << (i * 3 + 2) << ",\n"
           << "        \"win\": " << ((i < participants.size() / 2) ? "true" : "false") << "\n"
           << "      }" << (i + 1 < participants.size() ? "," : "") << "\n";
    }

    ss << "    ]\n"
       << "  }\n"
       << "}";
    return ss.str();
}

static std::string CreateSampleTimelineJson(
    const std::string& match_id,
    int frame_count = 2
) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"metadata\": {\"matchId\": \"" << match_id << "\"},\n"
       << "  \"info\": {\n"
       << "    \"frameInterval\": 60000,\n"
       << "    \"frames\": [\n";

    for (int f = 0; f < frame_count; ++f) {
        ss << "      {\n"
           << "        \"timestamp\": " << (f * 60000) << ",\n"
           << "        \"participantFrames\": {\n";
        for (int p = 1; p <= 4; ++p) {
            ss << "          \"" << p << "\": {\n"
               << "            \"participantId\": " << p << ",\n"
               << "            \"totalGold\": " << (500 + f * 400 + p * 50) << ",\n"
               << "            \"currentGold\": " << (200 + f * 100 + p * 20) << ",\n"
               << "            \"xp\": " << (f * 500 + p * 30) << ",\n"
               << "            \"level\": " << (1 + f) << ",\n"
               << "            \"minionsKilled\": " << (f * 10 + p) << ",\n"
               << "            \"jungleMinionsKilled\": 0,\n"
               << "            \"position\": {\"x\": " << (1000 * p) << ", \"y\": " << (1000 * p) << "},\n"
               << "            \"damageStats\": {\"totalDamageDoneToChampions\": " << (f * 300 + p * 50) << "}\n"
               << "          }" << (p < 4 ? "," : "") << "\n";
        }
        ss << "        },\n"
           << "        \"events\": [\n";
        if (f > 0) {
            ss << "          {\n"
               << "            \"timestamp\": " << (f * 60000 - 10000) << ",\n"
               << "            \"type\": \"CHAMPION_KILL\",\n"
               << "            \"killerId\": 1,\n"
               << "            \"victimId\": 3,\n"
               << "            \"position\": {\"x\": 5000, \"y\": 5000},\n"
               << "            \"assistingParticipantIds\": [2]\n"
               << "          },\n"
               << "          {\n"
               << "            \"timestamp\": " << (f * 60000 - 5000) << ",\n"
               << "            \"type\": \"ELITE_MONSTER_KILL\",\n"
               << "            \"killerId\": 1,\n"
               << "            \"teamId\": 100,\n"
               << "            \"monsterType\": \"DRAGON\",\n"
               << "            \"monsterSubType\": \"FIRE_DRAGON\",\n"
               << "            \"position\": {\"x\": 9800, \"y\": 4400}\n"
               << "          }\n";
        }
        ss << "        ]\n"
           << "      }" << (f + 1 < frame_count ? "," : "") << "\n";
    }

    ss << "    ]\n"
       << "  }\n"
       << "}";
    return ss.str();
}

// =============================================================================
// TIER 1: Feature Coverage (Happy Path)
// =============================================================================
template <typename DB = DatabaseManager>
void TestTier1_FeatureCoverage(DB& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Tier 1] Feature Coverage (Happy Path)" << std::endl;
    std::cout << "========================================================" << std::endl;

    ResetTestWorkspace();
    assert(db.Init(TEST_DB_PATH));
    assert(db.IsInitialized());

    // F1 & F2: Pre-filtering on PUUID with matches in DB
    std::cout << "[Tier 1.1] Testing Feature 1 (HasMatchesForPlayerPuuid) & Feature 2 (SkipPlayersWithMatchesInDB)..." << std::endl;
    std::string puuid_p1 = "PUUID_T1_PLAYER1";
    std::string puuid_p2 = "PUUID_T1_PLAYER2";

    db.ResolveOrCreatePlayer("DEV1", puuid_p1, "PlayerOne", "KR1");
    db.ResolveOrCreatePlayer("DEV1", puuid_p2, "PlayerTwo", "KR1");

    std::string match_json = CreateSampleMatchJson("KR_T1_001", {{puuid_p1, "Ahri"}, {"OTHER_PUUID", "LeeSin"}});
    assert(db.SaveMatch("KR_T1_001", match_json));

    if constexpr (HasHasMatchesForPlayerPuuid<DB>) {
        assert(db.HasMatchesForPlayerPuuid(puuid_p1, 1) == true);
        assert(db.HasMatchesForPlayerPuuid(puuid_p1, 2) == false); // only 1 match
        assert(db.HasMatchesForPlayerPuuid(puuid_p2, 1) == false); // 0 matches
        std::cout << "  [PASS] Feature 1: HasMatchesForPlayerPuuid returns true for ingested player and false for uningested." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 1] HasMatchesForPlayerPuuid not yet implemented on DatabaseManager." << std::endl;
    }

    if constexpr (HasSkipPlayersWithMatchesInDB<ValuePool>) {
        auto filter = ValuePool::SkipPlayersWithMatchesInDB(1);
        assert(filter != nullptr);
        ValuePool pool;
        pool.set_filter(filter);
        assert(!pool.push(puuid_p1)); // Player 1 has match -> filtered out
        assert(pool.filtered_out_count() == 1);
        assert(pool.push(puuid_p2));  // Player 2 has 0 matches -> accepted
        assert(pool.size() == 1);
        std::cout << "  [PASS] Feature 2: ValuePool::SkipPlayersWithMatchesInDB filters existing player." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 1] ValuePool::SkipPlayersWithMatchesInDB not yet implemented." << std::endl;
    }

    // F3: QueryForm Skip DB Binding for Match-v5 By Puuid
    std::cout << "[Tier 1.2] Testing Feature 3 (QueryForm Skip DB Binding)..." << std::endl;
    auto lol_form = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(1, 6, 2));
    assert(lol_form != nullptr);
    lol_form->set_param_value(0, "ASIA");
    lol_form->set_param_value(1, "FALLBACK_PUUID");
    lol_form->set_param_skip_db(1, true);
    lol_form->construct_base();
    lol_form->link_final_requests();
    assert(lol_form->get_base_request() != nullptr);
    auto pool1 = lol_form->get_base_request()->_node->unsent_request->get_or_create_pool(1);
    assert(pool1 != nullptr);
    assert(pool1->has_filter() == true);
    std::cout << "  [PASS] Feature 3: LinkedForm binds Skip DB pre-filter on Match-v5 By Puuid." << std::endl;

    // F4: Starter Challenger Template Node 2 skip_db
    std::cout << "[Tier 1.3] Testing Feature 4 (Challenger Template Node 2 skip_db)..." << std::endl;
    BatchForm bf;
    assert(TemplateManager::GetInstance().LoadTemplateFromString(TemplateManager::GetInstance().GetStarterChallengerDatabaseJson(), bf));
    assert(bf.get_forms().size() >= 4);
    assert(bf.get_forms()[2]->is_param_skip_db(1) == true);
    assert(bf.get_forms()[2]->is_param_iterative(1) == true);
    std::cout << "  [PASS] Feature 4: Starter Challenger template sets skip_db=true on Node 2 PUUID." << std::endl;

    // F5 & F6: DuckDB Timeline Frames & Events Schema and Unnesting
    std::cout << "[Tier 1.4] Testing Feature 5 & 6 (Timeline Unnesting & SaveTimeline)..." << std::endl;
    std::string timeline_json = CreateSampleTimelineJson("KR_T1_001", 3);
    assert(db.SaveTimeline("KR_T1_001", timeline_json));
    assert(db.HasTimeline("KR_T1_001"));

    // Check tables in DuckDB directly (Opaque-box Oracle)
    duckdb_database raw_db;
    duckdb_connection raw_conn;
    assert(duckdb_open(TEST_DB_PATH.c_str(), &raw_db) != DuckDBError);
    assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

    duckdb_result pf_res;
    if (duckdb_query(raw_conn, "SELECT COUNT(*) FROM timeline_participant_frames WHERE match_id = 'KR_T1_001';", &pf_res) != DuckDBError) {
        int64_t pf_count = duckdb_value_int64(&pf_res, 0, 0);
        duckdb_destroy_result(&pf_res);
        assert(pf_count == 3 * 4); // 3 frames * 4 participants = 12

        duckdb_result ev_res;
        assert(duckdb_query(raw_conn, "SELECT COUNT(*) FROM timeline_events WHERE match_id = 'KR_T1_001';", &ev_res) != DuckDBError);
        int64_t ev_count = duckdb_value_int64(&ev_res, 0, 0);
        duckdb_destroy_result(&ev_res);
        assert(ev_count == 4); // 2 events in frame 1, 2 events in frame 2 = 4
        std::cout << "  [PASS] Feature 5 & 6: timeline_participant_frames (12 rows) and timeline_events (4 rows) verified in DuckDB." << std::endl;
    } else {
        duckdb_destroy_result(&pf_res);
        std::cout << "  [PENDING - Milestone 2] Table timeline_participant_frames not yet created in schema." << std::endl;
    }

    duckdb_disconnect(&raw_conn);
    duckdb_close(&raw_db);

    // F7: BackfillUnnestedTimelines
    std::cout << "[Tier 1.5] Testing Feature 7 (BackfillUnnestedTimelines)..." << std::endl;
    if constexpr (HasBackfillUnnestedTimelines<DB>) {
        int64_t backfilled = db.BackfillUnnestedTimelines();
        // Since KR_T1_001 was already saved with unnesting, backfill count should be 0
        assert(backfilled == 0);
        std::cout << "  [PASS] Feature 7: BackfillUnnestedTimelines executed cleanly on existing data." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 2] BackfillUnnestedTimelines not yet declared on DatabaseManager." << std::endl;
    }

    // F8: Analytical Query Runner & CSV Exporter
    std::cout << "[Tier 1.6] Testing Feature 8 (Analytical Query Runner & CSV Exporter)..." << std::endl;
    if constexpr (HasExecuteQuery<DB> && HasGetTableCounts<DB>) {
        auto counts = db.GetTableCounts();
        assert(counts.matches >= 1);
        assert(counts.match_timelines >= 1);

        auto qres = db.ExecuteQuery("SELECT match_id, queue_id FROM matches;");
        assert(qres.success == true);
        assert(qres.row_count >= 1);
        assert(qres.column_names.size() == 2);
        assert(qres.column_names[0] == "match_id");

        if constexpr (HasExportQueryToCSV<DB>) {
            std::string csv_path = TEST_EXPORTS_DIR + "/tier1_matches.csv";
            assert(db.ExportQueryToCSV("SELECT match_id, queue_id FROM matches;", csv_path));
            assert(std::filesystem::exists(csv_path));
            assert(std::filesystem::file_size(csv_path) > 0);
            std::cout << "  [PASS] Feature 8: ExecuteQuery, GetTableCounts, and ExportQueryToCSV verified." << std::endl;
        }
    } else {
        std::cout << "  [PENDING - Milestone 3] ExecuteQuery and GetTableCounts not yet declared on DatabaseManager." << std::endl;
    }

    db.Close();
}

// =============================================================================
// TIER 2: Boundary & Corner Cases
// =============================================================================
template <typename DB = DatabaseManager>
void TestTier2_BoundaryAndCornerCases(DB& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Tier 2] Boundary & Corner Cases" << std::endl;
    std::cout << "========================================================" << std::endl;

    ResetTestWorkspace();
    assert(db.Init(TEST_DB_PATH));

    // B1: Pre-filtering Boundary Conditions
    std::cout << "[Tier 2.1] Pre-filtering Boundaries (Empty strings, zero/negative thresholds)..." << std::endl;
    if constexpr (HasHasMatchesForPlayerPuuid<DB>) {
        // Empty PUUID must safely return false
        assert(db.HasMatchesForPlayerPuuid("", 1) == false);
        assert(db.HasMatchesForPlayerPuuid("   ", 1) == false);

        // Zero or negative min_matches returns true per specification
        assert(db.HasMatchesForPlayerPuuid("ANY_PUUID", 0) == true);
        assert(db.HasMatchesForPlayerPuuid("ANY_PUUID", -1) == true);

        // Non-existent PUUID with min_matches >= 1 returns false
        assert(db.HasMatchesForPlayerPuuid("NON_EXISTENT_PUUID_999", 1) == false);

        // Player registered in players table but with 0 matches in match_participants
        db.ResolveOrCreatePlayer("DEV1", "PUUID_ZERO_MATCHES", "NoMatchPlayer", "KR1");
        assert(db.HasMatchesForPlayerPuuid("PUUID_ZERO_MATCHES", 1) == false);
        std::cout << "  [PASS] Pre-filtering boundary conditions verified." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 1] HasMatchesForPlayerPuuid not yet implemented." << std::endl;
    }

    // B2: Uninitialized Database Manager Guard
    std::cout << "[Tier 2.2] DatabaseManager Uninitialized Safety Guard..." << std::endl;
    db.Close();
    assert(!db.IsInitialized());
    if constexpr (HasHasMatchesForPlayerPuuid<DB>) {
        assert(db.HasMatchesForPlayerPuuid("ANY_PUUID", 1) == false);
    }
    assert(db.HasMatch("KR_ANY") == false);
    assert(db.HasTimeline("KR_ANY") == false);
    assert(db.GetMatchCount() == 0);
    assert(db.GetTimelineCount() == 0);
    std::cout << "  [PASS] Uninitialized DatabaseManager operations safely return false/0 without crash." << std::endl;

    // Reopen DB for subsequent boundary tests
    assert(db.Init(TEST_DB_PATH));

    // B3: Timeline Parsing Boundaries (Missing attributes, null positions, empty frames)
    std::cout << "[Tier 2.3] Timeline Parsing Boundaries (Missing positions, null damageStats, empty arrays)..." << std::endl;
    std::string boundary_timeline_json = R"({
        "metadata": { "matchId": "KR_BOUNDARY_001" },
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
                            "jungleMinionsKilled": 0
                        }
                    },
                    "events": [
                        { "timestamp": 1000, "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1055 },
                        { "timestamp": 2000, "type": "" },
                        { "timestamp": 3000 }
                    ]
                }
            ]
        }
    })";

    assert(db.SaveTimeline("KR_BOUNDARY_001", boundary_timeline_json));
    assert(db.HasTimeline("KR_BOUNDARY_001"));

    // Verify missing position was defaulted to 0,0 and invalid events were omitted
    duckdb_database raw_db;
    duckdb_connection raw_conn;
    assert(duckdb_open(TEST_DB_PATH.c_str(), &raw_db) != DuckDBError);
    assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

    duckdb_result b_res;
    if (duckdb_query(raw_conn, "SELECT pos_x, pos_y, total_damage_done_to_champions FROM timeline_participant_frames WHERE match_id = 'KR_BOUNDARY_001';", &b_res) != DuckDBError) {
        if (duckdb_row_count(&b_res) > 0) {
            int32_t px = duckdb_value_int32(&b_res, 0, 0);
            int32_t py = duckdb_value_int32(&b_res, 1, 0);
            int32_t dmg = duckdb_value_int32(&b_res, 2, 0);
            assert(px == 0 && py == 0 && dmg == 0);
            std::cout << "  [PASS] Missing position and damageStats gracefully defaulted to 0." << std::endl;
        }
        duckdb_destroy_result(&b_res);

        duckdb_result ev_res;
        assert(duckdb_query(raw_conn, "SELECT COUNT(*) FROM timeline_events WHERE match_id = 'KR_BOUNDARY_001';", &ev_res) != DuckDBError);
        int64_t valid_events = duckdb_value_int64(&ev_res, 0, 0);
        duckdb_destroy_result(&ev_res);
        assert(valid_events == 1); // Only the ITEM_PURCHASED event with non-empty type was saved
        std::cout << "  [PASS] Empty/omitted event types safely ignored." << std::endl;
    } else {
        duckdb_destroy_result(&b_res);
        std::cout << "  [PENDING - Milestone 2] timeline_participant_frames table pending." << std::endl;
    }

    // B4: Idempotency of SaveTimeline
    std::cout << "[Tier 2.4] Timeline Idempotency (Overwriting duplicate match_id)..." << std::endl;
    assert(db.SaveTimeline("KR_BOUNDARY_001", boundary_timeline_json)); // Save second time
    if (duckdb_query(raw_conn, "SELECT COUNT(*) FROM timeline_participant_frames WHERE match_id = 'KR_BOUNDARY_001';", &b_res) != DuckDBError) {
        int64_t count_after = duckdb_value_int64(&b_res, 0, 0);
        duckdb_destroy_result(&b_res);
        assert(count_after == 1); // Must still be 1, no duplicate rows
        std::cout << "  [PASS] SaveTimeline is idempotent; repeated calls do not duplicate unnested rows." << std::endl;
    } else {
        duckdb_destroy_result(&b_res);
    }

    duckdb_disconnect(&raw_conn);
    duckdb_close(&raw_db);

    // B5: SQL Query Runner Boundaries & Error Guarding
    std::cout << "[Tier 2.5] SQL Query Runner Boundaries (Syntax errors, mutations, empty results)..." << std::endl;
    if constexpr (HasExecuteQuery<DB>) {
        // Empty query
        auto r_empty = db.ExecuteQuery("");
        assert(r_empty.success == false);
        assert(!r_empty.error_message.empty());

        // Syntax error
        auto r_syntax = db.ExecuteQuery("SELECT FROM WHERE 1 = 1;");
        assert(r_syntax.success == false);
        assert(!r_syntax.error_message.empty());

        // Non-existent table
        auto r_nonexistent = db.ExecuteQuery("SELECT * FROM non_existent_dummy_table_xyz;");
        assert(r_nonexistent.success == false);

        // Query returning 0 rows
        auto r_zero = db.ExecuteQuery("SELECT * FROM matches WHERE match_id = 'DOES_NOT_EXIST';");
        assert(r_zero.success == true);
        assert(r_zero.row_count == 0);
        assert(r_zero.rows.empty());
        std::cout << "  [PASS] SQL query runner error handling and 0-row results verified." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 3] ExecuteQuery not yet declared on DatabaseManager." << std::endl;
    }

    db.Close();
}

// =============================================================================
// TIER 3: Cross-Feature Combinations
// =============================================================================
template <typename DB = DatabaseManager>
void TestTier3_CrossFeatureCombinations(DB& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Tier 3] Cross-Feature Combinations" << std::endl;
    std::cout << "========================================================" << std::endl;

    ResetTestWorkspace();
    assert(db.Init(TEST_DB_PATH));

    std::cout << "[Tier 3.1] Full Pipeline: Ladder -> Pre-filtering -> Matches -> Timelines -> Analytics..." << std::endl;

    // 1. Ingest 4 ladder players
    std::vector<std::string> puuids = {
        "PUUID_CHAL_FAKER",
        "PUUID_CHAL_CHOVY",
        "PUUID_CHAL_SHOWMAKER",
        "PUUID_CHAL_BDD"
    };

    db.ResolveOrCreatePlayer("DEV1", puuids[0], "Faker", "KR1");
    db.ResolveOrCreatePlayer("DEV1", puuids[1], "Chovy", "KR1");
    db.ResolveOrCreatePlayer("DEV1", puuids[2], "Showmaker", "KR1");
    db.ResolveOrCreatePlayer("DEV1", puuids[3], "Bdd", "KR1");

    // 2. Ingest match for Faker and Chovy
    std::string match1 = CreateSampleMatchJson("KR_E2E_COMB_01", {
        {puuids[0], "Ahri"},
        {puuids[1], "Azir"}
    });
    assert(db.SaveMatch("KR_E2E_COMB_01", match1));

    // 3. Verify Pre-filtering: Faker & Chovy skipped, Showmaker & Bdd accepted
    if constexpr (HasHasMatchesForPlayerPuuid<DB> && HasSkipPlayersWithMatchesInDB<ValuePool>) {
        auto filter = ValuePool::SkipPlayersWithMatchesInDB(1);
        ValuePool pool;
        pool.set_filter(filter);
        for (const auto& p : puuids) {
            pool.push(p);
        }
        assert(pool.size() == 2); // Showmaker and Bdd
        assert(pool.filtered_out_count() == 2); // Faker and Chovy
        assert(std::string(pool[0].param) == puuids[2]);
        assert(std::string(pool[1].param) == puuids[3]);
        std::cout << "  [PASS] Pre-filtering cleanly parted ingested players from uningested players." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 1] Pre-filtering components pending." << std::endl;
    }

    // 4. Ingest match for Showmaker and Bdd
    std::string match2 = CreateSampleMatchJson("KR_E2E_COMB_02", {
        {puuids[2], "Syndra"},
        {puuids[3], "Orianna"}
    });
    assert(db.SaveMatch("KR_E2E_COMB_02", match2));

    // 5. Ingest rich timelines for both matches
    assert(db.SaveTimeline("KR_E2E_COMB_01", CreateSampleTimelineJson("KR_E2E_COMB_01", 3)));
    assert(db.SaveTimeline("KR_E2E_COMB_02", CreateSampleTimelineJson("KR_E2E_COMB_02", 3)));

    // 6. Cross-table consistency verification in DuckDB
    duckdb_database raw_db;
    duckdb_connection raw_conn;
    assert(duckdb_open(TEST_DB_PATH.c_str(), &raw_db) != DuckDBError);
    assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

    duckdb_result join_res;
    std::string cross_sql = R"(
        SELECT mp.champion_name, pf.total_gold, pf.level
        FROM match_participants mp
        JOIN timeline_participant_frames pf
          ON mp.match_id = pf.match_id AND mp.participant_id = pf.participant_id
        WHERE pf.frame_index = 2
        ORDER BY mp.champion_name;
    )";

    if (duckdb_query(raw_conn, cross_sql.c_str(), &join_res) != DuckDBError) {
        int64_t rows = duckdb_row_count(&join_res);
        assert(rows == 4); // Ahri, Azir, Syndra, Orianna at frame 2
        duckdb_destroy_result(&join_res);
        std::cout << "  [PASS] Cross-table relational JOIN between participants and timeline frames verified." << std::endl;
    } else {
        duckdb_destroy_result(&join_res);
        std::cout << "  [PENDING - Milestone 2] Timeline frames table pending." << std::endl;
    }

    // 7. Verify analytical queries across ingested dataset
    if constexpr (HasExecuteQuery<DB>) {
        auto agg_res = db.ExecuteQuery(R"(
            SELECT champion_name, COUNT(*) AS games, SUM(CASE WHEN win THEN 1 ELSE 0 END) AS wins
            FROM match_participants
            GROUP BY champion_name
            ORDER BY champion_name;
        )");
        assert(agg_res.success == true);
        assert(agg_res.row_count == 4);
        std::cout << "  [PASS] Analytical aggregation query on ingested cross-feature dataset verified." << std::endl;
    }

    duckdb_disconnect(&raw_conn);
    duckdb_close(&raw_db);
    db.Close();
}

// =============================================================================
// TIER 4: Real-World Application Scenarios
// =============================================================================
template <typename DB = DatabaseManager>
void TestTier4_RealWorldScenarios(DB& db) {
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[Tier 4] Real-World Application Scenarios" << std::endl;
    std::cout << "========================================================" << std::endl;

    ResetTestWorkspace();
    assert(db.Init(TEST_DB_PATH));

    // Scenario 4.1: Multi-Run Challenger Batch Advancement (iter_limit Pagination)
    std::cout << "[Tier 4.1] Scenario 4.1: Multi-Run Challenger Batch Advancement..." << std::endl;
    std::vector<std::string> ladder_puuids;
    for (int i = 1; i <= 6; ++i) {
        ladder_puuids.push_back("CHAL_PLAYER_" + std::to_string(i));
        db.ResolveOrCreatePlayer("DEV1", ladder_puuids.back(), "Chal" + std::to_string(i), "KR1");
    }

    if constexpr (HasSkipPlayersWithMatchesInDB<ValuePool>) {
        // Run 1: Pool with iter_limit = 2
        ValuePool pool_run1;
        pool_run1.set_max_capacity(2);
        pool_run1.set_filter(ValuePool::SkipPlayersWithMatchesInDB(1));
        for (const auto& p : ladder_puuids) {
            pool_run1.push(p);
        }
        assert(pool_run1.size() == 2);
        assert(std::string(pool_run1[0].param) == "CHAL_PLAYER_1");
        assert(std::string(pool_run1[1].param) == "CHAL_PLAYER_2");

        // Simulate ingestion of matches for Run 1 players
        db.SaveMatch("KR_RUN1_M1", CreateSampleMatchJson("KR_RUN1_M1", {{"CHAL_PLAYER_1", "Ahri"}}));
        db.SaveMatch("KR_RUN1_M2", CreateSampleMatchJson("KR_RUN1_M2", {{"CHAL_PLAYER_2", "Azir"}}));

        // Run 2: Pool with iter_limit = 2
        ValuePool pool_run2;
        pool_run2.set_max_capacity(2);
        pool_run2.set_filter(ValuePool::SkipPlayersWithMatchesInDB(1));
        for (const auto& p : ladder_puuids) {
            pool_run2.push(p);
        }
        assert(pool_run2.size() == 2);
        assert(std::string(pool_run2[0].param) == "CHAL_PLAYER_3"); // Automatically advanced past 1 & 2!
        assert(std::string(pool_run2[1].param) == "CHAL_PLAYER_4");

        // Ingest matches for Run 2 players
        db.SaveMatch("KR_RUN2_M1", CreateSampleMatchJson("KR_RUN2_M1", {{"CHAL_PLAYER_3", "Zed"}}));
        db.SaveMatch("KR_RUN2_M2", CreateSampleMatchJson("KR_RUN2_M2", {{"CHAL_PLAYER_4", "Orianna"}}));

        // Run 3: Pool with iter_limit = 2
        ValuePool pool_run3;
        pool_run3.set_max_capacity(2);
        pool_run3.set_filter(ValuePool::SkipPlayersWithMatchesInDB(1));
        for (const auto& p : ladder_puuids) {
            pool_run3.push(p);
        }
        assert(pool_run3.size() == 2);
        assert(std::string(pool_run3[0].param) == "CHAL_PLAYER_5"); // Advanced past 1..4!
        assert(std::string(pool_run3[1].param) == "CHAL_PLAYER_6");

        // Ingest matches for Run 3 players
        db.SaveMatch("KR_RUN3_M1", CreateSampleMatchJson("KR_RUN3_M1", {{"CHAL_PLAYER_5", "Kaisa"}}));
        db.SaveMatch("KR_RUN3_M2", CreateSampleMatchJson("KR_RUN3_M2", {{"CHAL_PLAYER_6", "Vayne"}}));

        // Run 4: All 6 players now have matches in DB -> Pool size is 0
        ValuePool pool_run4;
        pool_run4.set_max_capacity(2);
        pool_run4.set_filter(ValuePool::SkipPlayersWithMatchesInDB(1));
        for (const auto& p : ladder_puuids) {
            pool_run4.push(p);
        }
        assert(pool_run4.size() == 0);
        assert(pool_run4.filtered_out_count() == 6);
        std::cout << "  [PASS] Scenario 4.1: Challenger batch pagination smoothly stepped across 3 consecutive runs." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 1] SkipPlayersWithMatchesInDB pending." << std::endl;
    }

    // Scenario 4.2: Timeline Backfill of Legacy Database
    std::cout << "[Tier 4.2] Scenario 4.2: Timeline Backfill of Legacy Database..." << std::endl;
    // Simulate legacy DB insertion: raw JSON in match_timelines only, without unnested tables
    duckdb_database raw_db;
    duckdb_connection raw_conn;
    assert(duckdb_open(TEST_DB_PATH.c_str(), &raw_db) != DuckDBError);
    assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

    std::string legacy_tl1 = CreateSampleTimelineJson("KR_LEGACY_01", 2);
    std::string legacy_tl2 = CreateSampleTimelineJson("KR_LEGACY_02", 2);

    // Insert directly into match_timelines
    duckdb_prepared_statement ins_stmt;
    assert(duckdb_prepare(raw_conn, "INSERT OR REPLACE INTO match_timelines (match_id, frame_interval, raw_json) VALUES ($1, $2, $3);", &ins_stmt) != DuckDBError);
    duckdb_bind_varchar(ins_stmt, 1, "KR_LEGACY_01");
    duckdb_bind_int32(ins_stmt, 2, 60000);
    duckdb_bind_varchar(ins_stmt, 3, legacy_tl1.c_str());
    duckdb_result ins_res;
    assert(duckdb_execute_prepared(ins_stmt, &ins_res) != DuckDBError);
    duckdb_destroy_result(&ins_res);

    duckdb_bind_varchar(ins_stmt, 1, "KR_LEGACY_02");
    duckdb_bind_int32(ins_stmt, 2, 60000);
    duckdb_bind_varchar(ins_stmt, 3, legacy_tl2.c_str());
    assert(duckdb_execute_prepared(ins_stmt, &ins_res) != DuckDBError);
    duckdb_destroy_result(&ins_res);
    duckdb_destroy_prepare(&ins_stmt);

    duckdb_disconnect(&raw_conn);
    duckdb_close(&raw_db);

    if constexpr (HasBackfillUnnestedTimelines<DB>) {
        int64_t count = db.BackfillUnnestedTimelines();
        assert(count == 2); // Both legacy matches backfilled

        // Call again -> must return 0
        int64_t count_again = db.BackfillUnnestedTimelines();
        assert(count_again == 0);
        std::cout << "  [PASS] Scenario 4.2: BackfillUnnestedTimelines migrated 2 legacy rows idempotently." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 2] BackfillUnnestedTimelines pending." << std::endl;
    }

    // Scenario 4.3: Analytical CSV Export Report Generation
    std::cout << "[Tier 4.3] Scenario 4.3: Analytical Report Generation & CSV Export..." << std::endl;
    if constexpr (HasExportQueryToCSV<DB> && HasExportTableToCSV<DB>) {
        std::string champ_csv = TEST_EXPORTS_DIR + "/champion_analytics_report.csv";
        std::string table_csv = TEST_EXPORTS_DIR + "/matches_table_dump.csv";

        assert(db.ExportQueryToCSV(R"(
            SELECT champion_name, COUNT(*) AS picks, SUM(CASE WHEN win THEN 1 ELSE 0 END) AS wins
            FROM match_participants
            GROUP BY champion_name
            ORDER BY picks DESC;
        )", champ_csv));

        assert(std::filesystem::exists(champ_csv));
        std::ifstream c_in(champ_csv);
        std::string header_line;
        std::getline(c_in, header_line);
        assert(header_line.find("champion_name") != std::string::npos);
        assert(header_line.find("picks") != std::string::npos);

        assert(db.ExportTableToCSV("matches", table_csv));
        assert(std::filesystem::exists(table_csv));
        std::ifstream t_in(table_csv);
        std::getline(t_in, header_line);
        assert(header_line.find("match_id") != std::string::npos);
        std::cout << "  [PASS] Scenario 4.3: Analytical queries and tables exported to valid CSV files." << std::endl;
    } else {
        std::cout << "  [PENDING - Milestone 3] ExportQueryToCSV / ExportTableToCSV pending." << std::endl;
    }

    db.Close();
}

// =============================================================================
// MAIN ENTRYPOINT
// =============================================================================
int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "  RESTfulEzreal Comprehensive Opaque-Box E2E Test Suite" << std::endl;
    std::cout << "================================================================" << std::endl;

    // Initialize headless ImGui context for form/template validation
    IMGUI_CHECKVERSION();
    ImGuiContext* imgui_ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);

    DatabaseManager& db = DatabaseManager::GetInstance();

    TestTier1_FeatureCoverage(db);
    TestTier2_BoundaryAndCornerCases(db);
    TestTier3_CrossFeatureCombinations(db);
    TestTier4_RealWorldScenarios(db);

    ImGui::DestroyContext(imgui_ctx);

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  [SUCCESS] All Activated E2E Test Cases Passed Cleanly!" << std::endl;
    std::cout << "================================================================" << std::endl;
    return 0;
}
