#include "BatchRequests.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace restfulEz;

int main() {
    std::cout << "[Test] Running BatchRequests & Pipeline Unit Tests..." << std::endl;

    // 1. Test json_access_info::get_param
    {
        std::string sample_account_json = R"({"puuid":"TEST_PUUID_12345","gameName":"Faker","tagLine":"KR1"})";
        raw_json raw(sample_account_json.begin(), sample_account_json.end());

        json_access_info info;
        info.keys.push_back(KEY_CONT("puuid"));
        PARAM_CONT extracted = info.get_param(raw);
        assert(std::string(extracted.param) == "TEST_PUUID_12345");

        json_access_info name_info;
        name_info.keys.push_back(KEY_CONT("gameName"));
        PARAM_CONT extracted_name = name_info.get_param(raw);
        assert(std::string(extracted_name.param) == "Faker");
        std::cout << "  [PASS] json_access_info scalar field extraction" << std::endl;
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

    // 4. Test 3-Stage Pipeline: Account -> Match List (by_puuid) -> Match Details (by_match_id)
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
        // Iteration 1
        request q3_1 = batch.get_next();
        assert(q3_1._game == 1 && q3_1._endpoint == 6 && q3_1._endpoint_method == 0);
        assert(std::string(q3_1.params[1].param) == "OC1_501");

        auto resp3_1 = std::make_unique<raw_json>(matches_resp.begin(), matches_resp.end());
        bool more_s3_1 = batch.insert_result(std::move(resp3_1));
        assert(more_s3_1); // More iterations remain!

        // Iteration 2
        request q3_2 = batch.get_current();
        assert(std::string(q3_2.params[1].param) == "OC1_502");

        auto resp3_2 = std::make_unique<raw_json>(matches_resp.begin(), matches_resp.end());
        bool more_s3_2 = batch.insert_result(std::move(resp3_2));
        assert(more_s3_2);

        // Iteration 3
        request q3_3 = batch.get_current();
        assert(std::string(q3_3.params[1].param) == "OC1_503");

        auto resp3_3 = std::make_unique<raw_json>(matches_resp.begin(), matches_resp.end());
        bool more_s3_3 = batch.insert_result(std::move(resp3_3));
        assert(!more_s3_3); // Completed all 3 match iterations!

        // Query 4: Finished!
        request q_done = batch.get_next();
        assert(q_done.same_endpoint(BatchRequest::FINISHED));

        std::cout << "  [PASS] 3-Stage Batch Pipeline Execution & Iteration" << std::endl;
    }

    // 5. Test Empty/Zero-Match Pipeline Graceful Completion
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

    std::cout << "[Test] All BatchRequests & Pipeline tests passed successfully!" << std::endl;
    return 0;
}
