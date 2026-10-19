#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <sstream>

#define private public
#define protected public
#include "db/DatabaseManager.h"
#include "duckdb.h"
#include "simdjson.h"
#undef private
#undef protected

using namespace restfulEz;

static const std::string TEST_DIR = "./test_m2_chal2_ws";
static const std::string TEST_DB_PATH = "./test_m2_chal2_ws/ezreal_data.duckdb";

static void ResetWorkspace() {
    std::filesystem::remove_all(TEST_DIR);
    std::filesystem::create_directories(TEST_DIR);
}

// =============================================================================
// TEST 1: Verbatim Schema Conformance & DuckDB Table Introspection
// =============================================================================
static void Test1_SchemaConformanceVerbatim(DatabaseManager& db) {
    std::cout << "[Test 1] Schema Conformance Verbatim via PRAGMA table_info..." << std::endl;

    // 1A. Introspect timeline_participant_frames
    std::vector<std::pair<std::string, std::string>> expected_pf_cols = {
        {"match_id", "VARCHAR"},
        {"frame_index", "INTEGER"},
        {"participant_id", "INTEGER"},
        {"timestamp", "BIGINT"},
        {"total_gold", "INTEGER"},
        {"current_gold", "INTEGER"},
        {"xp", "INTEGER"},
        {"level", "INTEGER"},
        {"minions_killed", "INTEGER"},
        {"jungle_minions_killed", "INTEGER"},
        {"pos_x", "INTEGER"},
        {"pos_y", "INTEGER"},
        {"total_damage_done_to_champions", "INTEGER"}
    };

    duckdb_result pf_info;
    assert(duckdb_query(db.conn_, "PRAGMA table_info('timeline_participant_frames');", &pf_info) != DuckDBError);
    idx_t pf_col_count = duckdb_row_count(&pf_info);
    assert(pf_col_count == expected_pf_cols.size());

    std::set<std::string> pf_pks;
    for (idx_t i = 0; i < pf_col_count; ++i) {
        char* col_name = duckdb_value_varchar(&pf_info, 1, i);
        char* col_type = duckdb_value_varchar(&pf_info, 2, i);
        int64_t pk = duckdb_value_int64(&pf_info, 5, i);

        std::string name_str = col_name ? col_name : "";
        std::string type_str = col_type ? col_type : "";
        if (col_name) duckdb_free(col_name);
        if (col_type) duckdb_free(col_type);

        assert(name_str == expected_pf_cols[i].first);
        assert(type_str == expected_pf_cols[i].second);
        if (pk > 0) {
            pf_pks.insert(name_str);
        }
    }
    duckdb_destroy_result(&pf_info);

    // Composite primary key: (match_id, frame_index, participant_id)
    assert(pf_pks.size() == 3);
    assert(pf_pks.count("match_id") == 1);
    assert(pf_pks.count("frame_index") == 1);
    assert(pf_pks.count("participant_id") == 1);

    // 1B. Introspect timeline_events
    std::vector<std::pair<std::string, std::string>> expected_ev_cols = {
        {"match_id", "VARCHAR"},
        {"frame_index", "INTEGER"},
        {"event_index", "INTEGER"},
        {"timestamp", "BIGINT"},
        {"event_type", "VARCHAR"},
        {"killer_id", "INTEGER"},
        {"victim_id", "INTEGER"},
        {"creator_id", "INTEGER"},
        {"participant_id", "INTEGER"},
        {"item_id", "INTEGER"},
        {"monster_type", "VARCHAR"},
        {"monster_sub_type", "VARCHAR"},
        {"building_type", "VARCHAR"},
        {"lane_type", "VARCHAR"},
        {"team_id", "INTEGER"},
        {"pos_x", "INTEGER"},
        {"pos_y", "INTEGER"},
        {"assisting_participant_ids", "VARCHAR"}
    };

    duckdb_result ev_info;
    assert(duckdb_query(db.conn_, "PRAGMA table_info('timeline_events');", &ev_info) != DuckDBError);
    idx_t ev_col_count = duckdb_row_count(&ev_info);
    assert(ev_col_count == expected_ev_cols.size());

    std::set<std::string> ev_pks;
    for (idx_t i = 0; i < ev_col_count; ++i) {
        char* col_name = duckdb_value_varchar(&ev_info, 1, i);
        char* col_type = duckdb_value_varchar(&ev_info, 2, i);
        int64_t pk = duckdb_value_int64(&ev_info, 5, i);

        std::string name_str = col_name ? col_name : "";
        std::string type_str = col_type ? col_type : "";
        if (col_name) duckdb_free(col_name);
        if (col_type) duckdb_free(col_type);

        assert(name_str == expected_ev_cols[i].first);
        assert(type_str == expected_ev_cols[i].second);
        if (pk > 0) {
            ev_pks.insert(name_str);
        }
    }
    duckdb_destroy_result(&ev_info);

    // Composite primary key: (match_id, frame_index, event_index)
    assert(ev_pks.size() == 3);
    assert(ev_pks.count("match_id") == 1);
    assert(ev_pks.count("frame_index") == 1);
    assert(ev_pks.count("event_index") == 1);

    std::cout << "  [PASS] 100% Verbatim column names, types, and composite primary keys verified." << std::endl;
}

// =============================================================================
// TEST 2: Extraction of All 12 Key Analytical Event Types
// =============================================================================
static void Test2_ExtractionOfAllKeyEvents(DatabaseManager& db) {
    std::cout << "[Test 2] Comprehensive extraction of all 12 key analytical event types..." << std::endl;

    std::string match_id = "KR_ALL_EVENTS_001";

    std::string timeline_json = R"({
      "metadata": { "matchId": "KR_ALL_EVENTS_001" },
      "info": {
        "frameInterval": 60000,
        "frames": [
          {
            "timestamp": 0,
            "participantFrames": {
              "1": { "participantId": 1, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1, "minionsKilled": 0, "jungleMinionsKilled": 0, "position": {"x": 100, "y": 200}, "damageStats": {"totalDamageDoneToChampions": 0} },
              "2": { "participantId": 2, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1, "minionsKilled": 0, "jungleMinionsKilled": 0, "position": {"x": 110, "y": 210}, "damageStats": {"totalDamageDoneToChampions": 0} }
            },
            "events": [
              { "timestamp": 12000, "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1055 },
              { "timestamp": 15000, "type": "ITEM_SOLD", "participantId": 1, "itemId": 1055 },
              { "timestamp": 18000, "type": "ITEM_DESTROYED", "participantId": 1, "itemId": 2003 },
              { "timestamp": 20000, "type": "ITEM_UNDO", "participantId": 1, "beforeId": 3057, "afterId": 3078 }
            ]
          },
          {
            "timestamp": 60000,
            "participantFrames": {
              "1": { "participantId": 1, "totalGold": 1200, "currentGold": 600, "xp": 750, "level": 2, "minionsKilled": 12, "jungleMinionsKilled": 0, "position": {"x": 3000, "y": 4000}, "damageStats": {"totalDamageDoneToChampions": 500} },
              "2": { "participantId": 2, "totalGold": 1100, "currentGold": 500, "xp": 700, "level": 2, "minionsKilled": 10, "jungleMinionsKilled": 0, "position": {"x": 3100, "y": 4100}, "damageStats": {"totalDamageDoneToChampions": 300} }
            },
            "events": [
              { "timestamp": 70000, "type": "WARD_PLACED", "creatorId": 1 },
              { "timestamp": 75000, "type": "WARD_KILL", "killerId": 2 },
              { "timestamp": 80000, "type": "SKILL_LEVEL_UP", "participantId": 1, "skillSlot": 1 },
              { "timestamp": 85000, "type": "LEVEL_UP", "participantId": 1, "level": 2 }
            ]
          },
          {
            "timestamp": 120000,
            "participantFrames": {
              "1": { "participantId": 1, "totalGold": 2500, "currentGold": 1000, "xp": 1800, "level": 4, "minionsKilled": 28, "jungleMinionsKilled": 0, "position": {"x": 5000, "y": 6000}, "damageStats": {"totalDamageDoneToChampions": 1200} },
              "2": { "participantId": 2, "totalGold": 2100, "currentGold": 800, "xp": 1600, "level": 4, "minionsKilled": 22, "jungleMinionsKilled": 0, "position": {"x": 5100, "y": 6100}, "damageStats": {"totalDamageDoneToChampions": 900} }
            },
            "events": [
              { "timestamp": 130000, "type": "CHAMPION_KILL", "killerId": 1, "victimId": 2, "assistingParticipantIds": [3, 4], "position": { "x": 5500, "y": 6500 } },
              { "timestamp": 140000, "type": "ELITE_MONSTER_KILL", "killerId": 1, "monsterType": "DRAGON", "monsterSubType": "WATER_DRAGON", "killerTeamId": 100, "position": { "x": 9800, "y": 4400 } },
              { "timestamp": 150000, "type": "BUILDING_KILL", "killerId": 1, "teamId": 200, "buildingType": "TOWER_BUILDING", "laneType": "BOT_LANE", "position": { "x": 10500, "y": 2500 } },
              { "timestamp": 160000, "type": "TURRET_PLATE_DESTROYED", "killerId": 1, "teamId": 200, "laneType": "BOT_LANE", "position": { "x": 10500, "y": 2500 } }
            ]
          }
        ]
      }
    })";

    assert(db.SaveTimeline(match_id, timeline_json));
    assert(db.GetTimelineParticipantFrameCount() == 6);
    assert(db.GetTimelineEventCount() == 12);

    // Verify all 12 events were stored
    duckdb_result count_res;
    assert(duckdb_query(db.conn_, "SELECT COUNT(*) FROM timeline_events WHERE match_id = 'KR_ALL_EVENTS_001';", &count_res) != DuckDBError);
    assert(duckdb_value_int64(&count_res, 0, 0) == 12);
    duckdb_destroy_result(&count_res);

    // Verify each event type and its respective specialized fields
    auto check_event = [&](const std::string& type, const std::string& extra_where) {
        std::string q = "SELECT COUNT(*) FROM timeline_events WHERE match_id = 'KR_ALL_EVENTS_001' AND event_type = '" + type + "' AND " + extra_where + ";";
        duckdb_result r;
        assert(duckdb_query(db.conn_, q.c_str(), &r) != DuckDBError);
        int64_t c = duckdb_value_int64(&r, 0, 0);
        duckdb_destroy_result(&r);
        if (c != 1) {
            std::cerr << "FAILED check_event for " << type << " (" << extra_where << "): count=" << c << std::endl;
        }
        assert(c == 1);
    };

    check_event("ITEM_PURCHASED", "participant_id = 1 AND item_id = 1055");
    check_event("ITEM_SOLD", "participant_id = 1 AND item_id = 1055");
    check_event("ITEM_DESTROYED", "participant_id = 1 AND item_id = 2003");
    check_event("ITEM_UNDO", "participant_id = 1 AND item_id = 3078"); // afterId parsed into item_id
    check_event("WARD_PLACED", "creator_id = 1");
    check_event("WARD_KILL", "killer_id = 2");
    check_event("SKILL_LEVEL_UP", "participant_id = 1");
    check_event("LEVEL_UP", "participant_id = 1");
    check_event("CHAMPION_KILL", "killer_id = 1 AND victim_id = 2 AND assisting_participant_ids = '3,4' AND pos_x = 5500 AND pos_y = 6500");
    check_event("ELITE_MONSTER_KILL", "killer_id = 1 AND monster_type = 'DRAGON' AND monster_sub_type = 'WATER_DRAGON' AND team_id = 100 AND pos_x = 9800");
    check_event("BUILDING_KILL", "killer_id = 1 AND team_id = 200 AND building_type = 'TOWER_BUILDING' AND lane_type = 'BOT_LANE' AND pos_x = 10500");
    check_event("TURRET_PLATE_DESTROYED", "killer_id = 1 AND team_id = 200 AND lane_type = 'BOT_LANE' AND pos_x = 10500");

    std::cout << "  [PASS] All 12 analytical event types extracted with exact column values." << std::endl;
}

// =============================================================================
// TEST 3: Relational JOINs between match_participants and timeline_participant_frames
// =============================================================================
static void Test3_RelationalQueriesAndJoins(DatabaseManager& db) {
    std::cout << "[Test 3] Relational JOINs (e.g. champion gold at 10m / 15m)..." << std::endl;

    std::string match_id = "KR_REL_MATCH_100";

    // 10 participants: Blue (1..5), Red (6..10)
    std::vector<std::string> champions = {
        "Aatrox", "LeeSin", "Ahri", "Ezreal", "Nautilus",
        "Ornn", "Sejuani", "Azir", "Jinx", "Lulu"
    };

    std::ostringstream mss;
    mss << "{\n"
        << "  \"metadata\": { \"matchId\": \"" << match_id << "\" },\n"
        << "  \"info\": {\n"
        << "    \"gameCreation\": 1700000000000,\n"
        << "    \"gameDuration\": 1800,\n"
        << "    \"gameVersion\": \"14.10.1\",\n"
        << "    \"queueId\": 420,\n"
        << "    \"participants\": [\n";
    for (int i = 0; i < 10; ++i) {
        int pid = i + 1;
        int team = (pid <= 5) ? 100 : 200;
        bool win = (team == 100);
        mss << "      {\n"
            << "        \"participantId\": " << pid << ",\n"
            << "        \"puuid\": \"PUUID_P" << pid << "\",\n"
            << "        \"riotIdGameName\": \"Player" << pid << "\",\n"
            << "        \"riotIdTagline\": \"KR1\",\n"
            << "        \"championName\": \"" << champions[i] << "\",\n"
            << "        \"championId\": " << (100 + pid) << ",\n"
            << "        \"teamId\": " << team << ",\n"
            << "        \"kills\": " << pid << ",\n"
            << "        \"deaths\": 2,\n"
            << "        \"assists\": 5,\n"
            << "        \"win\": " << (win ? "true" : "false") << "\n"
            << "      }" << (i < 9 ? "," : "") << "\n";
    }
    mss << "    ]\n  }\n}";

    assert(db.SaveMatch(match_id, mss.str()));

    // Generate timeline with 16 frames: 0m to 15m
    std::ostringstream tss;
    tss << "{\n"
        << "  \"metadata\": { \"matchId\": \"" << match_id << "\" },\n"
        << "  \"info\": {\n"
        << "    \"frameInterval\": 60000,\n"
        << "    \"frames\": [\n";

    for (int f = 0; f <= 15; ++f) {
        tss << "      {\n"
            << "        \"timestamp\": " << (f * 60000) << ",\n"
            << "        \"participantFrames\": {\n";
        for (int p = 1; p <= 10; ++p) {
            // Blue team (p 1..5) gets 400 + p*20, Red team (p 6..10) gets 250 + p*10
            int gold_per_min = (p <= 5) ? (400 + p * 20) : (250 + p * 10);
            int total_gold = 500 + f * gold_per_min;
            int xp = f * 450 + p * 50;
            int level = 1 + f / 2;
            int cs = f * (p <= 5 ? 8 : 7);
            int dmg = f * (p <= 5 ? 400 : 350);


            tss << "          \"" << p << "\": {\n"
                << "            \"participantId\": " << p << ",\n"
                << "            \"totalGold\": " << total_gold << ",\n"
                << "            \"currentGold\": " << (total_gold / 3) << ",\n"
                << "            \"xp\": " << xp << ",\n"
                << "            \"level\": " << level << ",\n"
                << "            \"minionsKilled\": " << cs << ",\n"
                << "            \"jungleMinionsKilled\": 0,\n"
                << "            \"position\": { \"x\": " << (1000 * p) << ", \"y\": " << (1000 * p) << " },\n"
                << "            \"damageStats\": { \"totalDamageDoneToChampions\": " << dmg << " }\n"
                << "          }" << (p < 10 ? "," : "") << "\n";
        }
        tss << "        },\n"
            << "        \"events\": []\n"
            << "      }" << (f < 15 ? "," : "") << "\n";
    }
    tss << "    ]\n  }\n}";

    assert(db.SaveTimeline(match_id, tss.str()));

    // Relational Query 1: Champion gold at 10m (frame_index = 10)
    const char* sql_10m = R"(
        SELECT mp.champion_name, mp.team_id, pf.total_gold, pf.level, pf.minions_killed
        FROM match_participants mp
        JOIN timeline_participant_frames pf
          ON mp.match_id = pf.match_id AND mp.participant_id = pf.participant_id
        WHERE pf.match_id = 'KR_REL_MATCH_100' AND pf.frame_index = 10
        ORDER BY pf.total_gold DESC;
    )";

    duckdb_result r10;
    assert(duckdb_query(db.conn_, sql_10m, &r10) != DuckDBError);
    assert(duckdb_row_count(&r10) == 10);
    // Highest gold at 10m should be participant 5 (Nautilus: 500 + 10 * 450 = 5000)
    char* top_champ = duckdb_value_varchar(&r10, 0, 0);
    int32_t top_gold = duckdb_value_int32(&r10, 2, 0);
    assert(std::string(top_champ) == "Nautilus");
    assert(top_gold == 5500);

    duckdb_free(top_champ);
    duckdb_destroy_result(&r10);

    // Relational Query 2: Team Gold differential at 15m (frame_index = 15)
    const char* sql_diff_15m = R"(
        SELECT 
            SUM(CASE WHEN mp.team_id = 100 THEN pf.total_gold ELSE 0 END) AS blue_gold,
            SUM(CASE WHEN mp.team_id = 200 THEN pf.total_gold ELSE 0 END) AS red_gold,
            SUM(CASE WHEN mp.team_id = 100 THEN pf.total_gold ELSE 0 END) - 
            SUM(CASE WHEN mp.team_id = 200 THEN pf.total_gold ELSE 0 END) AS gold_diff
        FROM match_participants mp
        JOIN timeline_participant_frames pf
          ON mp.match_id = pf.match_id AND mp.participant_id = pf.participant_id
        WHERE pf.match_id = 'KR_REL_MATCH_100' AND pf.frame_index = 15;
    )";

    duckdb_result rdiff;
    assert(duckdb_query(db.conn_, sql_diff_15m, &rdiff) != DuckDBError);
    assert(duckdb_row_count(&rdiff) == 1);
    int64_t blue_gold = duckdb_value_int64(&rdiff, 0, 0);
    int64_t red_gold = duckdb_value_int64(&rdiff, 1, 0);
    int64_t diff = duckdb_value_int64(&rdiff, 2, 0);
    assert(blue_gold > red_gold);
    assert(diff == (blue_gold - red_gold));
    assert(diff > 0);
    duckdb_destroy_result(&rdiff);

    std::cout << "  [PASS] Relational JOINs between match_participants and timeline_participant_frames verified." << std::endl;
}

// =============================================================================
// TEST 4: Boundary, High Scale & Adversarial Resilience
// =============================================================================
static void Test4_BoundaryAndScaleResilience(DatabaseManager& db) {
    std::cout << "[Test 4] Boundary, Scale & Adversarial Resilience..." << std::endl;

    // 4A: High frame scale (60 frames * 10 participants = 600 rows in single timeline)
    std::string scale_mid = "KR_SCALE_60MIN";
    std::ostringstream sss;
    sss << "{\n"
        << "  \"metadata\": { \"matchId\": \"" << scale_mid << "\" },\n"
        << "  \"info\": {\n"
        << "    \"frameInterval\": 60000,\n"
        << "    \"frames\": [\n";

    for (int f = 0; f < 60; ++f) {
        sss << "      {\n"
            << "        \"timestamp\": " << (f * 60000) << ",\n"
            << "        \"participantFrames\": {\n";
        for (int p = 1; p <= 10; ++p) {
            sss << "          \"" << p << "\": {\n"
                << "            \"participantId\": " << p << ",\n"
                << "            \"totalGold\": " << (500 + f * 300) << ",\n"
                << "            \"currentGold\": 200,\n"
                << "            \"xp\": " << (f * 400) << ",\n"
                << "            \"level\": " << (1 + f / 3) << ",\n"
                << "            \"minionsKilled\": " << (f * 5) << ",\n"
                << "            \"jungleMinionsKilled\": 0\n"
                << "          }" << (p < 10 ? "," : "") << "\n";
        }
        sss << "        },\n"
            << "        \"events\": []\n"
            << "      }" << (f < 59 ? "," : "") << "\n";
    }
    sss << "    ]\n  }\n}";

    assert(db.SaveTimeline(scale_mid, sss.str()));

    duckdb_result scale_res;
    assert(duckdb_query(db.conn_, "SELECT COUNT(*) FROM timeline_participant_frames WHERE match_id = 'KR_SCALE_60MIN';", &scale_res) != DuckDBError);
    assert(duckdb_value_int64(&scale_res, 0, 0) == 600);
    duckdb_destroy_result(&scale_res);

    // 4B: Repeated overwrite idempotency
    assert(db.SaveTimeline(scale_mid, sss.str()));
    assert(duckdb_query(db.conn_, "SELECT COUNT(*) FROM timeline_participant_frames WHERE match_id = 'KR_SCALE_60MIN';", &scale_res) != DuckDBError);
    assert(duckdb_value_int64(&scale_res, 0, 0) == 600);
    duckdb_destroy_result(&scale_res);

    // 4C: Malformed JSON rejection and metadata matchId fallback
    assert(!db.SaveTimeline("KR_BAD_01", "not json"));
    assert(!db.SaveTimeline("KR_BAD_02", "{\"status\": {\"message\": \"Data not found\", \"status_code\": 404}}"));
    assert(!db.SaveTimeline("KR_BAD_03", "{\"metadata\": {\"matchId\": \"\"}}"));
    // Empty match_id argument falls back to doc["metadata"]["matchId"]
    assert(db.SaveTimeline("", sss.str()));
    // If both match_id argument and JSON metadata.matchId are empty, reject
    std::string no_mid_json = R"({ "info": { "frameInterval": 60000, "frames": [] } })";
    assert(!db.SaveTimeline("", no_mid_json));


    // 4D: Backfill legacy matches
    // Insert 2 legacy records into match_timelines directly
    for (int i = 1; i <= 2; ++i) {
        std::string leg_mid = "KR_LEG_BACKFILL_" + std::to_string(i);
        std::string leg_json = R"({
          "metadata": { "matchId": ")" + leg_mid + R"(" },
          "info": {
            "frameInterval": 60000,
            "frames": [
              {
                "timestamp": 0,
                "participantFrames": {
                  "1": { "participantId": 1, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1, "minionsKilled": 0, "jungleMinionsKilled": 0 }
                },
                "events": [
                  { "timestamp": 1000, "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1054 }
                ]
              }
            ]
          }
        })";
        duckdb_prepared_statement stmt;
        assert(duckdb_prepare(db.conn_, "INSERT OR REPLACE INTO match_timelines (match_id, frame_interval, raw_json) VALUES ($1, $2, $3);", &stmt) != DuckDBError);
        duckdb_bind_varchar(stmt, 1, leg_mid.c_str());
        duckdb_bind_int32(stmt, 2, 60000);
        duckdb_bind_varchar(stmt, 3, leg_json.c_str());
        duckdb_result r;
        assert(duckdb_execute_prepared(stmt, &r) != DuckDBError);
        duckdb_destroy_result(&r);
        duckdb_destroy_prepare(&stmt);
    }

    int64_t backfilled = db.BackfillUnnestedTimelines();
    assert(backfilled == 2);
    assert(db.BackfillUnnestedTimelines() == 0); // Idempotent

    std::cout << "  [PASS] High scale (600 rows), idempotency, malformed rejection, and legacy backfill verified." << std::endl;
}

int main() {
    std::cout << "=====================================================================" << std::endl;
    std::cout << "[Empirical Challenger 2 - Milestone 2] Starting Adversarial Suite" << std::endl;
    std::cout << "=====================================================================" << std::endl;

    ResetWorkspace();
    DatabaseManager& db = DatabaseManager::GetInstance();
    assert(db.Init(TEST_DB_PATH));

    Test1_SchemaConformanceVerbatim(db);
    Test2_ExtractionOfAllKeyEvents(db);
    Test3_RelationalQueriesAndJoins(db);
    Test4_BoundaryAndScaleResilience(db);

    db.Close();

    // Verify uninitialized safety
    assert(db.GetTimelineParticipantFrameCount() == 0);
    assert(db.GetTimelineEventCount() == 0);
    assert(db.BackfillUnnestedTimelines() == 0);

    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[SUCCESS] ALL ADVERSARIAL CHALLENGES FOR MILESTONE 2 CONFIRMED PASS!" << std::endl;
    std::cout << "=====================================================================" << std::endl;
    return 0;
}
