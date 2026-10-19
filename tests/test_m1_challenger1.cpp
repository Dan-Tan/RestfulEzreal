#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <sstream>

#include "db/DatabaseManager.h"
#include "BatchRequests.h"

using namespace restfulEz;

static const std::string TEST_DIR = "./test_challenger1_ws";
static const std::string TEST_DB = "./test_challenger1_ws/ezreal_data.duckdb";

static void ResetWs() {
    std::filesystem::remove_all(TEST_DIR);
    std::filesystem::create_directories(TEST_DIR);
}

static std::string MakeMatchJson(const std::string& match_id, const std::vector<std::string>& puuids) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"metadata\": {\"matchId\": \"" << match_id << "\"},\n"
       << "  \"info\": {\n"
       << "    \"gameCreation\": 1700000000000,\n"
       << "    \"gameDuration\": 1800,\n"
       << "    \"gameVersion\": \"14.15.1\",\n"
       << "    \"queueId\": 420,\n"
       << "    \"participants\": [\n";
    for (size_t i = 0; i < puuids.size(); ++i) {
        ss << "      {\n"
           << "        \"participantId\": " << (i + 1) << ",\n"
           << "        \"puuid\": \"" << puuids[i] << "\",\n"
           << "        \"riotIdGameName\": \"Name_" << (i + 1) << "\",\n"
           << "        \"riotIdTagline\": \"KR1\",\n"
           << "        \"championName\": \"Champion_" << (i + 1) << "\",\n"
           << "        \"championId\": " << (100 + i) << ",\n"
           << "        \"teamId\": 100,\n"
           << "        \"kills\": 1,\n"
           << "        \"deaths\": 0,\n"
           << "        \"assists\": 0,\n"
           << "        \"win\": true\n"
           << "      }" << (i + 1 < puuids.size() ? "," : "") << "\n";
    }
    ss << "    ]\n"
       << "  }\n"
       << "}";
    return ss.str();
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Challenger 1 Empirical Stress Test: Feature 1 (Skip DB) " << std::endl;
    std::cout << "==========================================================" << std::endl;

    auto& db = DatabaseManager::GetInstance();

    // =========================================================================
    // Challenge 1: Multi-Developer Key Aliasing & Merge Topology
    // =========================================================================
    std::cout << "\n[Challenge 1] Multi-developer key aliasing and identity merging..." << std::endl;
    ResetWs();
    assert(db.Init(TEST_DB));

    // Developer Key 1 (DEV_A) registers Player Alpha
    std::string pid_alpha = db.ResolveOrCreatePlayer("DEV_A", "PUUID_DEV_A_ALPHA", "Faker", "KR1");
    assert(!pid_alpha.empty());

    // Developer Key 2 (DEV_B) registers same player by Riot ID ("Faker", "KR1")
    std::string pid_beta_resolved = db.ResolveOrCreatePlayer("DEV_B", "PUUID_DEV_B_ALPHA", "Faker", "KR1");
    assert(pid_beta_resolved == pid_alpha); // Resolved to same canonical player ID

    // Developer Key 3 (DEV_C) registers Player Gamma ("Hide on bush", "KR1")
    std::string pid_gamma = db.ResolveOrCreatePlayer("DEV_C", "PUUID_DEV_C_GAMMA", "Hide on bush", "KR1");
    assert(!pid_gamma.empty());
    assert(pid_gamma != pid_alpha);

    // Ingest Match 1 with participant PUUID_DEV_A_ALPHA
    assert(db.SaveMatch("MATCH_001", MakeMatchJson("MATCH_001", {"PUUID_DEV_A_ALPHA"})));

    // Ingest Match 2 with participant PUUID_DEV_B_ALPHA
    assert(db.SaveMatch("MATCH_002", MakeMatchJson("MATCH_002", {"PUUID_DEV_B_ALPHA"})));

    // Ingest Match 3 with participant PUUID_DEV_C_GAMMA
    assert(db.SaveMatch("MATCH_003", MakeMatchJson("MATCH_003", {"PUUID_DEV_C_GAMMA"})));

    // At this point:
    // PUUID_DEV_A_ALPHA has 2 matches (MATCH_001 and MATCH_002, both resolving to pid_alpha)
    // PUUID_DEV_B_ALPHA has 2 matches (same)
    // PUUID_DEV_C_GAMMA has 1 match (MATCH_003, resolving to pid_gamma)
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_A_ALPHA", 2));
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_B_ALPHA", 2));
    assert(!db.HasMatchesForPlayerPuuid("PUUID_DEV_A_ALPHA", 3));
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_C_GAMMA", 1));
    assert(!db.HasMatchesForPlayerPuuid("PUUID_DEV_C_GAMMA", 2));

    // Now execute MergePlayers: Merge Gamma into Alpha
    assert(db.MergePlayers(pid_alpha, pid_gamma));

    // After merge, all 3 matches should resolve to pid_alpha!
    // Testing from all 3 developer PUUIDs as well as canonical player ID
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_A_ALPHA", 3));
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_B_ALPHA", 3));
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_C_GAMMA", 3));
    assert(db.HasMatchesForPlayerPuuid(pid_alpha, 3));
    assert(!db.HasMatchesForPlayerPuuid("PUUID_DEV_A_ALPHA", 4));
    std::cout << "  [PASS] Multi-developer keys and player merging resolved across all encrypted PUUIDs." << std::endl;

    // =========================================================================
    // Challenge 2: Threshold Values (0, 1, 5) and Boundary Transitions
    // =========================================================================
    std::cout << "\n[Challenge 2] Threshold values (min_matches = 0, 1, 5) and boundary transitions..." << std::endl;

    // Register a new player with no matches yet
    std::string pid_tester = db.ResolveOrCreatePlayer("DEV_TEST", "PUUID_THRESH_TEST", "ThreshTester", "KR1");

    // min_matches <= 0 returns true for any non-empty PUUID on initialized DB
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 0) == true);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", -1) == true);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", -99) == true);

    // 0 matches:
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 1) == false);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 5) == false);

    // Add match 1:
    assert(db.SaveMatch("MATCH_T_1", MakeMatchJson("MATCH_T_1", {"PUUID_THRESH_TEST"})));
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 1) == true);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 2) == false);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 5) == false);

    // Add matches 2, 3, 4:
    assert(db.SaveMatch("MATCH_T_2", MakeMatchJson("MATCH_T_2", {"PUUID_THRESH_TEST"})));
    assert(db.SaveMatch("MATCH_T_3", MakeMatchJson("MATCH_T_3", {"PUUID_THRESH_TEST"})));
    assert(db.SaveMatch("MATCH_T_4", MakeMatchJson("MATCH_T_4", {"PUUID_THRESH_TEST"})));
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 4) == true);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 5) == false);

    // Add match 5:
    assert(db.SaveMatch("MATCH_T_5", MakeMatchJson("MATCH_T_5", {"PUUID_THRESH_TEST"})));
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 5) == true);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 6) == false);

    // Duplicate match ID ingestion (idempotency check): match_id uniqueness
    assert(db.SaveMatch("MATCH_T_5", MakeMatchJson("MATCH_T_5", {"PUUID_THRESH_TEST"})));
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 5) == true);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 6) == false); // Still 5 distinct matches

    std::cout << "  [PASS] Threshold transitions (0, 1, 5) and idempotency verified." << std::endl;

    // =========================================================================
    // Challenge 3: SQL Injection, Empty Strings, Whitespace, and Extreme Inputs
    // =========================================================================
    std::cout << "\n[Challenge 3] SQL injection resilience, empty strings, and adversarial inputs..." << std::endl;

    // Empty string
    assert(db.HasMatchesForPlayerPuuid("", 1) == false);
    assert(db.HasMatchesForPlayerPuuid("", 0) == false);
    assert(db.HasMatchesForPlayerPuuid("", -1) == false);

    // Whitespace strings
    assert(db.HasMatchesForPlayerPuuid(" ", 1) == false);
    assert(db.HasMatchesForPlayerPuuid("   ", 1) == false);
    assert(db.HasMatchesForPlayerPuuid("\t\n", 1) == false);

    // SQL Injection Payloads
    std::vector<std::string> sqli_payloads = {
        "' OR '1'='1",
        "'; DROP TABLE matches; --",
        "'; DROP TABLE match_participants; --",
        "'; DELETE FROM players; --",
        "' UNION SELECT * FROM dev_puuid_mappings; --",
        "admin'--",
        "' OR 1=1; --",
        "PUUID' OR '1'='1"
    };

    for (const auto& payload : sqli_payloads) {
        bool res = db.HasMatchesForPlayerPuuid(payload, 1);
        assert(res == false); // No matches returned, no injection executed
    }

    // Verify tables still exist and contain expected data
    assert(db.GetMatchCount() >= 5);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 5) == true);

    // Extremely long string (5000 characters)
    std::string long_str(5000, 'X');
    assert(db.HasMatchesForPlayerPuuid(long_str, 1) == false);
    assert(db.HasMatchesForPlayerPuuid(long_str, 0) == true); // non-empty, min_matches <= 0

    std::cout << "  [PASS] SQL injection defense and adversarial inputs completely safe." << std::endl;

    // =========================================================================
    // Challenge 4: Uninitialized DB States & Reopen Behavior
    // =========================================================================
    std::cout << "\n[Challenge 4] Uninitialized DB states and reopen resilience..." << std::endl;

    // Close the database
    db.Close();
    assert(!db.IsInitialized());

    // When DB is uninitialized, HasMatchesForPlayerPuuid must safely return false
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_A_ALPHA", 1) == false);
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_A_ALPHA", 0) == false);
    assert(db.HasMatchesForPlayerPuuid("", 1) == false);

    // When DB is uninitialized, SkipPlayersWithMatchesInDB must return TRUE (do not discard queries)
    auto uninit_filter = ValuePool::SkipPlayersWithMatchesInDB(1);
    assert(uninit_filter("PUUID_DEV_A_ALPHA") == true);
    assert(uninit_filter("ANY_PUUID") == true);

    // Reopen DB
    assert(db.Init(TEST_DB));
    assert(db.IsInitialized());

    // Verified data persisted across reopen
    assert(db.HasMatchesForPlayerPuuid("PUUID_DEV_A_ALPHA", 3) == true);
    assert(db.HasMatchesForPlayerPuuid("PUUID_THRESH_TEST", 5) == true);
    std::cout << "  [PASS] Uninitialized DB guards and reopen persistence verified." << std::endl;

    // =========================================================================
    // Challenge 5: ValuePool::SkipPlayersWithMatchesInDB Filtering Dynamics
    // =========================================================================
    std::cout << "\n[Challenge 5] ValuePool SkipPlayersWithMatchesInDB dynamic filtering..." << std::endl;

    // Register 3 players:
    // P_V5: 5 matches (PUUID_THRESH_TEST)
    // P_V3: 3 matches (PUUID_DEV_A_ALPHA)
    // P_V0: 0 matches (new player)
    std::string pid_v0 = db.ResolveOrCreatePlayer("DEV_V", "PUUID_ZERO_MATCH", "ZeroMatch", "KR1");

    // Case 5a: min_matches = 5
    // P_V5 has 5 -> skipped (false)
    // P_V3 has 3 (< 5) -> accepted (true)
    // P_V0 has 0 (< 5) -> accepted (true)
    {
        ValuePool pool(false, 0, ValuePool::SkipPlayersWithMatchesInDB(5));
        assert(!pool.push("PUUID_THRESH_TEST")); // Skipped
        assert(pool.push("PUUID_DEV_A_ALPHA"));   // Accepted
        assert(pool.push("PUUID_ZERO_MATCH"));    // Accepted
        assert(pool.size() == 2);
        assert(pool.filtered_out_count() == 1);
    }

    // Case 5b: min_matches = 1
    // P_V5 has 5 (>= 1) -> skipped
    // P_V3 has 3 (>= 1) -> skipped
    // P_V0 has 0 (< 1) -> accepted
    {
        ValuePool pool(false, 0, ValuePool::SkipPlayersWithMatchesInDB(1));
        assert(!pool.push("PUUID_THRESH_TEST")); // Skipped
        assert(!pool.push("PUUID_DEV_A_ALPHA"));  // Skipped
        assert(pool.push("PUUID_ZERO_MATCH"));   // Accepted
        assert(pool.size() == 1);
        assert(pool.filtered_out_count() == 2);
    }

    // Case 5c: min_matches = 0
    // Every non-empty PUUID has >= 0 matches -> all skipped!
    {
        ValuePool pool(false, 0, ValuePool::SkipPlayersWithMatchesInDB(0));
        assert(!pool.push("PUUID_THRESH_TEST"));
        assert(!pool.push("PUUID_DEV_A_ALPHA"));
        assert(!pool.push("PUUID_ZERO_MATCH"));
        assert(pool.size() == 0);
        assert(pool.filtered_out_count() == 3);
    }

    std::cout << "  [PASS] ValuePool dynamic filtering matches exact specification." << std::endl;

    db.Close();
    ResetWs();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "  ALL EMPIRICAL CHALLENGES PASSED! VERDICT: CONFIRMED     " << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
