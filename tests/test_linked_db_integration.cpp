#include "BatchRequests.h"
#include "RequestQueue.h"
#include "FileManager.h"
#include "db/DatabaseManager.h"
#include <iostream>
#include <filesystem>
#include <cassert>
#include <string>
#include <vector>

using namespace restfulEz;

static std::unique_ptr<std::vector<char>> make_raw_response(const std::string& str, bool append_null = true) {
    auto vec = std::make_unique<std::vector<char>>(str.begin(), str.end());
    if (append_null) {
        vec->push_back('\0');
    }
    return vec;
}

int main() {
    std::cout << "[Test] Running End-to-End Linked Requests -> Database Insertion Integration Tests..." << std::endl;

    std::string test_ws = "./test_integration_ws";
    if (std::filesystem::exists(test_ws)) {
        std::filesystem::remove_all(test_ws);
    }

    FileManager& fm = FileManager::GetInstance();
    fm.SetApiKey("RGAPI-11112222-3333-4444-5555-666677778888");
    fm.SetWorkspace(test_ws);

    DatabaseManager& db = DatabaseManager::GetInstance();
    bool db_init = db.Init(fm.GetWorkspacePaths().db_file.string());
    assert(db_init);
    assert(db.IsInitialized());

    // =========================================================================
    // Test 1: 4-Stage Linked Pipeline (Account -> Match List -> Details -> Timeline)
    // =========================================================================
    {
        std::vector<std::string> dispatched_calls;

        // Build 4-Stage Pipeline
        // Stage 1: Account-v1 By Riot ID (game=0, endpoint=0, method=1)
        auto s1_req = std::make_unique<LinkedRequest>();
        s1_req->_game = 0;
        s1_req->_endpoint = 0;
        s1_req->_endpoint_method = 1;
        s1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("Faker"), PARAM_CONT("KR1")};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));

        // Stage 2: Match-v5 By PUUID (game=1, endpoint=6, method=2)
        auto s2_req = std::make_unique<LinkedRequest>();
        s2_req->_game = 1;
        s2_req->_endpoint = 6;
        s2_req->_endpoint_method = 2;
        s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};

        request_link link_1_to_2;
        link_1_to_2.parent = stage1;
        link_1_to_2.param_indices = {1};
        json_access_info puuid_access;
        puuid_access.keys = {KEY_CONT("puuid")};
        link_1_to_2.link_info.push_back(puuid_access);
        s2_req->dependencies.push_back(link_1_to_2);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        // Stage 3: Match-v5 By Match ID (game=1, endpoint=6, method=0, Iterative)
        auto s3_req = std::make_unique<IterativeRequest>();
        s3_req->_game = 1;
        s3_req->_endpoint = 6;
        s3_req->_endpoint_method = 0;
        s3_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s3_req->param_indices = {1};

        iter_request_link link_2_to_3;
        link_2_to_3.parent = stage2;
        link_2_to_3.iter_link_info.push_back(iter_access_info());
        s3_req->iter_dependencies.push_back(link_2_to_3);

        auto stage3 = std::make_shared<RequestNode>(std::move(s3_req));
        stage2->_node->unsent_request->children.push_back(stage3);

        // Stage 4: Match-v5 Timeline (game=1, endpoint=6, method=1, Iterative chained from Stage 3 metadata.matchId)
        auto s4_req = std::make_unique<IterativeRequest>();
        s4_req->_game = 1;
        s4_req->_endpoint = 6;
        s4_req->_endpoint_method = 1;
        s4_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};

        request_link link_3_to_4;
        link_3_to_4.parent = stage3;
        link_3_to_4.param_indices = {1};
        json_access_info mid_access;
        mid_access.keys = {KEY_CONT("metadata"), KEY_CONT("matchId")};
        link_3_to_4.link_info.push_back(mid_access);
        s4_req->dependencies.push_back(link_3_to_4);

        auto stage4 = std::make_shared<RequestNode>(std::move(s4_req));
        stage3->_node->unsent_request->children.push_back(stage4);

        auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_output_directory(test_ws);
            sender.set_custom_handler([&](const request& req) -> std::unique_ptr<std::vector<char>> {
                // Stage 1: Account-v1
                if (req._game == 0 && req._endpoint == 0 && req._endpoint_method == 1) {
                    dispatched_calls.push_back("Account:" + std::string(req.params[1].param) + "#" + std::string(req.params[2].param));
                    return make_raw_response(R"({"puuid":"PUUID_DEV1_FAKER","gameName":"Faker","tagLine":"KR1"})");
                }
                // Stage 2: Match-v5 List By PUUID
                if (req._game == 1 && req._endpoint == 6 && req._endpoint_method == 2) {
                    dispatched_calls.push_back("MatchList:" + std::string(req.params[1].param));
                    return make_raw_response(R"(["KR_7001","KR_7002"])");
                }
                // Stage 3: Match-v5 Details By Match ID
                if (req._game == 1 && req._endpoint == 6 && req._endpoint_method == 0) {
                    std::string mid = req.params[1].param;
                    dispatched_calls.push_back("MatchDetail:" + mid);
                    if (mid == "KR_7001") {
                        return make_raw_response(R"({
                            "metadata": {"matchId": "KR_7001"},
                            "info": {
                                "gameCreation": 1700000001000,
                                "gameDuration": 1800,
                                "gameVersion": "14.15.1",
                                "queueId": 420,
                                "participants": [
                                    {"participantId": 1, "puuid": "PUUID_OTHER_1", "riotIdGameName": "Zeus", "riotIdTagline": "KR1", "championName": "Aatrox", "championId": 266, "teamId": 100, "kills": 5, "deaths": 2, "assists": 7, "win": true},
                                    {"participantId": 2, "puuid": "PUUID_DEV1_FAKER", "riotIdGameName": "Faker", "riotIdTagline": "KR1", "championName": "Azir", "championId": 268, "teamId": 100, "kills": 9, "deaths": 1, "assists": 11, "win": true}
                                ]
                            }
                        })");
                    } else if (mid == "KR_7002") {
                        return make_raw_response(R"({
                            "metadata": {"matchId": "KR_7002"},
                            "info": {
                                "gameCreation": 1700005000000,
                                "gameDuration": 2100,
                                "gameVersion": "14.15.1",
                                "queueId": 420,
                                "participants": [
                                    {"participantId": 1, "puuid": "PUUID_DEV1_FAKER", "riotIdGameName": "Faker", "riotIdTagline": "KR1", "championName": "Orianna", "championId": 61, "teamId": 200, "kills": 7, "deaths": 3, "assists": 9, "win": false}
                                ]
                            }
                        })");
                    }
                }
                // Stage 4: Match-v5 Timeline
                if (req._game == 1 && req._endpoint == 6 && req._endpoint_method == 1) {
                    std::string mid = req.params[1].param;
                    dispatched_calls.push_back("Timeline:" + mid);
                    return make_raw_response(
                        std::string(R"({"metadata":{"matchId":")") + mid + R"("},"info":{"frameInterval":60000}})"
                    );
                }
                return nullptr;
            });

            sender.add_batch_request(batch);
            // RequestSender destructor drains the queue and joins worker_thread
        }

        // Verify execution order and parameter propagation across all 4 linked stages
        assert(dispatched_calls.size() == 6);
        assert(dispatched_calls[0] == "Account:Faker#KR1");
        assert(dispatched_calls[1] == "MatchList:PUUID_DEV1_FAKER");
        assert(dispatched_calls[2] == "MatchDetail:KR_7001");
        assert(dispatched_calls[3] == "MatchDetail:KR_7002");
        assert(dispatched_calls[4] == "Timeline:KR_7001");
        assert(dispatched_calls[5] == "Timeline:KR_7002");

        // Verify raw JSON files were written to disk
        assert(std::filesystem::exists(fm.GetWorkspacePaths().raw_accounts / "Faker_KR1.json"));
        assert(std::filesystem::exists(fm.GetWorkspacePaths().raw_matches / "KR_7001.json"));
        assert(std::filesystem::exists(fm.GetWorkspacePaths().raw_matches / "KR_7002.json"));
        assert(std::filesystem::exists(fm.GetWorkspacePaths().raw_timelines / "KR_7001_timeline.json"));
        assert(std::filesystem::exists(fm.GetWorkspacePaths().raw_timelines / "KR_7002_timeline.json"));

        // Verify DuckDB database insertions
        assert(db.GetPlayerCount() == 1);
        assert(db.GetMatchCount() == 2);
        assert(db.HasMatch("KR_7001"));
        assert(db.HasMatch("KR_7002"));
        assert(db.GetTimelineCount() == 2);
        assert(db.HasTimeline("KR_7001"));
        assert(db.HasTimeline("KR_7002"));

        // Verify participant rows and automatic canonical player_id linking from Stage 1 Account
        assert(db.GetMatchParticipantCount("KR_7001") == 2);
        assert(db.GetMatchParticipantCount("KR_7002") == 1);

        std::string faker_pid = db.ResolveOrCreatePlayer(fm.GetDevKeyPrefix(), "PUUID_DEV1_FAKER", "Faker", "KR1");
        assert(!faker_pid.empty());
        assert(db.GetPlayerCount() == 1); // Did not create a duplicate player
        assert(db.GetParticipantPlayerId("KR_7001", 2) == faker_pid);
        assert(db.GetParticipantPlayerId("KR_7002", 1) == faker_pid);

        std::cout << "  [PASS] 4-Stage Linked Request Pipeline -> File & DuckDB Ingestion" << std::endl;
    }

    // =========================================================================
    // Test 2: Partial Error Resilience Mid-Pipeline (404 status in iterative batch)
    // =========================================================================
    {
        auto s1_req = std::make_unique<LinkedRequest>();
        s1_req->_game = 1;
        s1_req->_endpoint = 6;
        s1_req->_endpoint_method = 2;
        s1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_DEV1_FAKER")};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));

        auto s2_req = std::make_unique<IterativeRequest>();
        s2_req->_game = 1;
        s2_req->_endpoint = 6;
        s2_req->_endpoint_method = 0;
        s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s2_req->param_indices = {1};

        iter_request_link link;
        link.parent = stage1;
        link.iter_link_info.push_back(iter_access_info());
        s2_req->iter_dependencies.push_back(link);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_output_directory(test_ws);
            sender.set_custom_handler([&](const request& req) -> std::unique_ptr<std::vector<char>> {
                if (req._endpoint_method == 2) {
                    return make_raw_response(R"(["KR_7003","KR_7004_ERR","KR_7005"])");
                }
                if (req._endpoint_method == 0) {
                    std::string mid = req.params[1].param;
                    if (mid == "KR_7004_ERR") {
                        return make_raw_response(R"({"status":{"message":"Data not found","status_code":404}})");
                    }
                    return make_raw_response(
                        std::string(R"({"metadata":{"matchId":")") + mid + R"("},"info":{"gameCreation":1700010000000,"gameDuration":1900,"gameVersion":"14.15.1","queueId":420,"participants":[]}})"
                    );
                }
                return nullptr;
            });

            sender.add_batch_request(batch);
        }

        // Valid matches KR_7003 and KR_7005 must be ingested; KR_7004_ERR must be rejected
        assert(db.HasMatch("KR_7003"));
        assert(!db.HasMatch("KR_7004_ERR"));
        assert(db.HasMatch("KR_7005"));
        assert(db.GetMatchCount() == 4); // KR_7001, KR_7002, KR_7003, KR_7005
        assert(!std::filesystem::exists(fm.GetWorkspacePaths().raw_matches / "KR_7004_ERR.json"));

        std::cout << "  [PASS] Error response resilience mid-pipeline (404 skipped, remaining matches saved)" << std::endl;
    }

    // =========================================================================
    // Test 3: Multi-Developer Key & Alias Continuity Across Linked Pipelines
    // =========================================================================
    {
        std::string original_pid = db.ResolveOrCreatePlayer(fm.GetDevKeyPrefix(), "PUUID_DEV1_FAKER", "Faker", "KR1");

        // Switch to a second developer API key
        fm.SetApiKey("RGAPI-99998888-7777-6666-5555-444433332222");

        // Stage 1: Account-v1 By Riot ID ("Faker#KR1" queried under Dev Key 2 returns a different encrypted PUUID)
        auto s1_req = std::make_unique<LinkedRequest>();
        s1_req->_game = 0;
        s1_req->_endpoint = 0;
        s1_req->_endpoint_method = 1;
        s1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("Faker"), PARAM_CONT("KR1")};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));

        // Stage 2: Match-v5 By Match ID using PUUID from Dev Key 2
        auto s2_req = std::make_unique<LinkedRequest>();
        s2_req->_game = 1;
        s2_req->_endpoint = 6;
        s2_req->_endpoint_method = 0;
        s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("KR_7006")};
        request_link dummy_dep;
        dummy_dep.parent = stage1;
        s2_req->dependencies.push_back(dummy_dep);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_output_directory(test_ws);
            sender.set_custom_handler([&](const request& req) -> std::unique_ptr<std::vector<char>> {
                if (req._game == 0) {
                    return make_raw_response(R"({"puuid":"PUUID_DEV2_FAKER","gameName":"Faker","tagLine":"KR1"})");
                }
                if (req._game == 1 && req._endpoint == 6 && req._endpoint_method == 0) {
                    return make_raw_response(R"({
                        "metadata": {"matchId": "KR_7006"},
                        "info": {
                            "gameCreation": 1700020000000,
                            "gameDuration": 1750,
                            "gameVersion": "14.15.1",
                            "queueId": 420,
                            "participants": [
                                {"participantId": 1, "puuid": "PUUID_DEV2_FAKER", "riotIdGameName": "Faker", "riotIdTagline": "KR1", "championName": "LeBlanc", "championId": 7, "teamId": 100, "kills": 12, "deaths": 0, "assists": 6, "win": true}
                            ]
                        }
                    })");
                }
                return nullptr;
            });

            sender.add_batch_request(batch);
        }

        // Player count must remain 1 (Dev Key 2's PUUID mapped to original_pid via Faker#KR1 alias)
        assert(db.GetPlayerCount() == 1);
        assert(db.HasMatch("KR_7006"));
        assert(db.GetParticipantPlayerId("KR_7006", 1) == original_pid);

        std::cout << "  [PASS] Multi-Developer Key PUUID Resolution across Linked Pipelines" << std::endl;
    }

    // =========================================================================
    // Test 4: ValuePool DB-Aware Pre-Filtering (SkipExistingMatchesInDB & SkipExistingTimelinesInDB)
    // =========================================================================
    {
        std::vector<std::string> fetched_matches;
        std::vector<std::string> fetched_timelines;

        // Stage 1: Match-v5 List returns ["KR_7001", "KR_7002", "KR_7007", "KR_7001"]
        // KR_7001 and KR_7002 already exist in DuckDB from Test 1; KR_7007 is new; second KR_7001 is duplicate
        auto s1_req = std::make_unique<LinkedRequest>();
        s1_req->_game = 1; s1_req->_endpoint = 6; s1_req->_endpoint_method = 2;
        s1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_DEV2_FAKER")};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));

        // Stage 2: Match-v5 Details with SkipExistingMatchesInDB() filter on param_pools[1]
        auto s2_req = std::make_unique<LinkedRequest>();
        s2_req->_game = 1; s2_req->_endpoint = 6; s2_req->_endpoint_method = 0;
        s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s2_req->get_or_create_pool(1)->set_deduplicate(true);
        s2_req->get_or_create_pool(1)->set_filter(ValuePool::SkipExistingMatchesInDB());

        iter_request_link link_1_to_2;
        link_1_to_2.parent = stage1;
        link_1_to_2.param_indices = {1};
        link_1_to_2.iter_link_info.push_back(iter_access_info());
        s2_req->iter_dependencies.push_back(link_1_to_2);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        // Stage 3: Match-v5 Timeline linked directly to Stage 1 list with SkipExistingTimelinesInDB()
        auto s3_req = std::make_unique<LinkedRequest>();
        s3_req->_game = 1; s3_req->_endpoint = 6; s3_req->_endpoint_method = 1;
        s3_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s3_req->get_or_create_pool(1)->set_deduplicate(true);
        s3_req->get_or_create_pool(1)->set_filter(ValuePool::SkipExistingTimelinesInDB());

        iter_request_link link_1_to_3;
        link_1_to_3.parent = stage1;
        link_1_to_3.param_indices = {1};
        link_1_to_3.iter_link_info.push_back(iter_access_info());
        s3_req->iter_dependencies.push_back(link_1_to_3);

        auto stage3 = std::make_shared<RequestNode>(std::move(s3_req));
        stage1->_node->unsent_request->children.push_back(stage3);

        auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_output_directory(test_ws);
            sender.set_custom_handler([&](const request& req) -> std::unique_ptr<std::vector<char>> {
                if (req._endpoint_method == 2) {
                    return make_raw_response(R"(["KR_7001","KR_7002","KR_7007","KR_7001"])");
                }
                if (req._endpoint_method == 0) {
                    std::string mid = req.params[1].param;
                    fetched_matches.push_back(mid);
                    return make_raw_response(
                        std::string(R"({"metadata":{"matchId":")") + mid + R"("},"info":{"gameCreation":1700030000000,"gameDuration":1650,"gameVersion":"14.15.1","queueId":420,"participants":[]}})"
                    );
                }
                if (req._endpoint_method == 1) {
                    std::string mid = req.params[1].param;
                    fetched_timelines.push_back(mid);
                    return make_raw_response(
                        std::string(R"({"metadata":{"matchId":")") + mid + R"("},"info":{"frameInterval":60000}})"
                    );
                }
                return nullptr;
            });

            sender.add_batch_request(batch);
        }

        // Only KR_7007 should have been requested for both Match Detail and Timeline
        assert(fetched_matches.size() == 1);
        assert(fetched_matches[0] == "KR_7007");
        assert(fetched_timelines.size() == 1);
        assert(fetched_timelines[0] == "KR_7007");
        assert(db.HasMatch("KR_7007"));
        assert(db.HasTimeline("KR_7007"));

        std::cout << "  [PASS] ValuePool SkipExistingMatchesInDB & SkipExistingTimelinesInDB pre-filtering" << std::endl;
    }

    // =========================================================================
    // Test 5: PoolBindMode::ZIP Participant Riot ID Extraction -> Account-v1 -> DuckDB
    // =========================================================================
    {
        std::vector<std::string> queried_riot_ids;

        // Stage 1: Fetch Match KR_7001 (contains Zeus#KR1 and Faker#KR1)
        auto s1_req = std::make_unique<LinkedRequest>();
        s1_req->_game = 1; s1_req->_endpoint = 6; s1_req->_endpoint_method = 0;
        s1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("KR_7001")};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));

        // Stage 2: Account-v1 By Riot ID in ZIP mode (pairs riotIdGameName and riotIdTagline per participant)
        auto s2_req = std::make_unique<LinkedRequest>();
        s2_req->_game = 0; s2_req->_endpoint = 0; s2_req->_endpoint_method = 1;
        s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT(""), PARAM_CONT("")};
        s2_req->bind_mode = PoolBindMode::ZIP;

        iter_request_link zip_link;
        zip_link.parent = stage1;
        zip_link.param_indices = {1, 2};

        iter_access_info gn_info;
        gn_info.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        gn_info.access_after_iter.keys = {KEY_CONT("riotIdGameName")};

        iter_access_info tl_info;
        tl_info.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        tl_info.access_after_iter.keys = {KEY_CONT("riotIdTagline")};

        zip_link.iter_link_info = {gn_info, tl_info};
        s2_req->iter_dependencies.push_back(zip_link);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_output_directory(test_ws);
            sender.set_custom_handler([&](const request& req) -> std::unique_ptr<std::vector<char>> {
                if (req._game == 1 && req._endpoint == 6 && req._endpoint_method == 0) {
                    return make_raw_response(R"({
                        "metadata": {"matchId": "KR_7001"},
                        "info": {
                            "gameCreation": 1700000001000,
                            "gameDuration": 1800,
                            "gameVersion": "14.15.1",
                            "queueId": 420,
                            "participants": [
                                {"participantId": 1, "puuid": "PUUID_DEV2_ZEUS", "riotIdGameName": "Zeus", "riotIdTagline": "KR1", "championName": "Aatrox", "championId": 266, "teamId": 100, "kills": 5, "deaths": 2, "assists": 7, "win": true},
                                {"participantId": 2, "puuid": "PUUID_DEV2_FAKER", "riotIdGameName": "Faker", "riotIdTagline": "KR1", "championName": "Azir", "championId": 268, "teamId": 100, "kills": 9, "deaths": 1, "assists": 11, "win": true}
                            ]
                        }
                    })");
                }
                if (req._game == 0 && req._endpoint == 0 && req._endpoint_method == 1) {
                    std::string gn = req.params[1].param;
                    std::string tl = req.params[2].param;
                    queried_riot_ids.push_back(gn + "#" + tl);
                    if (gn == "Zeus") {
                        return make_raw_response(R"({"puuid":"PUUID_DEV2_ZEUS","gameName":"Zeus","tagLine":"KR1"})");
                    }
                    return make_raw_response(R"({"puuid":"PUUID_DEV2_FAKER","gameName":"Faker","tagLine":"KR1"})");
                }
                return nullptr;
            });

            sender.add_batch_request(batch);
        }

        assert(queried_riot_ids.size() == 2);
        assert(queried_riot_ids[0] == "Zeus#KR1");
        assert(queried_riot_ids[1] == "Faker#KR1");
        // Zeus#KR1 is added as Player 2; Faker#KR1 is deduplicated with existing Player 1
        assert(db.GetPlayerCount() == 2);

        std::cout << "  [PASS] PoolBindMode::ZIP participant (gameName, tagLine) -> Account-v1 -> DuckDB" << std::endl;
    }

    // =========================================================================
    // Test 6: Incremental Challenger Database Growth with SkipExistingPlayersInDB
    // =========================================================================
    {
        std::vector<std::string> queried_puuids_run1;
        std::vector<std::string> queried_puuids_run2;

        auto run_challenger_batch = [&](std::vector<std::string>& queried_out) {
            auto s1_req = std::make_unique<LinkedRequest>();
            s1_req->_game = 1; s1_req->_endpoint = 3; s1_req->_endpoint_method = 0; // League-v4 Challenger
            s1_req->params = {PARAM_CONT("KR"), PARAM_CONT("RANKED_SOLO_5x5")};
            auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));

            auto s2_req = std::make_unique<LinkedRequest>();
            s2_req->_game = 0; s2_req->_endpoint = 0; s2_req->_endpoint_method = 0; // Account-v1 By PUUID
            s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
            s2_req->get_or_create_pool(1)->set_deduplicate(true);
            s2_req->get_or_create_pool(1)->set_filter(ValuePool::SkipExistingPlayersInDB());

            iter_request_link link_1_to_2;
            link_1_to_2.parent = stage1;
            link_1_to_2.param_indices = {1};
            iter_access_info chal_acc;
            chal_acc.keys = {KEY_CONT("entries")};
            chal_acc.access_after_iter.keys = {KEY_CONT("puuid")};
            chal_acc.iter_limit = 2; // Fetch 2 new Challenger players per run
            link_1_to_2.iter_link_info.push_back(chal_acc);
            s2_req->iter_dependencies.push_back(link_1_to_2);

            auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
            stage1->_node->unsent_request->children.push_back(stage2);

            auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});
            {
                RequestSender sender;
                sender.set_output_directory(test_ws);
                sender.set_custom_handler([&](const request& req) -> std::unique_ptr<std::vector<char>> {
                    if (req._game == 1 && req._endpoint == 3 && req._endpoint_method == 0) {
                        return make_raw_response(R"({
                            "tier": "CHALLENGER",
                            "entries": [
                                {"puuid": "PUUID_CHAL_1"},
                                {"puuid": "PUUID_CHAL_2"},
                                {"puuid": "PUUID_CHAL_3"},
                                {"puuid": "PUUID_CHAL_4"}
                            ]
                        })");
                    }
                    if (req._game == 0 && req._endpoint == 0 && req._endpoint_method == 0) {
                        std::string p = req.params[1].param;
                        queried_out.push_back(p);
                        return make_raw_response(
                            std::string(R"({"puuid":")") + p + R"(","gameName":"Name_)" + p + R"(","tagLine":"KR1"})"
                        );
                    }
                    return nullptr;
                });
                sender.add_batch_request(batch);
            }
        };

        int64_t players_before = db.GetPlayerCount();
        // Run 1: Should ingest PUUID_CHAL_1 and PUUID_CHAL_2
        run_challenger_batch(queried_puuids_run1);
        assert(queried_puuids_run1.size() == 2);
        assert(queried_puuids_run1[0] == "PUUID_CHAL_1");
        assert(queried_puuids_run1[1] == "PUUID_CHAL_2");
        assert(db.GetPlayerCount() == players_before + 2);

        // Run 2: SkipExistingPlayersInDB skips PUUID_CHAL_1 & PUUID_CHAL_2 and ingests PUUID_CHAL_3 & PUUID_CHAL_4
        run_challenger_batch(queried_puuids_run2);
        assert(queried_puuids_run2.size() == 2);
        assert(queried_puuids_run2[0] == "PUUID_CHAL_3");
        assert(queried_puuids_run2[1] == "PUUID_CHAL_4");
        assert(db.GetPlayerCount() == players_before + 4);

        std::cout << "  [PASS] Incremental Challenger player database growth via SkipExistingPlayersInDB" << std::endl;
    }

    db.Close();
    std::filesystem::remove_all(test_ws);

    std::cout << "[Test] All Linked Request & Database Insertion Integration Tests passed successfully!" << std::endl;
    return 0;
}
