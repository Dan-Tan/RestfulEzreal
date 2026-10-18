#include "BatchRequests.h"
#include "RequestQueue.h"
#include "FileManager.h"
#include "db/DatabaseManager.h"
#include "client/client.h"
#include "simdjson.h"
#include <iostream>
#include <filesystem>
#include <cassert>
#include <string>
#include <vector>

using namespace restfulEz;

int main() {
    std::cout << "[LiveTest] Starting Live Riot API Linked Request & DuckDB Integration Tests..." << std::endl;

    FileManager& fm = FileManager::GetInstance();
    if (!fm.LoadEnvKey(".env") || fm.GetApiKey().empty()) {
        std::cerr << "[LiveTest] ERROR: Could not load RIOT_KEY from .env or environment." << std::endl;
        return 1;
    }
    // Never print the key or prefix; only verify non-empty length
    std::cout << "  [INFO] Loaded RIOT_KEY from .env (key length: " << fm.GetApiKey().length() << " chars)" << std::endl;

    std::string test_ws = "./test_live_ws";
    if (std::filesystem::exists(test_ws)) {
        std::filesystem::remove_all(test_ws);
    }
    fm.SetWorkspace(test_ws);

    DatabaseManager& db = DatabaseManager::GetInstance();
    bool db_init = db.Init(fm.GetWorkspacePaths().db_file.string());
    assert(db_init);
    assert(db.IsInitialized());

    auto riot_client = std::make_shared<riotcpp::client::RiotApiClient>(
        fm.GetApiKey(),
        (fm.GetWorkspacePaths().root_dir / "logs" / "live_test.log").string(),
        spdlog::level::info,
        false,
        true
    );

    std::string discovered_puuid, discovered_name, discovered_tag;
    std::vector<std::string> ingested_match_ids;

    // =========================================================================
    // Live Test 1: 5-Stage Live Pipeline
    // Stage 1: League-v4 Challenger (KR, RANKED_SOLO_5x5)
    // Stage 2: Account-v1 By PUUID (ASIA, entries[0].puuid, iter_limit=1) -> Ingests Player into DuckDB
    // Stage 3: Match-v5 By PUUID (ASIA, puuid from Stage 2, count=2)
    // Stage 4: Match-v5 By Match ID (ASIA, matchIds from Stage 3, iter_limit=2) -> Ingests 2 Matches + 20 Participants
    // Stage 5: Match-v5 Timeline (ASIA, metadata.matchId from Stage 4) -> Ingests 2 Timelines
    // =========================================================================
    {
        auto s1_req = std::make_unique<LinkedRequest>();
        s1_req->_game = 1;
        s1_req->_endpoint = 3;
        s1_req->_endpoint_method = 0;
        s1_req->params = {PARAM_CONT("KR"), PARAM_CONT("RANKED_SOLO_5x5")};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));

        auto s2_req = std::make_unique<IterativeRequest>();
        s2_req->_game = 0;
        s2_req->_endpoint = 0;
        s2_req->_endpoint_method = 0;
        s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s2_req->param_indices = {1};

        iter_request_link link_1_to_2;
        link_1_to_2.parent = stage1;
        link_1_to_2.param_indices = {1};
        iter_access_info challenger_puuid_access;
        challenger_puuid_access.keys = {KEY_CONT("entries")};
        challenger_puuid_access.access_after_iter.keys = {KEY_CONT("puuid")};
        challenger_puuid_access.iter_limit = 1;
        link_1_to_2.iter_link_info.push_back(challenger_puuid_access);
        s2_req->iter_dependencies.push_back(link_1_to_2);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        auto s3_req = std::make_unique<IterativeRequest>();
        s3_req->_game = 1;
        s3_req->_endpoint = 6;
        s3_req->_endpoint_method = 2;
        s3_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s3_req->optional_names = {P_NAME("count")};
        s3_req->optional_inputs = {PARAM_CONT("2")};

        request_link link_2_to_3;
        link_2_to_3.parent = stage2;
        link_2_to_3.param_indices = {1};
        json_access_info acc_puuid_access;
        acc_puuid_access.keys = {KEY_CONT("puuid")};
        link_2_to_3.link_info.push_back(acc_puuid_access);
        s3_req->dependencies.push_back(link_2_to_3);

        auto stage3 = std::make_shared<RequestNode>(std::move(s3_req));
        stage2->_node->unsent_request->children.push_back(stage3);

        auto s4_req = std::make_unique<IterativeRequest>();
        s4_req->_game = 1;
        s4_req->_endpoint = 6;
        s4_req->_endpoint_method = 0;
        s4_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s4_req->param_indices = {1};

        iter_request_link link_3_to_4;
        link_3_to_4.parent = stage3;
        link_3_to_4.param_indices = {1};
        iter_access_info match_list_access;
        match_list_access.iter_limit = 2;
        link_3_to_4.iter_link_info.push_back(match_list_access);
        s4_req->iter_dependencies.push_back(link_3_to_4);

        auto stage4 = std::make_shared<RequestNode>(std::move(s4_req));
        stage3->_node->unsent_request->children.push_back(stage4);

        auto s5_req = std::make_unique<IterativeRequest>();
        s5_req->_game = 1;
        s5_req->_endpoint = 6;
        s5_req->_endpoint_method = 1;
        s5_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};

        request_link link_4_to_5;
        link_4_to_5.parent = stage4;
        link_4_to_5.param_indices = {1};
        json_access_info mid_access;
        mid_access.keys = {KEY_CONT("metadata"), KEY_CONT("matchId")};
        link_4_to_5.link_info.push_back(mid_access);
        s5_req->dependencies.push_back(link_4_to_5);

        auto stage5 = std::make_shared<RequestNode>(std::move(s5_req));
        stage4->_node->unsent_request->children.push_back(stage5);

        auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_client(riot_client);
            sender.set_output_directory(test_ws);
            sender.add_batch_request(batch);
        }

        assert(db.GetPlayerCount() == 1);
        assert(db.GetMatchCount() == 2);
        assert(db.GetTimelineCount() == 2);

        for (const auto& entry : std::filesystem::directory_iterator(fm.GetWorkspacePaths().raw_matches)) {
            if (entry.path().extension() == ".json") {
                std::string mid = entry.path().stem().string();
                ingested_match_ids.push_back(mid);
                assert(db.HasMatch(mid));
                assert(db.HasTimeline(mid));
                assert(db.GetMatchParticipantCount(mid) == 10);
            }
        }
        assert(ingested_match_ids.size() == 2);

        for (const auto& entry : std::filesystem::directory_iterator(fm.GetWorkspacePaths().raw_accounts)) {
            if (entry.path().extension() == ".json") {
                simdjson::dom::parser parser;
                simdjson::dom::element doc;
                auto err = parser.load(entry.path().string()).get(doc);
                assert(err == simdjson::SUCCESS);
                std::string_view sv;
                if (doc["puuid"].get(sv) == simdjson::SUCCESS) discovered_puuid = std::string(sv);
                if (doc["gameName"].get(sv) == simdjson::SUCCESS) discovered_name = std::string(sv);
                if (doc["tagLine"].get(sv) == simdjson::SUCCESS) discovered_tag = std::string(sv);
            }
        }
        assert(!discovered_puuid.empty());
        assert(!discovered_name.empty());
        assert(!discovered_tag.empty());

        std::string canonical_pid = db.ResolveOrCreatePlayer(fm.GetDevKeyPrefix(), discovered_puuid, discovered_name, discovered_tag);
        assert(!canonical_pid.empty());
        assert(db.GetPlayerCount() == 1);

        for (const std::string& mid : ingested_match_ids) {
            bool found_linked_participant = false;
            for (int pid_idx = 1; pid_idx <= 10; ++pid_idx) {
                if (db.GetParticipantPlayerId(mid, pid_idx) == canonical_pid) {
                    found_linked_participant = true;
                    break;
                }
            }
            assert(found_linked_participant);
        }

        std::cout << "  [PASS] Live Test 1: 5-Stage Pipeline (Challenger -> Account -> Match List -> 2 Match Details -> 2 Timelines)" << std::endl;
    }

    // =========================================================================
    // Live Test 2: Multi-Parent Diamond DAG & Multi-Parameter Odometer
    // Stage 1: Account-v1 (by_riot_id) -> provides puuid
    // Stage 2: Champion-Mastery-v4 (Top By PUUID, count=2) -> provides [].championId (integers)
    // Stage 3: Champion-Mastery-v4 (By PUUID By Champion) -> depends on Stage 1 (puuid) AND Stage 2 (championId)
    // =========================================================================
    {
        auto s1_acc = std::make_unique<LinkedRequest>();
        s1_acc->_game = 0;
        s1_acc->_endpoint = 0;
        s1_acc->_endpoint_method = 1; // by_riot_id
        s1_acc->params = {PARAM_CONT("ASIA"), PARAM_CONT(discovered_name.c_str()), PARAM_CONT(discovered_tag.c_str())};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_acc));

        auto s2_top = std::make_unique<LinkedRequest>();
        s2_top->_game = 1;
        s2_top->_endpoint = 0;
        s2_top->_endpoint_method = 2; // top by_puuid
        s2_top->params = {PARAM_CONT("KR"), PARAM_CONT("")};
        s2_top->optional_names = {P_NAME("count")};
        s2_top->optional_inputs = {PARAM_CONT("2")};

        request_link link_1_to_2;
        link_1_to_2.parent = stage1;
        link_1_to_2.param_indices = {1};
        json_access_info puuid_info;
        puuid_info.keys = {KEY_CONT("puuid")};
        link_1_to_2.link_info.push_back(puuid_info);
        s2_top->dependencies.push_back(link_1_to_2);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_top));
        stage1->_node->unsent_request->children.push_back(stage2);

        // Stage 3: Champion Mastery By PUUID By Champion (game=1, endpoint=0, method=1)
        // Params: [0]=Routing("KR"), [1]=PUUID(from Stage 1), [2]=ChampionID(iteratively from Stage 2)
        auto s3_by_champ = std::make_unique<IterativeRequest>();
        s3_by_champ->_game = 1;
        s3_by_champ->_endpoint = 0;
        s3_by_champ->_endpoint_method = 1;
        s3_by_champ->params = {PARAM_CONT("KR"), PARAM_CONT(""), PARAM_CONT("")};

        request_link link_1_to_3;
        link_1_to_3.parent = stage1;
        link_1_to_3.param_indices = {1};
        link_1_to_3.link_info.push_back(puuid_info);
        s3_by_champ->dependencies.push_back(link_1_to_3);

        iter_request_link link_2_to_3;
        link_2_to_3.parent = stage2;
        link_2_to_3.param_indices = {2};
        iter_access_info champ_id_info;
        champ_id_info.access_after_iter.keys = {KEY_CONT("championId")};
        champ_id_info.iter_limit = 2;
        link_2_to_3.iter_link_info.push_back(champ_id_info);
        s3_by_champ->iter_dependencies.push_back(link_2_to_3);

        auto stage3 = std::make_shared<RequestNode>(std::move(s3_by_champ));
        stage1->_node->unsent_request->children.push_back(stage3);
        stage2->_node->unsent_request->children.push_back(stage3);

        auto batch_diamond = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_client(riot_client);
            sender.set_output_directory(test_ws);
            sender.add_batch_request(batch_diamond);
        }

        // Player count must still be 1 (deduplicated against Stage 2 player record)
        assert(db.GetPlayerCount() == 1);

        // Verify Stage 2 and Stage 3 raw output files exist and contain valid JSON
        std::filesystem::path mastery_top_file = fm.GetRawOutputPath(1, 0, 2, discovered_puuid);
        assert(std::filesystem::exists(mastery_top_file));
        std::filesystem::path mastery_champ_file = fm.GetRawOutputPath(1, 0, 1, discovered_puuid);
        assert(std::filesystem::exists(mastery_champ_file));

        simdjson::dom::parser parser;
        simdjson::dom::element champ_doc;
        assert(parser.load(mastery_champ_file.string()).get(champ_doc) == simdjson::SUCCESS);
        int64_t cid = 0;
        assert(champ_doc["championId"].get(cid) == simdjson::SUCCESS);
        assert(cid > 0);

        std::cout << "  [PASS] Live Test 2: Diamond DAG (Account-v1 + Mastery Top -> Mastery By Champion) & Player Deduplication" << std::endl;
    }

    // =========================================================================
    // Live Test 3: Match Participant Fan-Out -> Account-v1 -> Match Re-Ingestion
    // Extracts first 3 participant PUUIDs from a live match, resolves their accounts
    // via Account-v1 into DuckDB, then re-ingests the match and verifies all 3
    // participants have resolved canonical player_ids in match_participants.
    // =========================================================================
    {
        std::string target_match = ingested_match_ids[0];

        // Stage 1: Match-v5 By Match ID
        auto s1_match = std::make_unique<LinkedRequest>();
        s1_match->_game = 1;
        s1_match->_endpoint = 6;
        s1_match->_endpoint_method = 0;
        s1_match->params = {PARAM_CONT("ASIA"), PARAM_CONT(target_match.c_str())};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_match));

        // Stage 2: Account-v1 By PUUID (iterative over info.participants[].puuid, iter_limit=3)
        auto s2_accs = std::make_unique<IterativeRequest>();
        s2_accs->_game = 0;
        s2_accs->_endpoint = 0;
        s2_accs->_endpoint_method = 0;
        s2_accs->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s2_accs->param_indices = {1};

        iter_request_link link_m_to_a;
        link_m_to_a.parent = stage1;
        link_m_to_a.param_indices = {1};
        iter_access_info part_puuids;
        part_puuids.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        part_puuids.access_after_iter.keys = {KEY_CONT("puuid")};
        part_puuids.iter_limit = 3;
        link_m_to_a.iter_link_info.push_back(part_puuids);
        s2_accs->iter_dependencies.push_back(link_m_to_a);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_accs));
        stage1->_node->unsent_request->children.push_back(stage2);

        // Stage 3: Re-fetch Match-v5 By Match ID after participant accounts are in DuckDB
        auto s3_rematch = std::make_unique<LinkedRequest>();
        s3_rematch->_game = 1;
        s3_rematch->_endpoint = 6;
        s3_rematch->_endpoint_method = 0;
        s3_rematch->params = {PARAM_CONT("ASIA"), PARAM_CONT(target_match.c_str())};
        request_link wait_for_stage2;
        wait_for_stage2.parent = stage2;
        s3_rematch->dependencies.push_back(wait_for_stage2);

        auto stage3 = std::make_shared<RequestNode>(std::move(s3_rematch));
        stage2->_node->unsent_request->children.push_back(stage3);

        auto batch_parts = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_client(riot_client);
            sender.set_output_directory(test_ws);
            sender.add_batch_request(batch_parts);
        }

        // At least 3 distinct canonical players must now exist in DuckDB
        assert(db.GetPlayerCount() >= 3);
        // Match count must still be 2 (idempotent upsert on target_match)
        assert(db.GetMatchCount() == 2);
        assert(db.GetMatchParticipantCount(target_match) == 10);

        // Participants 1, 2, and 3 in target_match must all have non-empty resolved player_ids
        for (int p_idx = 1; p_idx <= 3; ++p_idx) {
            std::string resolved_pid = db.GetParticipantPlayerId(target_match, p_idx);
            assert(!resolved_pid.empty());
        }

        std::cout << "  [PASS] Live Test 3: Match Participant Fan-Out -> Account-v1 -> DuckDB Participant Linking" << std::endl;
    }

    // =========================================================================
    // Live Test 4: ValuePool ZIP Mode & SkipExistingMatchesInDB on Live Riot API
    // Part A: Queries Match-v5 by_puuid (count=3) where 2 matches are already in DuckDB;
    //         ValuePool::SkipExistingMatchesInDB() filters out the 2 existing matches and only fetches the 1 new match.
    // Part B: Uses PoolBindMode::ZIP to pair (riotIdGameName, riotIdTagline) from the new match's first 2 participants
    //         into Account-v1 By Riot ID.
    // =========================================================================
    {
        assert(db.GetMatchCount() == 2);

        // Stage 1: Match-v5 By PUUID (count=3)
        auto s1_list = std::make_unique<LinkedRequest>();
        s1_list->_game = 1; s1_list->_endpoint = 6; s1_list->_endpoint_method = 2;
        s1_list->params = {PARAM_CONT("ASIA"), PARAM_CONT(discovered_puuid.c_str())};
        s1_list->optional_names = {P_NAME("count")};
        s1_list->optional_inputs = {PARAM_CONT("3")};
        auto stage1 = std::make_shared<RequestNode>(std::move(s1_list));

        // Stage 2: Match-v5 By Match ID with SkipExistingMatchesInDB()
        auto s2_detail = std::make_unique<LinkedRequest>();
        s2_detail->_game = 1; s2_detail->_endpoint = 6; s2_detail->_endpoint_method = 0;
        s2_detail->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s2_detail->get_or_create_pool(1)->set_deduplicate(true);
        s2_detail->get_or_create_pool(1)->set_filter(ValuePool::SkipExistingMatchesInDB());

        iter_request_link link_1_to_2;
        link_1_to_2.parent = stage1;
        link_1_to_2.param_indices = {1};
        iter_access_info list_acc;
        list_acc.iter_limit = 3;
        link_1_to_2.iter_link_info.push_back(list_acc);
        s2_detail->iter_dependencies.push_back(link_1_to_2);

        auto stage2 = std::make_shared<RequestNode>(std::move(s2_detail));
        stage1->_node->unsent_request->children.push_back(stage2);

        // Stage 3: Account-v1 By Riot ID in PoolBindMode::ZIP (pairs riotIdGameName + riotIdTagline from Stage 2)
        auto s3_zip_acc = std::make_unique<LinkedRequest>();
        s3_zip_acc->_game = 0; s3_zip_acc->_endpoint = 0; s3_zip_acc->_endpoint_method = 1;
        s3_zip_acc->params = {PARAM_CONT("ASIA"), PARAM_CONT(""), PARAM_CONT("")};
        s3_zip_acc->bind_mode = PoolBindMode::ZIP;

        iter_request_link zip_link;
        zip_link.parent = stage2;
        zip_link.param_indices = {1, 2};

        iter_access_info gn_acc;
        gn_acc.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        gn_acc.access_after_iter.keys = {KEY_CONT("riotIdGameName")};
        gn_acc.iter_limit = 2;

        iter_access_info tl_acc;
        tl_acc.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        tl_acc.access_after_iter.keys = {KEY_CONT("riotIdTagline")};
        tl_acc.iter_limit = 2;

        zip_link.iter_link_info = {gn_acc, tl_acc};
        s3_zip_acc->iter_dependencies.push_back(zip_link);

        auto stage3 = std::make_shared<RequestNode>(std::move(s3_zip_acc));
        stage2->_node->unsent_request->children.push_back(stage3);

        auto batch_vp = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{stage1});

        {
            RequestSender sender;
            sender.set_client(riot_client);
            sender.set_output_directory(test_ws);
            sender.add_batch_request(batch_vp);
        }

        // Stage 2 must have skipped the 2 existing matches and only fetched the 3rd match
        assert(stage2->_node->request_results.size() == 1);
        assert(db.GetMatchCount() == 3);

        // Stage 3 must have executed the 2 paired (gameName, tagLine) Account-v1 queries in ZIP mode
        assert(stage3->sent);
        std::vector<PARAM_CONT> expected_gns = gn_acc.get_params(stage2->_node->request_results);
        std::vector<PARAM_CONT> expected_tls = tl_acc.get_params(stage2->_node->request_results);
        assert(expected_gns.size() == 2 && expected_tls.size() == 2);
        for (std::size_t i = 0; i < 2; ++i) {
            std::string acc_id = std::string(expected_gns[i].param) + "_" + std::string(expected_tls[i].param);
            assert(std::filesystem::exists(fm.GetRawOutputPath(0, 0, 1, acc_id)));
        }

        std::cout << "  [PASS] Live Test 4: ValuePool SkipExistingMatchesInDB & PoolBindMode::ZIP (riotIdGameName + riotIdTagline)" << std::endl;
    }

    db.Close();
    std::filesystem::remove_all(test_ws);

    std::cout << "[LiveTest] All Live Riot API Linked Request & DuckDB Integration Tests passed successfully!" << std::endl;
    return 0;
}
