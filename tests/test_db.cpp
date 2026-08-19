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

    // Dev B queries "Hide on bush#KR1" with Dev B's key -> finds existing player by alias and maps Dev B's PUUID!
    std::string p1_dev_b = db.ResolveOrCreatePlayer("DEV_B", "PUUID_B_FAKER", "Hide on bush", "KR1");
    assert(p1_dev_b == p1);
    assert(db.GetPlayerCount() == 1);
    std::cout << "  [PASS] Multi-Developer Player Identity & Alias Resolution" << std::endl;

    // 3. Test SaveAccount (Legacy & Universal)
    bool acc_ok = db.SaveAccount("DEV_A", "PUUID_A_EZREAL", "Ezreal", "NA1");
    assert(acc_ok);
    assert(db.GetPlayerCount() == 2);
    assert(db.GetAccountCount() == 2);
    std::cout << "  [PASS] SaveAccount" << std::endl;

    // 4. Test MergePlayers
    std::string p_temp = db.ResolveOrCreatePlayer("DEV_C", "PUUID_C_TEMP", "TempPlayer", "EUW1");
    assert(db.GetPlayerCount() == 3);
    bool merge_ok = db.MergePlayers(p1, p_temp);
    assert(merge_ok);
    assert(db.GetPlayerCount() == 2);
    std::cout << "  [PASS] MergePlayers" << std::endl;

    // 5. Test SaveMatch & HasMatch & GetMatchCount with JSON extraction
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
                { "participantId": 1, "puuid": "PUUID_A_FAKER", "riotIdGameName": "Hide on bush", "riotIdTagline": "KR1", "championName": "Ahri", "championId": 103, "teamId": 100, "kills": 10, "deaths": 2, "assists": 8, "win": true },
                { "participantId": 2, "puuid": "OTHER_PUUID", "riotIdGameName": "Player2", "riotIdTagline": "KR1", "championName": "LeeSin", "championId": 64, "teamId": 100, "kills": 3, "deaths": 4, "assists": 12, "win": true }
            ]
        }
    })";

    bool match_ok = db.SaveMatch(match_id, sample_match_json, p1);
    assert(match_ok);
    assert(db.HasMatch(match_id));
    assert(!db.HasMatch("NON_EXISTENT_MATCH"));
    assert(db.GetMatchCount() == 1);
    std::cout << "  [PASS] SaveMatch, HasMatch & GetMatchCount" << std::endl;

    // 6. Test SaveTimeline
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
    std::cout << "  [PASS] SaveTimeline" << std::endl;

    // 7. Test Reopen
    bool reopen_ok = db.Reopen(test_db_reopen_path);
    assert(reopen_ok);
    assert(db.IsInitialized());
    assert(db.GetDatabasePath() == test_db_reopen_path);
    assert(db.GetMatchCount() == 0); // New empty DB
    std::cout << "  [PASS] Database Reopen" << std::endl;

    // 8. Cleanup
    db.Close();
    assert(!db.IsInitialized());
    std::cout << "  [PASS] Close Database" << std::endl;

    std::filesystem::remove_all("./test_output");
    std::cout << "[Test] All DuckDB universal integration tests passed successfully!" << std::endl;
    return 0;
}
