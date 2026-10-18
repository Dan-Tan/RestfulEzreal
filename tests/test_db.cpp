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
    assert(!db.SaveTimeline("KR_ERROR_404", error_json));
    assert(!db.SaveTimeline("KR_MALFORMED", "invalid_json"));
    assert(db.GetTimelineCount() == 1);
    std::cout << "  [PASS] SaveTimeline, HasTimeline & Error Rejection" << std::endl;

    // 7. Test Reopen
    bool reopen_ok = db.Reopen(test_db_reopen_path);
    assert(reopen_ok);
    assert(db.IsInitialized());
    assert(db.GetDatabasePath() == test_db_reopen_path);
    assert(db.GetMatchCount() == 0); // New empty DB
    assert(db.GetTimelineCount() == 0);
    std::cout << "  [PASS] Database Reopen" << std::endl;

    // 8. Cleanup
    db.Close();
    assert(!db.IsInitialized());
    std::cout << "  [PASS] Close Database" << std::endl;

    std::filesystem::remove_all("./test_output");
    std::cout << "[Test] All DuckDB universal integration tests passed successfully!" << std::endl;
    return 0;
}
