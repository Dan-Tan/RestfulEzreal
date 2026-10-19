#include "BatchRequests.h"
#include "db/DatabaseManager.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <filesystem>

using namespace restfulEz;

int main() {
    std::cout << "[Test] Running BatchRequests & Pipeline Unit Tests..." << std::endl;

    // 1. Test json_access_info::get_param (scalar string, nested keys, integer conversion, trailing nulls, and edge cases)
    {
        std::string sample_account_json = R"({"puuid":"TEST_PUUID_12345","gameName":"Faker","tagLine":"KR1"})";
        raw_json raw(sample_account_json.begin(), sample_account_json.end());
        // Simulate riot-cpp appending trailing null characters
        raw.push_back('\0');
        raw.push_back('\0');

        json_access_info info;
        info.keys.push_back(KEY_CONT("puuid"));
        PARAM_CONT extracted = info.get_param(raw);
        assert(std::string(extracted.param) == "TEST_PUUID_12345");

        json_access_info name_info;
        name_info.keys.push_back(KEY_CONT("gameName"));
        PARAM_CONT extracted_name = name_info.get_param(raw);
        assert(std::string(extracted_name.param) == "Faker");

        // Nested key + integer extraction
        std::string nested_json = R"({"metadata":{"matchId":"KR_9001"},"info":{"queueId":420,"gameDuration":1845}})";
        raw_json nested_raw(nested_json.begin(), nested_json.end());

        json_access_info match_id_info;
        match_id_info.keys = {KEY_CONT("metadata"), KEY_CONT("matchId")};
        assert(std::string(match_id_info.get_param(nested_raw).param) == "KR_9001");

        json_access_info queue_info;
        queue_info.keys = {KEY_CONT("info"), KEY_CONT("queueId")};
        assert(std::string(queue_info.get_param(nested_raw).param) == "420");

        // Missing key and malformed JSON resilience
        json_access_info missing_info;
        missing_info.keys = {KEY_CONT("info"), KEY_CONT("nonExistentKey")};
        assert(std::string(missing_info.get_param(nested_raw).param).empty());

        raw_json empty_raw;
        assert(std::string(info.get_param(empty_raw).param).empty());

        std::string bad_json = "{not_valid_json";
        raw_json bad_raw(bad_json.begin(), bad_json.end());
        assert(std::string(info.get_param(bad_raw).param).empty());

        std::cout << "  [PASS] json_access_info scalar, nested, integer & edge-case extraction" << std::endl;
    }

    // 2. Test iter_access_info::get_params on root array (Match-v5 by_puuid format)
    {
        std::string sample_match_ids_json = R"(["OC1_1001","OC1_1002","OC1_1003"])";
        raw_json raw(sample_match_ids_json.begin(), sample_match_ids_json.end());

        iter_access_info iter_info;
        std::vector<PARAM_CONT> match_ids = iter_info.get_params(raw);
        assert(match_ids.size() == 3);
        assert(std::string(match_ids[0].param) == "OC1_1001");
        assert(std::string(match_ids[1].param) == "OC1_1002");
        assert(std::string(match_ids[2].param) == "OC1_1003");
        std::cout << "  [PASS] iter_access_info root array extraction" << std::endl;
    }

    // 3. Test iter_access_info with iter_limit
    {
        std::string sample_match_ids_json = R"(["OC1_1001","OC1_1002","OC1_1003","OC1_1004","OC1_1005"])";
        raw_json raw(sample_match_ids_json.begin(), sample_match_ids_json.end());

        iter_access_info iter_info;
        iter_info.iter_limit = 2;
        std::vector<PARAM_CONT> match_ids = iter_info.get_params(raw);
        assert(match_ids.size() == 2);
        assert(std::string(match_ids[0].param) == "OC1_1001");
        assert(std::string(match_ids[1].param) == "OC1_1002");
        std::cout << "  [PASS] iter_access_info with iter_limit" << std::endl;
    }

    // 4. Test iter_access_info nested object array (e.g. Match-v5 participants -> puuid & championId)
    {
        std::string match_participants_json = R"({
            "info": {
                "participants": [
                    {"puuid": "PUUID_P1", "championId": 81},
                    {"puuid": "PUUID_P2", "championId": 103},
                    {"puuid": "PUUID_P3", "championId": 64}
                ]
            }
        })";
        raw_json raw(match_participants_json.begin(), match_participants_json.end());

        iter_access_info part_puuid_info;
        part_puuid_info.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        part_puuid_info.access_after_iter.keys = {KEY_CONT("puuid")};
        part_puuid_info.iter_limit = 2;

        std::vector<PARAM_CONT> puuids = part_puuid_info.get_params(raw);
        assert(puuids.size() == 2);
        assert(std::string(puuids[0].param) == "PUUID_P1");
        assert(std::string(puuids[1].param) == "PUUID_P2");

        iter_access_info part_champ_info;
        part_champ_info.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        part_champ_info.access_after_iter.keys = {KEY_CONT("championId")};

        std::vector<PARAM_CONT> champs = part_champ_info.get_params(raw);
        assert(champs.size() == 3);
        assert(std::string(champs[0].param) == "81");
        assert(std::string(champs[1].param) == "103");
        assert(std::string(champs[2].param) == "64");

        std::cout << "  [PASS] iter_access_info nested object array extraction" << std::endl;
    }

    // 5. Test 3-Stage Pipeline: Account -> Match List (by_puuid) -> Match Details (by_match_id)
    {
        // Stage 1: Account-v1 (non-iterative)
        auto account_req = std::make_unique<LinkedRequest>();
        account_req->_game = 0;
        account_req->_endpoint = 0;
        account_req->_endpoint_method = 1; // by_riot_id
        account_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("Monkeys R Us"), PARAM_CONT("fresn")};
        auto stage1 = std::make_shared<RequestNode>(std::move(account_req));

        // Stage 2: Match-v5 By Puuid (non-iterative, depends on Stage 1 puuid)
        auto match_list_req = std::make_unique<LinkedRequest>();
        match_list_req->_game = 1;
        match_list_req->_endpoint = 6;
        match_list_req->_endpoint_method = 2; // by_puuid
        match_list_req->params = {PARAM_CONT("OC1"), PARAM_CONT("")}; // param[1] will be filled with puuid

        request_link link_acc_to_matchlist;
        link_acc_to_matchlist.parent = stage1;
        link_acc_to_matchlist.param_indices = {1};
        json_access_info puuid_access;
        puuid_access.keys.push_back(KEY_CONT("puuid"));
        link_acc_to_matchlist.link_info.push_back(puuid_access);
        match_list_req->dependencies.push_back(link_acc_to_matchlist);

        auto stage2 = std::make_shared<RequestNode>(std::move(match_list_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        // Stage 3: Match-v5 By Match ID (iterative, depends on Stage 2 match ID list)
        auto match_detail_req = std::make_unique<IterativeRequest>();
        match_detail_req->_game = 1;
        match_detail_req->_endpoint = 6;
        match_detail_req->_endpoint_method = 0; // by_match_id
        match_detail_req->params = {PARAM_CONT("OC1"), PARAM_CONT("")}; // param[1] will be filled iteratively
        match_detail_req->param_indices = {1};

        iter_request_link link_list_to_detail;
        link_list_to_detail.parent = stage2;
        iter_access_info match_ids_access;
        link_list_to_detail.iter_link_info.push_back(match_ids_access);
        match_detail_req->iter_dependencies.push_back(link_list_to_detail);

        auto stage3 = std::make_shared<RequestNode>(std::move(match_detail_req));
        stage2->_node->unsent_request->children.push_back(stage3);

        // Execute Batch
        BatchRequest batch({stage1});

        // Query 1: Stage 1 should be Account-v1
        request q1 = batch.get_next();
        assert(q1._game == 0 && q1._endpoint == 0 && q1._endpoint_method == 1);
        assert(std::string(q1.params[1].param) == "Monkeys R Us");

        // Simulate Stage 1 response
        std::string acc_resp = R"({"puuid":"ACC_PUUID_99999","gameName":"Monkeys R Us","tagLine":"fresn"})";
        auto resp1 = std::make_unique<raw_json>(acc_resp.begin(), acc_resp.end());
        bool more_s1 = batch.insert_result(std::move(resp1));
        assert(!more_s1); // Stage 1 is non-iterative

        // Query 2: Stage 2 should be Match-v5 By Puuid with filled PUUID
        request q2 = batch.get_next();
        assert(q2._game == 1 && q2._endpoint == 6 && q2._endpoint_method == 2);
        assert(std::string(q2.params[1].param) == "ACC_PUUID_99999");

        // Simulate Stage 2 response (array of 3 match IDs)
        std::string matches_resp = R"(["OC1_501","OC1_502","OC1_503"])";
        auto resp2 = std::make_unique<raw_json>(matches_resp.begin(), matches_resp.end());
        bool more_s2 = batch.insert_result(std::move(resp2));
        assert(!more_s2); // Stage 2 is non-iterative

        // Query 3: Stage 3 should iterate through all 3 match IDs
        request q3_1 = batch.get_next();
        assert(q3_1._game == 1 && q3_1._endpoint == 6 && q3_1._endpoint_method == 0);
        assert(std::string(q3_1.params[1].param) == "OC1_501");

        auto resp3_1 = std::make_unique<raw_json>(matches_resp.begin(), matches_resp.end());
        assert(batch.insert_result(std::move(resp3_1)));

        request q3_2 = batch.get_current();
        assert(std::string(q3_2.params[1].param) == "OC1_502");

        auto resp3_2 = std::make_unique<raw_json>(matches_resp.begin(), matches_resp.end());
        assert(batch.insert_result(std::move(resp3_2)));

        request q3_3 = batch.get_current();
        assert(std::string(q3_3.params[1].param) == "OC1_503");

        auto resp3_3 = std::make_unique<raw_json>(matches_resp.begin(), matches_resp.end());
        assert(!batch.insert_result(std::move(resp3_3)));

        request q_done = batch.get_next();
        assert(q_done.same_endpoint(BatchRequest::FINISHED));

        std::cout << "  [PASS] 3-Stage Batch Pipeline Execution & Iteration" << std::endl;
    }

    // 6. Test 4-Stage Chained Iterative Pipeline: Match List -> Match Details (Iterative) -> Match Timeline (Iterative via scalar metadata.matchId)
    {
        for (int num_matches : {1, 2}) {
            auto list_req = std::make_unique<LinkedRequest>();
            list_req->_game = 1;
            list_req->_endpoint = 6;
            list_req->_endpoint_method = 2;
            list_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_CHAIN")};
            auto stage1 = std::make_shared<RequestNode>(std::move(list_req));

            auto detail_req = std::make_unique<IterativeRequest>();
            detail_req->_game = 1;
            detail_req->_endpoint = 6;
            detail_req->_endpoint_method = 0;
            detail_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
            detail_req->param_indices = {1};

            iter_request_link link_list_to_detail;
            link_list_to_detail.parent = stage1;
            link_list_to_detail.iter_link_info.push_back(iter_access_info());
            detail_req->iter_dependencies.push_back(link_list_to_detail);

            auto stage2 = std::make_shared<RequestNode>(std::move(detail_req));
            stage1->_node->unsent_request->children.push_back(stage2);

            // Stage 3: Timeline (IterativeRequest linked via non-iterative request_link on metadata.matchId of Stage 2 results)
            auto timeline_req = std::make_unique<IterativeRequest>();
            timeline_req->_game = 1;
            timeline_req->_endpoint = 6;
            timeline_req->_endpoint_method = 1; // timeline
            timeline_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};

            request_link link_detail_to_timeline;
            link_detail_to_timeline.parent = stage2;
            link_detail_to_timeline.param_indices = {1};
            json_access_info mid_access;
            mid_access.keys = {KEY_CONT("metadata"), KEY_CONT("matchId")};
            link_detail_to_timeline.link_info.push_back(mid_access);
            timeline_req->dependencies.push_back(link_detail_to_timeline);

            auto stage3 = std::make_shared<RequestNode>(std::move(timeline_req));
            stage2->_node->unsent_request->children.push_back(stage3);

            BatchRequest batch({stage1});

            request q_list = batch.get_next();
            assert(q_list._endpoint_method == 2);

            std::string list_json = (num_matches == 1) ? R"(["KR_101"])" : R"(["KR_101","KR_102"])";
            assert(!batch.insert_result(std::make_unique<raw_json>(list_json.begin(), list_json.end())));

            // Stage 2: Match Details
            request q_d1 = batch.get_next();
            assert(q_d1._endpoint_method == 0);
            assert(std::string(q_d1.params[1].param) == "KR_101");
            std::string d1_json = R"({"metadata":{"matchId":"KR_101"},"info":{"gameDuration":1500}})";
            bool more_d = batch.insert_result(std::make_unique<raw_json>(d1_json.begin(), d1_json.end()));

            if (num_matches == 2) {
                assert(more_d);
                request q_d2 = batch.get_current();
                assert(std::string(q_d2.params[1].param) == "KR_102");
                std::string d2_json = R"({"metadata":{"matchId":"KR_102"},"info":{"gameDuration":1600}})";
                assert(!batch.insert_result(std::make_unique<raw_json>(d2_json.begin(), d2_json.end())));
            } else {
                assert(!more_d);
            }

            // Stage 3: Match Timeline (must execute for both num_matches == 1 and num_matches == 2)
            request q_t1 = batch.get_next();
            assert(!q_t1.same_endpoint(BatchRequest::FINISHED));
            assert(q_t1._endpoint_method == 1);
            assert(std::string(q_t1.params[1].param) == "KR_101");
            std::string t_json = R"({"metadata":{"matchId":"KR_101"},"info":{"frameInterval":60000}})";
            bool more_t = batch.insert_result(std::make_unique<raw_json>(t_json.begin(), t_json.end()));

            if (num_matches == 2) {
                assert(more_t);
                request q_t2 = batch.get_current();
                assert(q_t2._endpoint_method == 1);
                assert(std::string(q_t2.params[1].param) == "KR_102");
                assert(!batch.insert_result(std::make_unique<raw_json>(t_json.begin(), t_json.end())));
            } else {
                assert(!more_t);
            }

            assert(batch.get_next().same_endpoint(BatchRequest::FINISHED));
        }
        std::cout << "  [PASS] Chained Iterative -> Iterative (via scalar request_link, M=1 and M=2)" << std::endl;
    }

    // 7. Test Fan-out (Branching DAG) Pipeline: Match List -> [Match Details, Match Timeline]
    {
        auto list_req = std::make_unique<LinkedRequest>();
        list_req->_game = 1;
        list_req->_endpoint = 6;
        list_req->_endpoint_method = 2;
        list_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_FANOUT")};
        auto root = std::make_shared<RequestNode>(std::move(list_req));

        // Branch A: Match Details
        auto detail_req = std::make_unique<IterativeRequest>();
        detail_req->_game = 1;
        detail_req->_endpoint = 6;
        detail_req->_endpoint_method = 0;
        detail_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        detail_req->param_indices = {1};
        iter_request_link link_a;
        link_a.parent = root;
        link_a.iter_link_info.push_back(iter_access_info());
        detail_req->iter_dependencies.push_back(link_a);
        auto branch_a = std::make_shared<RequestNode>(std::move(detail_req));

        // Branch B: Match Timeline
        auto timeline_req = std::make_unique<IterativeRequest>();
        timeline_req->_game = 1;
        timeline_req->_endpoint = 6;
        timeline_req->_endpoint_method = 1;
        timeline_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        timeline_req->param_indices = {1};
        iter_request_link link_b;
        link_b.parent = root;
        link_b.iter_link_info.push_back(iter_access_info());
        timeline_req->iter_dependencies.push_back(link_b);
        auto branch_b = std::make_shared<RequestNode>(std::move(timeline_req));

        root->_node->unsent_request->children.push_back(branch_a);
        root->_node->unsent_request->children.push_back(branch_b);

        BatchRequest batch({root});

        request q0 = batch.get_next();
        assert(q0._endpoint_method == 2);
        std::string list_json = R"(["KR_201","KR_202"])";
        assert(!batch.insert_result(std::make_unique<raw_json>(list_json.begin(), list_json.end())));

        // Branch A executes 2 iterations
        request qa1 = batch.get_next();
        assert(qa1._endpoint_method == 0 && std::string(qa1.params[1].param) == "KR_201");
        assert(batch.insert_result(std::make_unique<raw_json>(list_json.begin(), list_json.end())));
        request qa2 = batch.get_current();
        assert(qa2._endpoint_method == 0 && std::string(qa2.params[1].param) == "KR_202");
        assert(!batch.insert_result(std::make_unique<raw_json>(list_json.begin(), list_json.end())));

        // Branch B executes 2 iterations
        request qb1 = batch.get_next();
        assert(qb1._endpoint_method == 1 && std::string(qb1.params[1].param) == "KR_201");
        assert(batch.insert_result(std::make_unique<raw_json>(list_json.begin(), list_json.end())));
        request qb2 = batch.get_current();
        assert(qb2._endpoint_method == 1 && std::string(qb2.params[1].param) == "KR_202");
        assert(!batch.insert_result(std::make_unique<raw_json>(list_json.begin(), list_json.end())));

        assert(batch.get_next().same_endpoint(BatchRequest::FINISHED));
        std::cout << "  [PASS] Fan-out (Branching DAG) Pipeline Execution" << std::endl;
    }

    // 8. Test Multi-Parameter Odometer (Cartesian Product in IterativeRequest)
    {
        // Parent: returns JSON with puuid and array of championIds
        auto parent_req = std::make_unique<LinkedRequest>();
        parent_req->_game = 0;
        parent_req->_endpoint = 0;
        parent_req->_endpoint_method = 1;
        parent_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("Ezreal"), PARAM_CONT("KR1")};
        auto parent_node = std::make_shared<RequestNode>(std::move(parent_req));

        // Child: Champion Mastery By PUUID By Champion (params: [Routing, PUUID, ChampionID])
        auto mastery_req = std::make_unique<IterativeRequest>();
        mastery_req->_game = 1;
        mastery_req->_endpoint = 0;
        mastery_req->_endpoint_method = 1;
        mastery_req->params = {PARAM_CONT("KR"), PARAM_CONT(""), PARAM_CONT("")};

        // Link param[1] = puuid (scalar request_link)
        request_link puuid_link;
        puuid_link.parent = parent_node;
        puuid_link.param_indices = {1};
        json_access_info puuid_acc;
        puuid_acc.keys = {KEY_CONT("puuid")};
        puuid_link.link_info.push_back(puuid_acc);
        mastery_req->dependencies.push_back(puuid_link);

        // Link param[2] = champions[].id (iterative iter_request_link)
        iter_request_link champ_link;
        champ_link.parent = parent_node;
        champ_link.param_indices = {2};
        iter_access_info champ_acc;
        champ_acc.keys = {KEY_CONT("champions")};
        champ_acc.access_after_iter.keys = {KEY_CONT("id")};
        champ_link.iter_link_info.push_back(champ_acc);
        mastery_req->iter_dependencies.push_back(champ_link);

        auto child_node = std::make_shared<RequestNode>(std::move(mastery_req));
        parent_node->_node->unsent_request->children.push_back(child_node);

        BatchRequest batch({parent_node});

        request qp = batch.get_next();
        assert(qp._game == 0);
        std::string p_resp = R"({"puuid":"EZ_PUUID_77","champions":[{"id":81},{"id":103},{"id":236}]})";
        assert(!batch.insert_result(std::make_unique<raw_json>(p_resp.begin(), p_resp.end())));

        // Child iteration 1: (EZ_PUUID_77, 81)
        request qc1 = batch.get_next();
        assert(std::string(qc1.params[1].param) == "EZ_PUUID_77");
        assert(std::string(qc1.params[2].param) == "81");
        assert(batch.insert_result(std::make_unique<raw_json>(p_resp.begin(), p_resp.end())));

        // Child iteration 2: (EZ_PUUID_77, 103)
        request qc2 = batch.get_current();
        assert(std::string(qc2.params[1].param) == "EZ_PUUID_77");
        assert(std::string(qc2.params[2].param) == "103");
        assert(batch.insert_result(std::make_unique<raw_json>(p_resp.begin(), p_resp.end())));

        // Child iteration 3: (EZ_PUUID_77, 236)
        request qc3 = batch.get_current();
        assert(std::string(qc3.params[1].param) == "EZ_PUUID_77");
        assert(std::string(qc3.params[2].param) == "236");
        assert(!batch.insert_result(std::make_unique<raw_json>(p_resp.begin(), p_resp.end())));

        assert(batch.get_next().same_endpoint(BatchRequest::FINISHED));
        std::cout << "  [PASS] Multi-parameter odometer (scalar + iterative links)" << std::endl;
    }

    // 9. Test Empty/Zero-Match Pipeline Graceful Completion
    {
        auto stage1_req = std::make_unique<LinkedRequest>();
        stage1_req->_game = 1;
        stage1_req->_endpoint = 6;
        stage1_req->_endpoint_method = 2;
        stage1_req->params = {PARAM_CONT("OC1"), PARAM_CONT("EMPTY_PUUID")};
        auto stage1 = std::make_shared<RequestNode>(std::move(stage1_req));

        auto stage2_req = std::make_unique<IterativeRequest>();
        stage2_req->_game = 1;
        stage2_req->_endpoint = 6;
        stage2_req->_endpoint_method = 0;
        stage2_req->params = {PARAM_CONT("OC1"), PARAM_CONT("")};
        stage2_req->param_indices = {1};

        iter_request_link link;
        link.parent = stage1;
        link.iter_link_info.push_back(iter_access_info());
        stage2_req->iter_dependencies.push_back(link);

        auto stage2 = std::make_shared<RequestNode>(std::move(stage2_req));
        stage1->_node->unsent_request->children.push_back(stage2);

        BatchRequest batch({stage1});
        request q1 = batch.get_next();
        assert(q1._endpoint == 6);

        // Empty match array from Riot
        std::string empty_resp = R"([])";
        auto resp_empty = std::make_unique<raw_json>(empty_resp.begin(), empty_resp.end());
        bool more = batch.insert_result(std::move(resp_empty));
        assert(!more);

        // Should be FINISHED immediately without crashing on empty array
        request q_done = batch.get_next();
        assert(q_done.same_endpoint(BatchRequest::FINISHED));
        std::cout << "  [PASS] Empty/Zero-match pipeline graceful completion" << std::endl;
    }

    // 10. Test ValuePool Unit Mechanics (Deduplication, Capacity Cap, Filter Predicate, Delimited String)
    {
        auto pool = ValuePool::FromDelimitedString("KR_101, KR_102, KR_101, KR_103 , KR_102", ',', true);
        assert(pool->size() == 3);
        assert(std::string((*pool)[0].param) == "KR_101");
        assert(std::string((*pool)[1].param) == "KR_102");
        assert(std::string((*pool)[2].param) == "KR_103");

        // Test filter predicate + max_capacity
        ValuePool filtered_pool(true, 2, [](const std::string& val) {
            return val.find("OC1_") == 0;
        });
        filtered_pool.push("KR_999");
        filtered_pool.push("OC1_100");
        filtered_pool.push("OC1_100"); // duplicate
        filtered_pool.push("OC1_200");
        filtered_pool.push("OC1_300"); // exceeds max_capacity=2
        assert(filtered_pool.size() == 2);
        assert(std::string(filtered_pool[0].param) == "OC1_100");
        assert(std::string(filtered_pool[1].param) == "OC1_200");

        std::cout << "  [PASS] ValuePool deduplication, capacity cap, filter predicate & delimited string parsing" << std::endl;
    }

    // 11. Test Multi-Level Nested Array Extraction (info.teams[*].bans[*].championId)
    {
        std::string nested_teams_json = R"({
            "info": {
                "teams": [
                    {"teamId": 100, "bans": [{"championId": 81, "pickTurn": 1}, {"championId": 103, "pickTurn": 2}]},
                    {"teamId": 200, "bans": [{"championId": 64, "pickTurn": 3}, {"championId": 81, "pickTurn": 4}]}
                ]
            }
        })";
        raw_json raw(nested_teams_json.begin(), nested_teams_json.end());

        iter_access_info bans_extractor;
        bans_extractor.keys = {KEY_CONT("info"), KEY_CONT("teams")};
        bans_extractor.access_after_iter.keys = {KEY_CONT("bans"), KEY_CONT("championId")};
        std::vector<PARAM_CONT> all_bans = bans_extractor.get_params(raw);
        assert(all_bans.size() == 4);
        assert(std::string(all_bans[0].param) == "81");
        assert(std::string(all_bans[1].param) == "103");
        assert(std::string(all_bans[2].param) == "64");
        assert(std::string(all_bans[3].param) == "81");

        // Feed into deduplicated ValuePool
        ValuePool unique_bans(true);
        bans_extractor.extract_into_pool(raw, unique_bans);
        assert(unique_bans.size() == 3);
        assert(std::string(unique_bans[0].param) == "81");
        assert(std::string(unique_bans[1].param) == "103");
        assert(std::string(unique_bans[2].param) == "64");

        std::cout << "  [PASS] Multi-level nested array JSON extraction & ValuePool deduplication" << std::endl;
    }

    // 12. Test PoolBindMode::ZIP (Correlated Tuple Iteration: gameName + tagLine from participants)
    {
        // Parent: Match Detail returning participants with riotIdGameName and riotIdTagline
        auto match_req = std::make_unique<LinkedRequest>();
        match_req->_game = 1;
        match_req->_endpoint = 6;
        match_req->_endpoint_method = 0;
        match_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("KR_ZIP_MATCH")};
        auto match_node = std::make_shared<RequestNode>(std::move(match_req));

        // Child: Account-v1 By Riot ID (params: [0]=Routing, [1]=gameName, [2]=tagLine) in ZIP mode
        auto acc_req = std::make_unique<LinkedRequest>();
        acc_req->_game = 0;
        acc_req->_endpoint = 0;
        acc_req->_endpoint_method = 1;
        acc_req->params = {PARAM_CONT("ASIA"), PARAM_CONT(""), PARAM_CONT("")};
        acc_req->bind_mode = PoolBindMode::ZIP;

        iter_request_link zip_link;
        zip_link.parent = match_node;
        zip_link.param_indices = {1, 2};

        iter_access_info name_extract;
        name_extract.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        name_extract.access_after_iter.keys = {KEY_CONT("riotIdGameName")};

        iter_access_info tag_extract;
        tag_extract.keys = {KEY_CONT("info"), KEY_CONT("participants")};
        tag_extract.access_after_iter.keys = {KEY_CONT("riotIdTagline")};

        zip_link.iter_link_info = {name_extract, tag_extract};
        acc_req->iter_dependencies.push_back(zip_link);

        auto acc_node = std::make_shared<RequestNode>(std::move(acc_req));
        match_node->_node->unsent_request->children.push_back(acc_node);

        BatchRequest batch({match_node});
        request q_match = batch.get_next();
        assert(q_match._game == 1);

        std::string match_json = R"({
            "info": {
                "participants": [
                    {"riotIdGameName": "Faker", "riotIdTagline": "T1"},
                    {"riotIdGameName": "Chovy", "riotIdTagline": "GEN"},
                    {"riotIdGameName": "ShowMaker", "riotIdTagline": "DK"}
                ]
            }
        })";
        assert(!batch.insert_result(std::make_unique<raw_json>(match_json.begin(), match_json.end())));

        // In ZIP mode, must produce exactly 3 requests: (Faker, T1), (Chovy, GEN), (ShowMaker, DK)
        // instead of 3x3 = 9 Cartesian product combinations
        request q_z1 = batch.get_next();
        assert(std::string(q_z1.params[0].param) == "ASIA");
        assert(std::string(q_z1.params[1].param) == "Faker");
        assert(std::string(q_z1.params[2].param) == "T1");
        assert(batch.insert_result(std::make_unique<raw_json>(match_json.begin(), match_json.end())));

        request q_z2 = batch.get_current();
        assert(std::string(q_z2.params[0].param) == "ASIA");
        assert(std::string(q_z2.params[1].param) == "Chovy");
        assert(std::string(q_z2.params[2].param) == "GEN");
        assert(batch.insert_result(std::make_unique<raw_json>(match_json.begin(), match_json.end())));

        request q_z3 = batch.get_current();
        assert(std::string(q_z3.params[0].param) == "ASIA");
        assert(std::string(q_z3.params[1].param) == "ShowMaker");
        assert(std::string(q_z3.params[2].param) == "DK");
        assert(!batch.insert_result(std::make_unique<raw_json>(match_json.begin(), match_json.end())));

        assert(batch.get_next().same_endpoint(BatchRequest::FINISHED));
        std::cout << "  [PASS] PoolBindMode::ZIP correlated tuple iteration (1-to-1 paired pools)" << std::endl;
    }

    // 13. Test Multi-Parent Pool Merging & Automatic Deduplication
    {
        // Parent A and Parent B both return match ID lists for two teammates who played together
        auto p1_req = std::make_unique<LinkedRequest>();
        p1_req->_game = 1; p1_req->_endpoint = 6; p1_req->_endpoint_method = 2;
        p1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_TEAMMATE_1")};
        auto parent1 = std::make_shared<RequestNode>(std::move(p1_req));

        auto p2_req = std::make_unique<LinkedRequest>();
        p2_req->_game = 1; p2_req->_endpoint = 6; p2_req->_endpoint_method = 2;
        p2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_TEAMMATE_2")};
        auto parent2 = std::make_shared<RequestNode>(std::move(p2_req));

        // Child: Match Detail node with deduplicated ValuePool on param[1]
        auto detail_req = std::make_unique<LinkedRequest>();
        detail_req->_game = 1; detail_req->_endpoint = 6; detail_req->_endpoint_method = 0;
        detail_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        detail_req->get_or_create_pool(1)->set_deduplicate(true);

        iter_request_link link_p1;
        link_p1.parent = parent1;
        link_p1.param_indices = {1};
        link_p1.iter_link_info.push_back(iter_access_info());
        detail_req->iter_dependencies.push_back(link_p1);

        iter_request_link link_p2;
        link_p2.parent = parent2;
        link_p2.param_indices = {1};
        link_p2.iter_link_info.push_back(iter_access_info());
        detail_req->iter_dependencies.push_back(link_p2);

        auto child = std::make_shared<RequestNode>(std::move(detail_req));
        parent1->_node->unsent_request->children.push_back(child);
        parent2->_node->unsent_request->children.push_back(child);

        BatchRequest batch({parent1, parent2});

        // Parent 1 returns ["KR_SHARED_1", "KR_SHARED_2", "KR_ONLY_P1"]
        request qp1 = batch.get_next();
        assert(std::string(qp1.params[1].param) == "PUUID_TEAMMATE_1");
        std::string p1_json = R"(["KR_SHARED_1","KR_SHARED_2","KR_ONLY_P1"])";
        assert(!batch.insert_result(std::make_unique<raw_json>(p1_json.begin(), p1_json.end())));

        // Parent 2 returns ["KR_SHARED_2", "KR_ONLY_P2", "KR_SHARED_1"]
        request qp2 = batch.get_next();
        assert(std::string(qp2.params[1].param) == "PUUID_TEAMMATE_2");
        std::string p2_json = R"(["KR_SHARED_2","KR_ONLY_P2","KR_SHARED_1"])";
        assert(!batch.insert_result(std::make_unique<raw_json>(p2_json.begin(), p2_json.end())));

        // Child pool must contain the 4 unique matches in insertion order:
        // KR_SHARED_1, KR_SHARED_2, KR_ONLY_P1, KR_ONLY_P2
        std::vector<std::string> expected = {"KR_SHARED_1", "KR_SHARED_2", "KR_ONLY_P1", "KR_ONLY_P2"};
        for (size_t i = 0; i < expected.size(); ++i) {
            request qc = (i == 0) ? batch.get_next() : batch.get_current();
            assert(std::string(qc.params[1].param) == expected[i]);
            bool more = batch.insert_result(std::make_unique<raw_json>(p1_json.begin(), p1_json.end()));
            assert(more == (i + 1 < expected.size()));
        }

        assert(batch.get_next().same_endpoint(BatchRequest::FINISHED));
        std::cout << "  [PASS] Multi-parent ValuePool merging & cross-parent deduplication" << std::endl;
    }

    // 14. Test Root Node with Multi-Value Seeded ValuePool -> Downstream Child
    {
        auto root_req = std::make_unique<LinkedRequest>();
        root_req->_game = 0; root_req->_endpoint = 0; root_req->_endpoint_method = 0; // Account by_puuid
        root_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        root_req->set_param_pool(0, ValuePool::FromDelimitedString("ASIA"));
        root_req->set_param_pool(1, ValuePool::FromDelimitedString("PUUID_SEED_1, PUUID_SEED_2"));
        auto root_node = std::make_shared<RequestNode>(std::move(root_req));

        auto child_req = std::make_unique<LinkedRequest>();
        child_req->_game = 1; child_req->_endpoint = 6; child_req->_endpoint_method = 2; // Match list by_puuid
        child_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        request_link link;
        link.parent = root_node;
        link.param_indices = {1};
        json_access_info puuid_acc;
        puuid_acc.keys = {KEY_CONT("puuid")};
        link.link_info.push_back(puuid_acc);
        child_req->dependencies.push_back(link);
        auto child_node = std::make_shared<RequestNode>(std::move(child_req));
        root_node->_node->unsent_request->children.push_back(child_node);

        BatchRequest batch({root_node});

        // Root executes for PUUID_SEED_1 and PUUID_SEED_2
        request qr1 = batch.get_next();
        assert(std::string(qr1.params[1].param) == "PUUID_SEED_1");
        std::string r1_resp = R"({"puuid":"RESOLVED_1"})";
        assert(batch.insert_result(std::make_unique<raw_json>(r1_resp.begin(), r1_resp.end())));

        request qr2 = batch.get_current();
        assert(std::string(qr2.params[1].param) == "PUUID_SEED_2");
        std::string r2_resp = R"({"puuid":"RESOLVED_2"})";
        assert(!batch.insert_result(std::make_unique<raw_json>(r2_resp.begin(), r2_resp.end())));

        // Child automatically iterates over RESOLVED_1 and RESOLVED_2
        request qc1 = batch.get_next();
        assert(std::string(qc1.params[1].param) == "RESOLVED_1");
        assert(batch.insert_result(std::make_unique<raw_json>(r1_resp.begin(), r1_resp.end())));

        request qc2 = batch.get_current();
        assert(std::string(qc2.params[1].param) == "RESOLVED_2");
        assert(!batch.insert_result(std::make_unique<raw_json>(r2_resp.begin(), r2_resp.end())));

        assert(batch.get_next().same_endpoint(BatchRequest::FINISHED));
        std::cout << "  [PASS] Root node with multi-value seeded ValuePool -> downstream child iteration" << std::endl;
    }

    // 15. Test Rate-Limit-Aware Ready Node Interleaving & Opportunistic Child Streaming
    {
        auto root_req = std::make_unique<LinkedRequest>();
        root_req->_game = 0; root_req->_endpoint = 0; root_req->_endpoint_method = 0; // Account by_puuid
        root_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        root_req->set_param_pool(0, ValuePool::FromDelimitedString("ASIA"));
        root_req->set_param_pool(1, ValuePool::FromDelimitedString("P1, P2"));
        auto root_node = std::make_shared<RequestNode>(std::move(root_req));

        auto child_req = std::make_unique<LinkedRequest>();
        child_req->_game = 1; child_req->_endpoint = 6; child_req->_endpoint_method = 2; // Match list by_puuid
        child_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        request_link link;
        link.parent = root_node;
        link.param_indices = {1};
        json_access_info puuid_acc;
        puuid_acc.keys = {KEY_CONT("puuid")};
        link.link_info.push_back(puuid_acc);
        child_req->dependencies.push_back(link);
        auto child_node = std::make_shared<RequestNode>(std::move(child_req));
        root_node->_node->unsent_request->children.push_back(child_node);

        BatchRequest batch({root_node});
        int account_wait = 0;
        auto wait_fn = [&](const request& req) -> int {
            return (req._game == 0) ? account_wait : 0;
        };

        // 1. Root sends first item (P1) with 0 wait
        request r1 = batch.select_next_ready(wait_fn);
        assert(r1._game == 0 && std::string(r1.params[1].param) == "P1");
        std::string resp1 = R"({"puuid":"STREAMED_1"})";
        batch.insert_result(std::make_unique<raw_json>(resp1.begin(), resp1.end()));

        // 2. Now simulate Account-v1 hitting a Method Rate Limit (account_wait = 10s), while Match-v5 has 0s wait!
        // select_next_ready should opportunistically start the child node using P1's already-available response!
        account_wait = 10;
        request c1 = batch.select_next_ready(wait_fn);
        assert(c1._game == 1 && c1._endpoint == 6 && std::string(c1.params[1].param) == "STREAMED_1");
        std::string child_resp1 = R"(["KR_1"])";
        batch.insert_result(std::make_unique<raw_json>(child_resp1.begin(), child_resp1.end()));

        // 3. Account-v1 rate limit clears; root node resumes at P2 without losing state
        account_wait = 0;
        request r2 = batch.select_next_ready(wait_fn);
        assert(r2._game == 0 && std::string(r2.params[1].param) == "P2");
        std::string resp2 = R"({"puuid":"STREAMED_2"})";
        batch.insert_result(std::make_unique<raw_json>(resp2.begin(), resp2.end()));

        // 4. Child node receives STREAMED_2 and finishes
        request c2 = batch.select_next_ready(wait_fn);
        assert(c2._game == 1 && c2._endpoint == 6 && std::string(c2.params[1].param) == "STREAMED_2");
        batch.insert_result(std::make_unique<raw_json>(child_resp1.begin(), child_resp1.end()));

        assert(batch.select_next_ready(wait_fn).same_endpoint(BatchRequest::FINISHED));
        std::cout << "  [PASS] Rate-limit-aware node interleaving & opportunistic child streaming" << std::endl;
    }

    // 25. Test ValuePool::SkipPlayersWithMatchesInDB pre-filtering
    {
        std::string batch_db_path = "./test_output_batch/test_batch.duckdb";
        if (std::filesystem::exists("./test_output_batch")) {
            std::filesystem::remove_all("./test_output_batch");
        }
        std::filesystem::create_directories("./test_output_batch");

        restfulEz::DatabaseManager& db = restfulEz::DatabaseManager::GetInstance();
        db.Init(batch_db_path);

        // Populate a match with participant PUUID_A_FAKER
        db.ResolveOrCreatePlayer("DEV_A", "PUUID_A_FAKER", "Faker", "KR1");
        std::string sample_match = R"({
            "metadata": { "matchId": "KR_BATCH_001" },
            "info": {
                "gameCreation": 1650000000000,
                "gameDuration": 1800,
                "gameVersion": "14.10.1",
                "queueId": 420,
                "participants": [
                    { "participantId": 1, "puuid": "PUUID_A_FAKER", "riotIdGameName": "Faker", "riotIdTagline": "KR1", "championName": "Ahri", "championId": 103, "teamId": 100, "kills": 5, "deaths": 0, "assists": 10, "win": true }
                ]
            }
        })";
        db.SaveMatch("KR_BATCH_001", sample_match);

        // Also save player without matches
        db.ResolveOrCreatePlayer("DEV_A", "PUUID_A_EZREAL", "Ezreal", "NA1");

        // Create pool with SkipPlayersWithMatchesInDB
        ValuePool pool(true);
        pool.set_filter(ValuePool::SkipPlayersWithMatchesInDB(1));
        assert(pool.has_filter());

        // Pushing PUUID_A_FAKER should be filtered out because player already has 1 match in DB
        bool pushed_faker = pool.push("PUUID_A_FAKER");
        assert(!pushed_faker);
        assert(pool.filtered_out_count() == 1);
        assert(pool.size() == 0);

        // Pushing PUUID_A_EZREAL should succeed because player has 0 matches
        bool pushed_ezreal = pool.push("PUUID_A_EZREAL");
        assert(pushed_ezreal);
        assert(pool.filtered_out_count() == 1);
        assert(pool.size() == 1);

        // Pushing brand new PUUID should succeed
        bool pushed_new = pool.push("PUUID_BRAND_NEW");
        assert(pushed_new);
        assert(pool.filtered_out_count() == 1);
        assert(pool.size() == 2);

        // Test with higher min_matches threshold (e.g. min_matches = 2)
        ValuePool pool2(true);
        pool2.set_filter(ValuePool::SkipPlayersWithMatchesInDB(2));
        // PUUID_A_FAKER has only 1 match (< 2), so it should NOT be skipped!
        bool pushed_faker2 = pool2.push("PUUID_A_FAKER");
        assert(pushed_faker2);
        assert(pool2.filtered_out_count() == 0);
        assert(pool2.size() == 1);

        db.Close();
        std::filesystem::remove_all("./test_output_batch");
        std::cout << "  [PASS] ValuePool::SkipPlayersWithMatchesInDB filter & threshold behavior" << std::endl;
    }

    std::cout << "[Test] All BatchRequests & ValuePool Pipeline tests passed successfully!" << std::endl;
    return 0;
}
