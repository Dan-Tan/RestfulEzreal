#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>

#include "imgui.h"
#include "QueryForm.h"
#include "TemplateManager.h"
#include "FileManager.h"
#include "BatchRequests.h"

using namespace restfulEz;

int main() {
    std::cout << "[Test] Running TemplateManager and Template System Unit Tests..." << std::endl;

    // Headless ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext* imgui_ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);

    // =========================================================================
    // TEST 1: Starter Template 1 (Match History & Timelines) Deserialization
    // =========================================================================
    {
        std::cout << "\n[Test 1] Starter Template 1 (Match History & Timelines Pipeline)..." << std::endl;
        BatchForm bf;
        auto& tm = TemplateManager::GetInstance();
        const std::string& json_str = tm.GetStarterMatchHistoryTimelinesJson();
        assert(!json_str.empty());

        bool ok = tm.LoadTemplateFromString(json_str, bf);
        assert(ok);
        assert(bf.get_forms().size() == 4);

        // Node 0: Account-v1 By Riot ID (Root)
        auto node0 = bf.get_forms()[0];
        assert(node0 != nullptr);
        assert(node0->get_id() == 0);
        assert(node0->get_game() == 0);
        assert(node0->get_endpoint() == 0);
        assert(node0->get_endpoint_method() == 1);
        assert(!node0->get_zip_mode());
        assert(node0->get_window_position().x == 165.0f);
        assert(node0->get_window_position().y == 70.0f);
        assert(node0->get_param_count() == 3);
        assert(node0->get_param_name(0) == "Routing");
        assert(node0->get_param_value(0) == "ASIA");
        assert(node0->get_param_name(1) == "Game Name");
        assert(node0->get_param_value(1) == "Faker");
        assert(node0->get_param_name(2) == "Tagline");
        assert(node0->get_param_value(2) == "KR1");
        assert(!node0->is_param_linked(0));
        assert(!node0->is_param_linked(1));
        assert(!node0->is_param_linked(2));
        assert(!node0->check_dependent());
        assert(node0->check_ready());

        // Node 1: Match-v5 By Puuid
        auto node1 = bf.get_forms()[1];
        assert(node1 != nullptr);
        assert(node1->get_id() == 1);
        assert(node1->get_game() == 1);
        assert(node1->get_endpoint() == 6);
        assert(node1->get_endpoint_method() == 2);
        assert(node1->get_window_position().x == 435.0f);
        assert(node1->get_window_position().y == 70.0f);
        assert(node1->get_param_count() == 2);
        assert(node1->get_param_name(0) == "Routing");
        assert(node1->get_param_value(0) == "ASIA");
        assert(node1->get_param_name(1) == "PUUID");
        assert(node1->is_param_linked(1));
        assert(!node1->is_param_iterative(1));
        assert(node1->get_parent_for_index(1) == node0);
        auto keys_b1 = node1->get_keys_before(1);
        assert(keys_b1.size() == 1 && keys_b1[0] == "puuid");
        assert(node1->check_dependent());
        assert(node1->check_ready());

        // Verify count optional on Node 1
        bool found_count_opt = false;
        for (std::size_t i = 0; i < node1->get_optional_count(); ++i) {
            if (node1->get_optional_name(i) == "count") {
                assert(node1->is_optional_enabled(i));
                assert(node1->get_optional_value(i) == "5");
                found_count_opt = true;
                break;
            }
        }
        assert(found_count_opt);

        // Node 2: Match-v5 By Match ID
        auto node2 = bf.get_forms()[2];
        assert(node2 != nullptr);
        assert(node2->get_id() == 2);
        assert(node2->get_game() == 1);
        assert(node2->get_endpoint() == 6);
        assert(node2->get_endpoint_method() == 0);
        assert(node2->get_window_position().x == 705.0f);
        assert(node2->get_window_position().y == 50.0f);
        assert(node2->is_param_linked(1));
        assert(node2->is_param_iterative(1));
        assert(node2->is_param_deduplicate(1));
        assert(node2->is_param_skip_db(1));
        assert(node2->get_iter_limit_val(1) == 5);
        assert(node2->get_parent_for_index(1) == node1);
        assert(node2->check_dependent());
        assert(node2->check_ready());

        // Node 3: Match-v5 Timeline
        auto node3 = bf.get_forms()[3];
        assert(node3 != nullptr);
        assert(node3->get_id() == 3);
        assert(node3->get_game() == 1);
        assert(node3->get_endpoint() == 6);
        assert(node3->get_endpoint_method() == 1);
        assert(node3->get_window_position().x == 705.0f);
        assert(node3->get_window_position().y == 270.0f);
        assert(node3->is_param_linked(1));
        assert(node3->is_param_iterative(1));
        assert(node3->is_param_deduplicate(1));
        assert(node3->is_param_skip_db(1));
        assert(node3->get_iter_limit_val(1) == 5);
        assert(node3->get_parent_for_index(1) == node1);
        assert(node3->check_dependent());
        assert(node3->check_ready());

        // Construct request and verify underlying ValuePool & link wiring
        for (auto& form : bf.get_forms()) form->construct_base();
        for (auto& form : bf.get_forms()) form->link_final_requests();

        auto& req2 = node2->get_base_request()->_node->unsent_request;
        assert(req2 != nullptr);
        assert(req2->get_or_create_pool(1)->is_deduplicated());

        auto& req3 = node3->get_base_request()->_node->unsent_request;
        assert(req3 != nullptr);
        assert(req3->get_or_create_pool(1)->is_deduplicated());

        std::cout << "  [PASS] Starter Template 1 loaded and verified successfully." << std::endl;
    }

    // =========================================================================
    // TEST 2: Starter Template 2 (Match Participants Expansion) Deserialization
    // =========================================================================
    {
        std::cout << "\n[Test 2] Starter Template 2 (Match Participants Expansion Pipeline)..." << std::endl;
        BatchForm bf;
        auto& tm = TemplateManager::GetInstance();
        const std::string& json_str = tm.GetStarterParticipantExpansionJson();
        assert(!json_str.empty());

        bool ok = tm.LoadTemplateFromString(json_str, bf);
        assert(ok);
        assert(bf.get_forms().size() == 4);

        // Node positions for Starter Template 2
        auto node0 = bf.get_forms()[0];
        auto node1 = bf.get_forms()[1];
        auto node2 = bf.get_forms()[2];
        assert(node0 != nullptr);
        assert(node0->get_window_position().x == 165.0f);
        assert(node0->get_window_position().y == 70.0f);
        assert(node1 != nullptr);
        assert(node1->get_window_position().x == 435.0f);
        assert(node1->get_window_position().y == 70.0f);
        assert(node2 != nullptr);
        assert(node2->get_window_position().x == 705.0f);
        assert(node2->get_window_position().y == 70.0f);

        // Node 3: Account-v1 By Riot ID in ZIP mode
        auto node3 = bf.get_forms()[3];
        assert(node3 != nullptr);
        assert(node3->get_id() == 3);
        assert(node3->get_game() == 0);
        assert(node3->get_endpoint() == 0);
        assert(node3->get_endpoint_method() == 1);
        assert(node3->get_zip_mode() == true);
        assert(node3->get_window_position().x == 705.0f);
        assert(node3->get_window_position().y == 290.0f);
        assert(node3->get_param_count() == 3);

        // Param 1: Game Name (linked to Match-v5 Details Node 2)
        assert(node3->is_param_linked(1));
        assert(node3->is_param_iterative(1));
        assert(node3->is_param_deduplicate(1));
        assert(!node3->is_param_skip_db(1));
        assert(node3->get_iter_limit_val(1) == 5);
        assert(node3->get_parent_for_index(1) == bf.get_forms()[2]);
        auto keys_b1 = node3->get_keys_before(1);
        auto keys_a1 = node3->get_keys_after(1);
        assert(keys_b1.size() == 2 && keys_b1[0] == "info" && keys_b1[1] == "participants");
        assert(keys_a1.size() == 1 && keys_a1[0] == "riotIdGameName");

        // Param 2: Tagline (linked to Match-v5 Details Node 2)
        assert(node3->is_param_linked(2));
        assert(node3->is_param_iterative(2));
        assert(node3->is_param_deduplicate(2));
        assert(!node3->is_param_skip_db(2));
        assert(node3->get_iter_limit_val(2) == 5);
        assert(node3->get_parent_for_index(2) == bf.get_forms()[2]);
        auto keys_b2 = node3->get_keys_before(2);
        auto keys_a2 = node3->get_keys_after(2);
        assert(keys_b2.size() == 2 && keys_b2[0] == "info" && keys_b2[1] == "participants");
        assert(keys_a2.size() == 1 && keys_a2[0] == "riotIdTagline");

        // Construct request and verify underlying ZIP mode binding
        for (auto& form : bf.get_forms()) form->construct_base();
        for (auto& form : bf.get_forms()) form->link_final_requests();

        auto& req3 = node3->get_base_request()->_node->unsent_request;
        assert(req3 != nullptr);
        assert(req3->bind_mode == PoolBindMode::ZIP);
        assert(req3->get_or_create_pool(1)->is_deduplicated());
        assert(req3->get_or_create_pool(2)->is_deduplicated());

        std::cout << "  [PASS] Starter Template 2 (ZIP mode) loaded and verified successfully." << std::endl;
    }

    // =========================================================================
    // TEST 3: QuickFillRootParameters
    // =========================================================================
    {
        std::cout << "\n[Test 3] QuickFillRootParameters Propagation..." << std::endl;
        BatchForm bf;
        auto& tm = TemplateManager::GetInstance();
        tm.LoadTemplateFromString(tm.GetStarterMatchHistoryTimelinesJson(), bf);

        bool filled = tm.QuickFillRootParameters(bf, "EUROPE", "Caps", "EUW");
        assert(filled);

        // Root node parameters updated
        auto node0 = bf.get_forms()[0];
        assert(node0->get_param_value(0) == "EUROPE");
        assert(node0->get_param_value(1) == "Caps");
        assert(node0->get_param_value(2) == "EUW");

        // Downstream unlinked Routing parameters updated to EUROPE
        for (std::size_t i = 1; i < bf.get_forms().size(); ++i) {
            auto form = bf.get_forms()[i];
            assert(form->get_param_value(0) == "EUROPE");
            assert(form->check_ready());
        }

        std::cout << "  [PASS] Quick-Fill successfully updated root and routing parameters." << std::endl;
    }

    // =========================================================================
    // TEST 4: SaveTemplate and LoadTemplate Roundtrip
    // =========================================================================
    {
        std::cout << "\n[Test 4] SaveTemplate and LoadTemplate Roundtrip..." << std::endl;
        std::string test_dir = "./test_template_ws";
        std::string test_file = test_dir / std::filesystem::path("custom_test_template.json");
        std::filesystem::remove_all(test_dir);
        std::filesystem::create_directories(test_dir);

        BatchForm bf_src;
        // Node 0: Account By Riot ID (N=3)
        auto formA = LinkedInterface::make_linked(0, 0, 1);
        formA->set_id(10);
        formA->set_window_position(ImVec2(100.0f, 150.0f));
        formA->set_locked(true);
        formA->set_param_value(0, "AMERICAS");
        formA->set_param_value(1, "Tyler1");
        formA->set_param_value(2, "NA1");
        formA->refresh_display();
        bf_src.add_form(formA);

        // Node 1: Match By Puuid (N=2)
        auto formB = LinkedInterface::make_linked(1, 6, 2);
        formB->set_id(20);
        formB->set_window_position(ImVec2(450.0f, 150.0f));
        formB->set_locked(false);
        formB->set_param_value(0, "AMERICAS");
        formB->set_parent_for_index(1, formA);
        formA->insert_child(formB);
        formB->set_keys_before(1, {"puuid"});
        // Set count optional = 12
        for (std::size_t j = 0; j < formB->get_optional_count(); ++j) {
            if (formB->get_optional_name(j) == "count") {
                formB->set_optional(j, "12", true);
            }
        }
        formB->refresh_display();
        bf_src.add_form(formB);

        // Node 2: Match By Match ID (N=2)
        auto formC = LinkedInterface::make_linked(1, 6, 0);
        formC->set_id(30);
        formC->set_window_position(ImVec2(800.0f, 150.0f));
        formC->set_locked(false);
        formC->set_param_value(0, "AMERICAS");
        formC->set_parent_for_index(1, formB);
        formB->insert_child(formC);
        formC->set_param_iterative(1, true);
        formC->set_param_deduplicate(1, true);
        formC->set_param_skip_db(1, true);
        formC->set_iter_limit_val(1, 8);
        formC->refresh_display();
        bf_src.add_form(formC);

        // Save
        auto& tm = TemplateManager::GetInstance();
        bool saved = tm.SaveTemplate(test_file, "Custom Test Graph", "Graph with 3 linked nodes", bf_src);
        assert(saved);
        assert(std::filesystem::exists(test_file));

        // Load into new BatchForm
        BatchForm bf_dst;
        bool loaded = tm.LoadTemplate(test_file, bf_dst);
        assert(loaded);
        assert(bf_dst.get_forms().size() == 3);

        auto loadedA = bf_dst.get_forms()[0];
        assert(loadedA->get_id() == 10);
        assert(loadedA->get_window_position().x == 100.0f);
        assert(loadedA->get_window_position().y == 150.0f);
        assert(loadedA->is_locked() == true);
        assert(loadedA->get_param_value(0) == "AMERICAS");
        assert(loadedA->get_param_value(1) == "Tyler1");
        assert(loadedA->get_param_value(2) == "NA1");

        auto loadedB = bf_dst.get_forms()[1];
        assert(loadedB->get_id() == 20);
        assert(loadedB->get_window_position().x == 450.0f);
        assert(loadedB->is_locked() == false);
        assert(loadedB->is_param_linked(1));
        assert(!loadedB->is_param_iterative(1));
        assert(loadedB->get_parent_for_index(1) == loadedA);
        assert(loadedB->get_keys_before(1).size() == 1 && loadedB->get_keys_before(1)[0] == "puuid");
        for (std::size_t j = 0; j < loadedB->get_optional_count(); ++j) {
            if (loadedB->get_optional_name(j) == "count") {
                assert(loadedB->is_optional_enabled(j));
                assert(loadedB->get_optional_value(j) == "12");
            }
        }

        auto loadedC = bf_dst.get_forms()[2];
        assert(loadedC->get_id() == 30);
        assert(loadedC->get_window_position().x == 800.0f);
        assert(loadedC->is_param_linked(1));
        assert(loadedC->is_param_iterative(1));
        assert(loadedC->is_param_deduplicate(1));
        assert(loadedC->is_param_skip_db(1));
        assert(loadedC->get_iter_limit_val(1) == 8);
        assert(loadedC->get_parent_for_index(1) == loadedB);

        std::filesystem::remove_all(test_dir);
        std::cout << "  [PASS] SaveTemplate & LoadTemplate full roundtrip preserved all metadata." << std::endl;
    }

    // =========================================================================
    // TEST 5: Workspace Templates Management
    // =========================================================================
    {
        std::cout << "\n[Test 5] Workspace Templates Synchronization..." << std::endl;
        std::string ws_dir = "./test_ws_templates_sync";
        std::filesystem::remove_all(ws_dir);

        auto& tm = TemplateManager::GetInstance();
        tm.EnsureStarterTemplatesInWorkspace(ws_dir);

        assert(std::filesystem::exists(ws_dir + "/starter_match_history_timelines.json"));
        assert(std::filesystem::exists(ws_dir + "/starter_participant_expansion.json"));
        assert(std::filesystem::exists(ws_dir + "/starter_challenger_database.json"));

        auto available = tm.ListAvailableTemplates(ws_dir);
        assert(available.size() == 3);
        assert(!available[0].first.empty());
        assert(!available[1].first.empty());
        assert(!available[2].first.empty());

        // Verify Challenger Database Builder template loading and incremental SkipExistingPlayersInDB
        BatchForm bf_chal;
        bool ok_chal = tm.LoadTemplateFromString(tm.GetStarterChallengerDatabaseJson(), bf_chal);
        assert(ok_chal);
        assert(bf_chal.get_forms().size() == 5);
        assert(bf_chal.get_forms()[0]->get_game() == 1);
        assert(bf_chal.get_forms()[0]->get_endpoint() == 3);
        assert(bf_chal.get_forms()[0]->get_endpoint_method() == 0);
        assert(bf_chal.get_forms()[1]->is_param_skip_db(1) == true);
        assert(bf_chal.get_forms()[1]->get_iter_limit_val(1) == 25);
        assert(bf_chal.get_forms()[3]->is_param_skip_db(1) == true);
        assert(bf_chal.get_forms()[4]->is_param_skip_db(1) == true);

        std::filesystem::remove_all(ws_dir);
        std::cout << "  [PASS] Workspace templates creation and Challenger Database Builder verified." << std::endl;
    }

    // =========================================================================
    // TEST 6: Error Handling & Malformed Input
    // =========================================================================
    {
        std::cout << "\n[Test 6] Malformed Input & Error Handling..." << std::endl;
        BatchForm bf;
        auto& tm = TemplateManager::GetInstance();

        // Malformed JSON
        bool ok1 = tm.LoadTemplateFromString("{ invalid json", bf);
        assert(!ok1);
        assert(bf.get_forms().empty());

        // Missing nodes
        bool ok2 = tm.LoadTemplateFromString(R"({"name": "No nodes"})", bf);
        assert(!ok2);
        assert(bf.get_forms().empty());

        // Non-existent file
        bool ok3 = tm.LoadTemplate("./non_existent_file_path_9999.json", bf);
        assert(!ok3);

        // Invalid game enum (make_linked throws std::invalid_argument)
        std::string invalid_game_json = R"({
          "schema_version": 1,
          "name": "Invalid Game",
          "nodes": [
            {
              "id": 0,
              "game": 999,
              "endpoint": 0,
              "endpoint_method": 0
            }
          ]
        })";
        bool ok4 = tm.LoadTemplateFromString(invalid_game_json, bf);
        assert(!ok4);
        assert(bf.get_forms().empty());

        // Schema type mismatch (id is string instead of int -> Json::LogicError)
        std::string type_mismatch_json = R"({
          "schema_version": 1,
          "name": "Type Mismatch",
          "nodes": [
            {
              "id": "not_an_int",
              "game": 0,
              "endpoint": 0,
              "endpoint_method": 1
            }
          ]
        })";
        bool ok5 = tm.LoadTemplateFromString(type_mismatch_json, bf);
        assert(!ok5);
        assert(bf.get_forms().empty());

        // Partial success followed by failure (ensures clear_canvas on exception)
        std::string dirty_state_json = R"({
          "schema_version": 1,
          "name": "Dirty State Test",
          "nodes": [
            {
              "id": 0,
              "game": 0,
              "endpoint": 0,
              "endpoint_method": 1
            },
            {
              "id": 1,
              "game": 999,
              "endpoint": 0,
              "endpoint_method": 0
            }
          ]
        })";
        bool ok6 = tm.LoadTemplateFromString(dirty_state_json, bf);
        assert(!ok6);
        assert(bf.get_forms().empty());

        std::cout << "  [PASS] Graceful error handling for corrupt and missing files." << std::endl;
    }

    ImGui::DestroyContext(imgui_ctx);
    std::cout << "\n[SUCCESS] All TemplateManager unit & integration tests passed!" << std::endl;
    return 0;
}
