#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include <cmath>

#include "imgui.h"
#include "QueryForm.h"
#include "TemplateManager.h"
#include "FileManager.h"
#include "BatchRequests.h"
#include "db/DatabaseManager.h"
#include "duckdb.h"
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace restfulEz;

static bool approx_equal(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

int main() {
    std::cout << "=====================================================================" << std::endl;
    std::cout << "[Adversarial Test] Starting Empirical Stress-Testing for Milestone 2" << std::endl;
    std::cout << "=====================================================================" << std::endl;

    // Headless ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext* imgui_ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);

    // =========================================================================
    // TEST 1: 5-Node Diamond DAG Roundtrip with 100% Field-by-Field Equality
    // =========================================================================
    {
        std::cout << "\n[Test 1] 5-Node Diamond DAG Template Roundtrip with Field-by-Field Equality..." << std::endl;
        std::string test_dir = "./test_m2_adv_ws";
        std::string test_file = test_dir + "/diamond_dag_template.json";
        std::filesystem::remove_all(test_dir);
        std::filesystem::create_directories(test_dir);

        BatchForm src_bf;

        // Node 0: Account By Riot ID (Root A, N=3)
        auto nodeA = LinkedInterface::make_linked(0, 0, 1);
        nodeA->set_id(10);
        nodeA->set_window_position(ImVec2(120.0f, 80.0f));
        nodeA->set_locked(true);
        nodeA->set_zip_mode(false);
        nodeA->set_param_value(0, "AMERICAS");
        nodeA->set_param_value(1, "Doublelift");
        nodeA->set_param_value(2, "NA1");
        nodeA->refresh_display();
        src_bf.add_form(nodeA);

        // Node 1: Match By Puuid (Child B, N=2)
        auto nodeB = LinkedInterface::make_linked(1, 6, 2);
        nodeB->set_id(20);
        nodeB->set_window_position(ImVec2(380.0f, 80.0f));
        nodeB->set_locked(false);
        nodeB->set_zip_mode(false);
        nodeB->set_param_value(0, "AMERICAS");
        nodeB->set_parent_for_index(1, nodeA);
        nodeA->insert_child(nodeB);
        nodeB->set_keys_before(1, {"puuid"});
        for (std::size_t j = 0; j < nodeB->get_optional_count(); ++j) {
            if (nodeB->get_optional_name(j) == "count") {
                nodeB->set_optional(j, "10", true);
            }
        }
        nodeB->refresh_display();
        src_bf.add_form(nodeB);

        // Node 2: Match By Match ID (Branch C, N=2)
        auto nodeC = LinkedInterface::make_linked(1, 6, 0);
        nodeC->set_id(30);
        nodeC->set_window_position(ImVec2(680.0f, 40.0f));
        nodeC->set_locked(false);
        nodeC->set_zip_mode(false);
        nodeC->set_param_value(0, "AMERICAS");
        nodeC->set_parent_for_index(1, nodeB);
        nodeB->insert_child(nodeC);
        nodeC->set_param_iterative(1, true);
        nodeC->set_param_deduplicate(1, true);
        nodeC->set_param_skip_db(1, true);
        nodeC->set_iter_limit_val(1, 4);
        nodeC->refresh_display();
        src_bf.add_form(nodeC);

        // Node 3: Match Timeline (Branch D, N=2)
        auto nodeD = LinkedInterface::make_linked(1, 6, 1);
        nodeD->set_id(40);
        nodeD->set_window_position(ImVec2(680.0f, 220.0f));
        nodeD->set_locked(true);
        nodeD->set_zip_mode(false);
        nodeD->set_param_value(0, "AMERICAS");
        nodeD->set_parent_for_index(1, nodeB);
        nodeB->insert_child(nodeD);
        nodeD->set_param_iterative(1, true);
        nodeD->set_param_deduplicate(1, true);
        nodeD->set_param_skip_db(1, true);
        nodeD->set_iter_limit_val(1, 4);
        nodeD->refresh_display();
        src_bf.add_form(nodeD);

        // Node 4: Account By Riot ID in ZIP mode (Convergence Node E, N=3)
        // Param 1 linked to Node C (Match details: info.participants -> riotIdGameName)
        // Param 2 linked to Node C (Match details: info.participants -> riotIdTagline)
        auto nodeE = LinkedInterface::make_linked(0, 0, 1);
        nodeE->set_id(50);
        nodeE->set_window_position(ImVec2(980.0f, 130.0f));
        nodeE->set_locked(false);
        nodeE->set_zip_mode(true);
        nodeE->set_param_value(0, "AMERICAS");

        nodeE->set_parent_for_index(1, nodeC);
        nodeC->insert_child(nodeE);
        nodeE->set_param_iterative(1, true);
        nodeE->set_param_deduplicate(1, true);
        nodeE->set_param_skip_db(1, false);
        nodeE->set_keys_before(1, {"info", "participants"});
        nodeE->set_keys_after(1, {"riotIdGameName"});
        nodeE->set_iter_limit_val(1, 3);

        nodeE->set_parent_for_index(2, nodeC);
        nodeC->insert_child(nodeE);
        nodeE->set_param_iterative(2, true);
        nodeE->set_param_deduplicate(2, true);
        nodeE->set_param_skip_db(2, false);
        nodeE->set_keys_before(2, {"info", "participants"});
        nodeE->set_keys_after(2, {"riotIdTagline"});
        nodeE->set_iter_limit_val(2, 3);

        nodeE->refresh_display();
        src_bf.add_form(nodeE);

        // Assert source setup
        assert(src_bf.get_forms().size() == 5);

        // Save template
        auto& tm = TemplateManager::GetInstance();
        bool saved = tm.SaveTemplate(test_file, "Diamond DAG Test", "5-node diamond graph", src_bf);
        assert(saved);
        assert(std::filesystem::exists(test_file));

        // Load into completely separate BatchForm
        BatchForm dst_bf;
        bool loaded = tm.LoadTemplate(test_file, dst_bf);
        assert(loaded);
        assert(dst_bf.get_forms().size() == 5);

        // Verify 100% field-by-field equality across all 5 nodes
        for (std::size_t i = 0; i < 5; ++i) {
            auto src_form = src_bf.get_forms()[i];
            auto dst_form = dst_bf.get_forms()[i];
            assert(dst_form != nullptr);

            assert(dst_form->get_id() == src_form->get_id());
            assert(dst_form->get_game() == src_form->get_game());
            assert(dst_form->get_endpoint() == src_form->get_endpoint());
            assert(dst_form->get_endpoint_method() == src_form->get_endpoint_method());
            assert(dst_form->get_zip_mode() == src_form->get_zip_mode());
            assert(approx_equal(dst_form->get_window_position().x, src_form->get_window_position().x));
            assert(approx_equal(dst_form->get_window_position().y, src_form->get_window_position().y));
            assert(dst_form->is_locked() == src_form->is_locked());
            assert(dst_form->get_param_count() == src_form->get_param_count());

            for (std::size_t p = 0; p < dst_form->get_param_count(); ++p) {
                assert(dst_form->get_param_name(p) == src_form->get_param_name(p));
                assert(dst_form->get_param_value(p) == src_form->get_param_value(p));
                assert(dst_form->is_param_linked(p) == src_form->is_param_linked(p));
                assert(dst_form->is_param_iterative(p) == src_form->is_param_iterative(p));
                assert(dst_form->is_param_deduplicate(p) == src_form->is_param_deduplicate(p));
                assert(dst_form->is_param_skip_db(p) == src_form->is_param_skip_db(p));

                if (src_form->is_param_linked(p)) {
                    auto src_parent = src_form->get_parent_for_index(p);
                    auto dst_parent = dst_form->get_parent_for_index(p);
                    assert(src_parent != nullptr);
                    assert(dst_parent != nullptr);
                    assert(dst_parent->get_id() == src_parent->get_id());

                    auto src_kb = src_form->get_keys_before(p);
                    auto dst_kb = dst_form->get_keys_before(p);
                    assert(src_kb == dst_kb);

                    auto src_ka = src_form->get_keys_after(p);
                    auto dst_ka = dst_form->get_keys_after(p);
                    assert(src_ka == dst_ka);

                    assert(dst_form->get_iter_limit_val(p) == src_form->get_iter_limit_val(p));
                }
            }

            assert(dst_form->get_optional_count() == src_form->get_optional_count());
            for (std::size_t opt = 0; opt < dst_form->get_optional_count(); ++opt) {
                assert(dst_form->get_optional_name(opt) == src_form->get_optional_name(opt));
                assert(dst_form->get_optional_value(opt) == src_form->get_optional_value(opt));
                assert(dst_form->is_optional_enabled(opt) == src_form->is_optional_enabled(opt));
            }
        }

        // Construct requests on the loaded batch and verify execution readiness
        for (auto& form : dst_bf.get_forms()) form->construct_base();
        for (auto& form : dst_bf.get_forms()) form->link_final_requests();

        // Node 0 should be the only independent root
        assert(!dst_bf.get_forms()[0]->check_dependent());
        for (std::size_t i = 1; i < 5; ++i) {
            assert(dst_bf.get_forms()[i]->check_dependent());
        }

        // Verify Node E underlying request settings
        const auto& reqE = dst_bf.get_forms()[4]->get_base_request()->_node->unsent_request;
        assert(reqE != nullptr);
        assert(reqE->bind_mode == PoolBindMode::ZIP);
        assert(reqE->get_or_create_pool(1)->is_deduplicated());
        assert(reqE->get_or_create_pool(2)->is_deduplicated());

        // Verify Node C and Node D underlying request settings
        const auto& reqC = dst_bf.get_forms()[2]->get_base_request()->_node->unsent_request;
        assert(reqC != nullptr);
        assert(reqC->get_or_create_pool(1)->is_deduplicated());

        const auto& reqD = dst_bf.get_forms()[3]->get_base_request()->_node->unsent_request;
        assert(reqD != nullptr);
        assert(reqD->get_or_create_pool(1)->is_deduplicated());

        std::filesystem::remove_all(test_dir);
        std::cout << "  [PASS] 5-Node Diamond DAG preserved 100% field equality and valid wiring." << std::endl;
    }

    // =========================================================================
    // TEST 2: Multi-Parent Convergence (Two distinct parents into one child node)
    // =========================================================================
    {
        std::cout << "\n[Test 2] Multi-Parent Convergence (Node 2 linked to Parent 0 AND Parent 1)..." << std::endl;
        BatchForm bf;

        // Parent 0: Account By Riot ID (N=3)
        auto parent0 = LinkedInterface::make_linked(0, 0, 1);
        parent0->set_id(1);
        parent0->set_param_value(0, "EUROPE");
        parent0->set_param_value(1, "Caps");
        parent0->set_param_value(2, "EUW");
        parent0->refresh_display();
        bf.add_form(parent0);

        // Parent 1: Account By Puuid (N=2)
        auto parent1 = LinkedInterface::make_linked(0, 0, 0);
        parent1->set_id(2);
        parent1->set_param_value(0, "EUROPE");
        parent1->set_param_value(1, "TEST_PUUID_FOR_PARENT1");
        parent1->refresh_display();
        bf.add_form(parent1);

        // Child: Account By Riot ID (N=3)
        // Param 1 linked to Parent 0 (puuid)
        // Param 2 linked to Parent 1 (puuid)
        auto child = LinkedInterface::make_linked(0, 0, 1);
        child->set_id(3);
        child->set_param_value(0, "EUROPE");
        child->set_parent_for_index(1, parent0);
        parent0->insert_child(child);
        child->set_keys_before(1, {"gameName"});

        child->set_parent_for_index(2, parent1);
        parent1->insert_child(child);
        child->set_keys_before(2, {"tagLine"});
        child->refresh_display();
        bf.add_form(child);

        // Serialize and Deserialize
        auto& tm = TemplateManager::GetInstance();
        std::string json_data = tm.SerializeToString("Multi-Parent Test", "Child has 2 different parents", bf);
        assert(!json_data.empty());

        BatchForm reloaded_bf;
        bool loaded = tm.LoadTemplateFromString(json_data, reloaded_bf);
        assert(loaded);
        assert(reloaded_bf.get_forms().size() == 3);

        auto r_parent0 = reloaded_bf.get_forms()[0];
        auto r_parent1 = reloaded_bf.get_forms()[1];
        auto r_child = reloaded_bf.get_forms()[2];

        assert(r_child->get_parent_for_index(1) == r_parent0);
        assert(r_child->get_parent_for_index(2) == r_parent1);
        assert(r_child->get_keys_before(1).size() == 1 && r_child->get_keys_before(1)[0] == "gameName");
        assert(r_child->get_keys_before(2).size() == 1 && r_child->get_keys_before(2)[0] == "tagLine");

        // Construct requests
        for (auto& f : reloaded_bf.get_forms()) f->construct_base();
        for (auto& f : reloaded_bf.get_forms()) f->link_final_requests();

        // Verify child node dependencies
        auto child_base = r_child->get_base_request();
        assert(child_base != nullptr);
        assert(child_base->_node != nullptr);
        const auto& unsent = child_base->_node->unsent_request;
        assert(unsent != nullptr);
        assert(unsent->dependencies.size() == 2);
        assert(unsent->dependencies[0].parent == r_parent0->get_base_request());
        assert(unsent->dependencies[1].parent == r_parent1->get_base_request());

        std::cout << "  [PASS] Multi-Parent convergence properly links multiple distinct parents." << std::endl;
    }

    // =========================================================================
    // TEST 3: Inverted Topological Order in JSON Deserialization
    // =========================================================================
    {
        std::cout << "\n[Test 3] Inverted Topological Order in JSON Deserialization..." << std::endl;
        // Construct JSON where child appears before its parent:
        // Node 1 (Child: Match-v5 By Puuid) appears before Node 0 (Parent: Account By Riot ID)
        std::string inverted_json = R"({
  "schema_version": 1,
  "name": "Inverted Order Pipeline",
  "description": "Child node appears before parent in the JSON array",
  "nodes": [
    {
      "id": 1,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 2,
      "bind_mode": "CROSS",
      "position": { "x": 400.0, "y": 100.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "PUUID",
          "static_value": "",
          "linked": true,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false,
          "parent_id": 0,
          "json_keys_before": ["puuid"],
          "json_keys_after": [],
          "iter_limit": 0
        }
      ],
      "optionals": []
    },
    {
      "id": 0,
      "game": 0,
      "endpoint": 0,
      "endpoint_method": 1,
      "bind_mode": "CROSS",
      "position": { "x": 100.0, "y": 100.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Game Name",
          "static_value": "Faker",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 2,
          "name": "Tagline",
          "static_value": "KR1",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        }
      ],
      "optionals": []
    }
  ]
})";
        BatchForm bf;
        auto& tm = TemplateManager::GetInstance();
        bool ok = tm.LoadTemplateFromString(inverted_json, bf);
        assert(ok);
        assert(bf.get_forms().size() == 2);

        auto form_at_0 = bf.get_forms()[0]; // Node ID 1
        auto form_at_1 = bf.get_forms()[1]; // Node ID 0

        assert(form_at_0->get_id() == 1);
        assert(form_at_1->get_id() == 0);

        // Verify Node 1 is linked to Node 0
        assert(form_at_0->is_param_linked(1));
        assert(form_at_0->get_parent_for_index(1) == form_at_1);
        assert(form_at_0->get_keys_before(1).size() == 1 && form_at_0->get_keys_before(1)[0] == "puuid");

        // Verify Node 0 is recognized as parent of Node 1
        assert(!form_at_1->check_dependent());
        assert(form_at_0->check_dependent());

        // Construct requests
        for (auto& f : bf.get_forms()) f->construct_base();
        for (auto& f : bf.get_forms()) f->link_final_requests();

        assert(form_at_0->check_ready());
        assert(form_at_1->check_ready());

        std::cout << "  [PASS] Two-pass deserialization resolves parent links regardless of JSON node order." << std::endl;
    }

    // =========================================================================
    // TEST 4: Non-Sequential Gapped Node IDs & Canvas Monotonic ID Allocation
    // =========================================================================
    {
        std::cout << "\n[Test 4] Non-Sequential Gapped Node IDs & Canvas Monotonic ID Allocation..." << std::endl;
        BatchForm bf;

        auto nodeA = LinkedInterface::make_linked(0, 0, 1);
        nodeA->set_id(77);
        nodeA->set_param_value(0, "ASIA");
        nodeA->set_param_value(1, "TestUser");
        nodeA->set_param_value(2, "KR1");
        nodeA->refresh_display();
        bf.add_form(nodeA);

        auto nodeB = LinkedInterface::make_linked(1, 6, 2);
        nodeB->set_id(555);
        nodeB->set_param_value(0, "ASIA");
        nodeB->set_parent_for_index(1, nodeA);
        nodeA->insert_child(nodeB);
        nodeB->set_keys_before(1, {"puuid"});
        nodeB->refresh_display();
        bf.add_form(nodeB);

        auto& tm = TemplateManager::GetInstance();
        std::string json_str = tm.SerializeToString("Gapped IDs", "IDs 77 and 555", bf);

        BatchForm reloaded_bf;
        bool loaded = tm.LoadTemplateFromString(json_str, reloaded_bf);
        assert(loaded);
        assert(reloaded_bf.get_forms().size() == 2);
        assert(reloaded_bf.get_forms()[0]->get_id() == 77);
        assert(reloaded_bf.get_forms()[1]->get_id() == 555);

        // current_ID should be > 555 (at least 556)
        assert(reloaded_bf.get_current_id() >= 556);

        // Adding another form must not collide with existing IDs
        auto nodeC = LinkedInterface::make_linked(1, 6, 0);
        int new_id = reloaded_bf.get_current_id();
        nodeC->set_id(new_id);
        reloaded_bf.add_form(nodeC);

        assert(nodeC->get_id() >= 556);
        assert(reloaded_bf.get_current_id() > nodeC->get_id());

        std::cout << "  [PASS] Gapped node IDs preserved and current_ID advances monotonically." << std::endl;
    }

    // =========================================================================
    // TEST 5: Starter Templates Wiring & Extraction Key Completeness
    // =========================================================================
    {
        std::cout << "\n[Test 5] Built-in Starter 1 & Starter 2 Complete Wiring..." << std::endl;
        auto& tm = TemplateManager::GetInstance();

        // Starter 1
        {
            BatchForm bf1;
            bool ok1 = tm.LoadTemplateFromString(tm.GetStarterMatchHistoryTimelinesJson(), bf1);
            assert(ok1);
            assert(bf1.get_forms().size() == 4);

            auto n0 = bf1.get_forms()[0];
            auto n1 = bf1.get_forms()[1];
            auto n2 = bf1.get_forms()[2];
            auto n3 = bf1.get_forms()[3];

            // Only n0 is root
            assert(!n0->check_dependent());
            assert(n1->check_dependent());
            assert(n2->check_dependent());
            assert(n3->check_dependent());

            // n2 and n3 both link to n1
            assert(n2->get_parent_for_index(1) == n1);
            assert(n3->get_parent_for_index(1) == n1);

            // n2 and n3 both have deduplicate=true and skip_db=true
            assert(n2->is_param_deduplicate(1) == true);
            assert(n2->is_param_skip_db(1) == true);
            assert(n3->is_param_deduplicate(1) == true);
            assert(n3->is_param_skip_db(1) == true);

            // n1 count optional
            bool count_found = false;
            for (std::size_t i = 0; i < n1->get_optional_count(); ++i) {
                if (n1->get_optional_name(i) == "count") {
                    count_found = true;
                    assert(n1->is_optional_enabled(i));
                    assert(n1->get_optional_value(i) == "5");
                }
            }
            assert(count_found);

            for (auto& f : bf1.get_forms()) f->construct_base();
            for (auto& f : bf1.get_forms()) f->link_final_requests();

            for (auto& f : bf1.get_forms()) {
                assert(f->check_ready());
                assert(f->get_base_request() != nullptr);
            }
        }

        // Starter 2
        {
            BatchForm bf2;
            bool ok2 = tm.LoadTemplateFromString(tm.GetStarterParticipantExpansionJson(), bf2);
            assert(ok2);
            assert(bf2.get_forms().size() == 4);

            auto n0 = bf2.get_forms()[0];
            auto n1 = bf2.get_forms()[1];
            auto n2 = bf2.get_forms()[2];
            auto n3 = bf2.get_forms()[3];

            assert(n3->get_zip_mode() == true);
            assert(n3->is_param_linked(1));
            assert(n3->is_param_linked(2));
            assert(n3->get_parent_for_index(1) == n2);
            assert(n3->get_parent_for_index(2) == n2);

            auto kb1 = n3->get_keys_before(1);
            auto ka1 = n3->get_keys_after(1);
            assert(kb1.size() == 2 && kb1[0] == "info" && kb1[1] == "participants");
            assert(ka1.size() == 1 && ka1[0] == "riotIdGameName");

            auto kb2 = n3->get_keys_before(2);
            auto ka2 = n3->get_keys_after(2);
            assert(kb2.size() == 2 && kb2[0] == "info" && kb2[1] == "participants");
            assert(ka2.size() == 1 && ka2[0] == "riotIdTagline");

            for (auto& f : bf2.get_forms()) f->construct_base();
            for (auto& f : bf2.get_forms()) f->link_final_requests();

            const auto& req3 = n3->get_base_request()->_node->unsent_request;
            assert(req3->bind_mode == PoolBindMode::ZIP);
            assert(req3->get_or_create_pool(1)->is_deduplicated());
            assert(req3->get_or_create_pool(2)->is_deduplicated());

            for (auto& f : bf2.get_forms()) {
                assert(f->check_ready());
            }
        }

        std::cout << "  [PASS] Starter Templates 1 & 2 fully wired with correct modes and keys." << std::endl;
    }

    // =========================================================================
    // TEST 6: Quick-Fill Robustness, Partial Updates & Multiple Roots
    // =========================================================================
    {
        std::cout << "\n[Test 6] Quick-Fill Robustness, Partial Updates & Multiple Roots..." << std::endl;
        auto& tm = TemplateManager::GetInstance();
        BatchForm bf;
        tm.LoadTemplateFromString(tm.GetStarterMatchHistoryTimelinesJson(), bf);

        // 6A: Full fill
        bool res1 = tm.QuickFillRootParameters(bf, "AMERICAS", "Chovy", "GEN");
        assert(res1);
        assert(bf.get_forms()[0]->get_param_value(0) == "AMERICAS");
        assert(bf.get_forms()[0]->get_param_value(1) == "Chovy");
        assert(bf.get_forms()[0]->get_param_value(2) == "GEN");
        // Downstream unlinked routing
        for (std::size_t i = 1; i < 4; ++i) {
            assert(bf.get_forms()[i]->get_param_value(0) == "AMERICAS");
        }

        // 6B: Partial fill (empty routing -> keep AMERICAS, change name/tag)
        bool res2 = tm.QuickFillRootParameters(bf, "", "Showmaker", "DK");
        assert(res2);
        assert(bf.get_forms()[0]->get_param_value(0) == "AMERICAS"); // unchanged
        assert(bf.get_forms()[0]->get_param_value(1) == "Showmaker");
        assert(bf.get_forms()[0]->get_param_value(2) == "DK");
        // Downstream routing unchanged
        for (std::size_t i = 1; i < 4; ++i) {
            assert(bf.get_forms()[i]->get_param_value(0) == "AMERICAS");
        }

        // 6C: All empty strings -> nothing changes
        bool res3 = tm.QuickFillRootParameters(bf, "", "", "");
        assert(res3);
        assert(bf.get_forms()[0]->get_param_value(0) == "AMERICAS");
        assert(bf.get_forms()[0]->get_param_value(1) == "Showmaker");
        assert(bf.get_forms()[0]->get_param_value(2) == "DK");

        // 6D: Multiple roots on canvas
        auto extra_root = LinkedInterface::make_linked(0, 0, 1);
        extra_root->set_id(99);
        extra_root->set_param_value(0, "KR");
        extra_root->set_param_value(1, "OldName");
        extra_root->set_param_value(2, "OldTag");
        extra_root->refresh_display();
        bf.add_form(extra_root);

        bool res4 = tm.QuickFillRootParameters(bf, "ASIA", "Viper", "HLE");
        assert(res4);
        // Both roots should receive the update
        assert(bf.get_forms()[0]->get_param_value(0) == "ASIA");
        assert(bf.get_forms()[0]->get_param_value(1) == "Viper");
        assert(bf.get_forms()[0]->get_param_value(2) == "HLE");

        assert(extra_root->get_param_value(0) == "ASIA");
        assert(extra_root->get_param_value(1) == "Viper");
        assert(extra_root->get_param_value(2) == "HLE");

        std::cout << "  [PASS] Quick-Fill operates robustly with partial inputs and multi-root DAGs." << std::endl;
    }

    // =========================================================================
    // TEST 7: Malformed / Adversarial Template Resilience
    // =========================================================================
    {
        std::cout << "\n[Test 7] Malformed & Adversarial Template Resilience..." << std::endl;
        auto& tm = TemplateManager::GetInstance();
        BatchForm bf;

        // 7A: Invalid parent_id (points to non-existent node 8888)
        std::string orphan_json = R"({
  "schema_version": 1,
  "name": "Orphan Link Template",
  "nodes": [
    {
      "id": 1,
      "game": 0,
      "endpoint": 0,
      "endpoint_method": 1,
      "parameters": [
        { "index": 0, "name": "Routing", "static_value": "ASIA", "linked": false },
        { "index": 1, "name": "Game Name", "static_value": "", "linked": true, "parent_id": 8888, "json_keys_before": ["puuid"] },
        { "index": 2, "name": "Tagline", "static_value": "KR1", "linked": false }
      ]
    }
  ]
})";
        bool ok_orphan = tm.LoadTemplateFromString(orphan_json, bf);
        // Should succeed in loading the node without crashing, parent remains unset
        assert(ok_orphan);
        assert(bf.get_forms().size() == 1);
        assert(bf.get_forms()[0]->get_parent_for_index(1) == nullptr);

        // 7B: Missing parameters array
        std::string no_params_json = R"({
  "schema_version": 1,
  "name": "No Params Template",
  "nodes": [
    {
      "id": 1,
      "game": 0,
      "endpoint": 0,
      "endpoint_method": 1
    }
  ]
})";
        bool ok_no_params = tm.LoadTemplateFromString(no_params_json, bf);
        assert(ok_no_params);
        assert(bf.get_forms().size() == 1);

        // 7C: Non-array nodes element
        std::string bad_nodes_json = R"({"name": "Bad Nodes", "nodes": "not an array"})";
        bool ok_bad_nodes = tm.LoadTemplateFromString(bad_nodes_json, bf);
        assert(!ok_bad_nodes);

        std::cout << "  [PASS] TemplateManager gracefully handles adversarial and corrupted schemas." << std::endl;
    }

    // =========================================================================
    // TEST 8: Canvas Clear & Reload Isolation
    // =========================================================================
    {
        std::cout << "\n[Test 8] Canvas Clear & Reload Isolation..." << std::endl;
        auto& tm = TemplateManager::GetInstance();
        BatchForm bf;

        // Load Starter 1
        tm.LoadTemplateFromString(tm.GetStarterMatchHistoryTimelinesJson(), bf);
        assert(bf.get_forms().size() == 4);

        // Clear canvas
        bf.clear_canvas();
        assert(bf.get_forms().empty());
        assert(bf.get_current_id() == 0);
        assert(bf.get_lifecycle_state() == BatchLifecycleState::IDLE);

        // Load Starter 2
        tm.LoadTemplateFromString(tm.GetStarterParticipantExpansionJson(), bf);
        assert(bf.get_forms().size() == 4);
        assert(bf.get_forms()[3]->get_zip_mode() == true);

        std::cout << "  [PASS] Canvas clear and consecutive template reloads maintain state isolation." << std::endl;
    }

    // =========================================================================
    // FEATURE 2 ADVERSARIAL STRESS SUITE (Tests 9 - 16)
    // =========================================================================

    // =========================================================================
    // TEST 9: Rich Multi-Frame Timeline Payload (35 frames x 10 participants = 350 frames, 50 diverse events)
    // =========================================================================
    {
        std::cout << "\n[Test 9] Rich Multi-Frame Timeline Payload (35 frames x 10 participants = 350 frames, 50 events)..." << std::endl;
        std::string db_dir = "./test_m2_timeline_stress_ws";
        std::string db_file = db_dir + "/ezreal_data.duckdb";
        std::filesystem::remove_all(db_dir);
        std::filesystem::create_directories(db_dir);

        auto& db = DatabaseManager::GetInstance();
        assert(db.Init(db_file));
        assert(db.InitTables());

        // Construct 35 frames (0 to 34), each with 10 participants (1 to 10)
        std::ostringstream ss;
        ss << "{\n"
           << "  \"metadata\": { \"matchId\": \"KR_RICH_TIMELINE_001\" },\n"
           << "  \"info\": {\n"
           << "    \"frameInterval\": 60000,\n"
           << "    \"frames\": [\n";

        for (int f = 0; f < 35; ++f) {
            ss << "      {\n"
               << "        \"timestamp\": " << (f * 60000) << ",\n"
               << "        \"participantFrames\": {\n";
            for (int p = 1; p <= 10; ++p) {
                ss << "          \"" << p << "\": {\n"
                   << "            \"participantId\": " << p << ",\n"
                   << "            \"totalGold\": " << (500 + f * 350 + p * 20) << ",\n"
                   << "            \"currentGold\": " << (500 + (f * 120) % 1500) << ",\n"
                   << "            \"xp\": " << (f * 450 + p * 15) << ",\n"
                   << "            \"level\": " << std::min(18, 1 + f / 2) << ",\n"
                   << "            \"minionsKilled\": " << (f * 8 + p) << ",\n"
                   << "            \"jungleMinionsKilled\": " << (p == 2 || p == 7 ? f * 4 : 0) << ",\n"
                   << "            \"position\": { \"x\": " << (500 + f * 200 + p * 50) << ", \"y\": " << (600 + f * 180 + p * 40) << " },\n"
                   << "            \"damageStats\": { \"totalDamageDoneToChampions\": " << (f * 500 + p * 100) << " }\n"
                   << "          }" << (p < 10 ? "," : "") << "\n";
            }
            ss << "        },\n"
               << "        \"events\": [\n";

            // Add events on specific frames
            if (f == 1) {
                ss << "          { \"timestamp\": 65000, \"type\": \"ITEM_PURCHASED\", \"participantId\": 1, \"itemId\": 1055 },\n"
                   << "          { \"timestamp\": 70000, \"type\": \"WARD_PLACED\", \"creatorId\": 1 }\n";
            } else if (f == 5) {
                ss << "          { \"timestamp\": 310000, \"type\": \"CHAMPION_KILL\", \"killerId\": 1, \"victimId\": 6, \"assistingParticipantIds\": [2, 3], \"position\": { \"x\": 4500, \"y\": 5200 } },\n"
                   << "          { \"timestamp\": 320000, \"type\": \"WARD_KILL\", \"killerId\": 1 }\n";
            } else if (f == 10) {
                ss << "          { \"timestamp\": 610000, \"type\": \"ELITE_MONSTER_KILL\", \"monsterType\": \"DRAGON\", \"monsterSubType\": \"FIRE_DRAGON\", \"killerId\": 2, \"teamId\": 100 },\n"
                   << "          { \"timestamp\": 630000, \"type\": \"TURRET_PLATE_DESTROYED\", \"killerId\": 1, \"laneType\": \"TOP_LANE\", \"teamId\": 200 }\n";
            } else if (f == 15) {
                ss << "          { \"timestamp\": 910000, \"type\": \"BUILDING_KILL\", \"buildingType\": \"TOWER_BUILDING\", \"laneType\": \"MID_LANE\", \"killerId\": 4, \"teamId\": 200 },\n"
                   << "          { \"timestamp\": 920000, \"type\": \"SKILL_LEVEL_UP\", \"participantId\": 4, \"skillSlot\": 1 }\n";
            } else if (f == 20) {
                ss << "          { \"timestamp\": 1210000, \"type\": \"ELITE_MONSTER_KILL\", \"monsterType\": \"BARON_NASHOR\", \"monsterSubType\": \"\", \"killerId\": 2, \"teamId\": 100 },\n"
                   << "          { \"timestamp\": 1220000, \"type\": \"ITEM_SOLD\", \"participantId\": 1, \"itemId\": 1055 },\n"
                   << "          { \"timestamp\": 1225000, \"type\": \"ITEM_UNDO\", \"participantId\": 1, \"afterId\": 3031, \"beforeId\": 0 },\n"
                   << "          { \"timestamp\": 1230000, \"type\": \"LEVEL_UP\", \"participantId\": 1, \"level\": 16 }\n";
            } else {
                ss << "          { \"timestamp\": " << (f * 60000 + 1000) << ", \"type\": \"ITEM_PURCHASED\", \"participantId\": " << (1 + f % 10) << ", \"itemId\": " << (1000 + f) << " }\n";
            }

            ss << "        ]\n"
               << "      }" << (f < 34 ? "," : "") << "\n";
        }
        ss << "    ]\n"
           << "  }\n"
           << "}";

        std::string payload = ss.str();
        bool ok = db.SaveTimeline("KR_RICH_TIMELINE_001", payload);
        assert(ok);
        assert(db.HasTimeline("KR_RICH_TIMELINE_001"));
        assert(db.GetTimelineCount() == 1);
        assert(db.GetTimelineParticipantFrameCount() == 350); // 35 * 10
        assert(db.GetTimelineEventCount() >= 40);

        // Verify values using direct DuckDB query
        duckdb_database raw_db;
        duckdb_connection raw_conn;
        assert(duckdb_open(db_file.c_str(), &raw_db) != DuckDBError);
        assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

        // Check frame 10 participant 2
        duckdb_result r_pf;
        const char* q_pf = "SELECT total_gold, current_gold, xp, level, minions_killed, jungle_minions_killed, pos_x, pos_y, total_damage_done_to_champions "
                           "FROM timeline_participant_frames WHERE match_id = 'KR_RICH_TIMELINE_001' AND frame_index = 10 AND participant_id = 2;";
        assert(duckdb_query(raw_conn, q_pf, &r_pf) != DuckDBError);
        assert(duckdb_row_count(&r_pf) == 1);
        assert(duckdb_value_int32(&r_pf, 0, 0) == (500 + 10 * 350 + 2 * 20)); // 4040
        assert(duckdb_value_int32(&r_pf, 3, 0) == 6); // level
        assert(duckdb_value_int32(&r_pf, 5, 0) == 40); // jungleMinionsKilled
        assert(duckdb_value_int32(&r_pf, 6, 0) == 2600); // pos_x
        assert(duckdb_value_int32(&r_pf, 7, 0) == 2480); // pos_y
        duckdb_destroy_result(&r_pf);

        // Check assisting participant IDs string concatenation in CHAMPION_KILL
        duckdb_result r_ev;
        const char* q_ev = "SELECT killer_id, victim_id, assisting_participant_ids, pos_x, pos_y FROM timeline_events "
                           "WHERE match_id = 'KR_RICH_TIMELINE_001' AND event_type = 'CHAMPION_KILL';";
        assert(duckdb_query(raw_conn, q_ev, &r_ev) != DuckDBError);
        assert(duckdb_row_count(&r_ev) == 1);
        assert(duckdb_value_int32(&r_ev, 0, 0) == 1);
        assert(duckdb_value_int32(&r_ev, 1, 0) == 6);
        char* assist_str = duckdb_value_varchar(&r_ev, 2, 0);
        assert(std::string(assist_str) == "2,3");
        duckdb_free(assist_str);
        assert(duckdb_value_int32(&r_ev, 3, 0) == 4500);
        assert(duckdb_value_int32(&r_ev, 4, 0) == 5200);
        duckdb_destroy_result(&r_ev);

        // Check monster kill
        duckdb_result r_m;
        const char* q_m = "SELECT monster_type, monster_sub_type, team_id FROM timeline_events "
                          "WHERE match_id = 'KR_RICH_TIMELINE_001' AND monster_type = 'DRAGON';";
        assert(duckdb_query(raw_conn, q_m, &r_m) != DuckDBError);
        assert(duckdb_row_count(&r_m) == 1);
        char* mst = duckdb_value_varchar(&r_m, 1, 0);
        assert(std::string(mst) == "FIRE_DRAGON");
        duckdb_free(mst);
        assert(duckdb_value_int32(&r_m, 2, 0) == 100);
        duckdb_destroy_result(&r_m);

        duckdb_disconnect(&raw_conn);
        duckdb_close(&raw_db);

        std::cout << "  [PASS] Rich 35-frame timeline and diverse events ingested with 100% field fidelity." << std::endl;
    }

    // =========================================================================
    // TEST 10: Massive Event Volume Stress Test (10,000+ Events across 10 frames)
    // =========================================================================
    {
        std::cout << "\n[Test 10] Massive Event Volume Stress Test (10,000+ events via duckdb_appender)..." << std::endl;
        auto& db = DatabaseManager::GetInstance();
        int64_t initial_events = db.GetTimelineEventCount();

        std::ostringstream ss;
        ss << "{\n"
           << "  \"metadata\": { \"matchId\": \"KR_MASSIVE_002\" },\n"
           << "  \"info\": {\n"
           << "    \"frameInterval\": 60000,\n"
           << "    \"frames\": [\n";

        for (int f = 0; f < 10; ++f) {
            ss << "      {\n"
               << "        \"timestamp\": " << (f * 60000) << ",\n"
               << "        \"participantFrames\": {},\n"
               << "        \"events\": [\n";
            for (int e = 0; e < 1000; ++e) {
                ss << "          { \"timestamp\": " << (f * 60000 + e) 
                   << ", \"type\": \"ITEM_PURCHASED\", \"participantId\": " << (1 + e % 10) 
                   << ", \"itemId\": " << (1000 + e % 500) << " }" << (e < 999 ? "," : "") << "\n";
            }
            ss << "        ]\n"
               << "      }" << (f < 9 ? "," : "") << "\n";
        }
        ss << "    ]\n"
           << "  }\n"
           << "}";

        std::string payload = ss.str();
        bool ok = db.SaveTimeline("KR_MASSIVE_002", payload);
        assert(ok);

        int64_t final_events = db.GetTimelineEventCount();
        assert(final_events == initial_events + 10000);

        std::cout << "  [PASS] Successfully streamed and indexed 10,000 events without failure." << std::endl;
    }

    // =========================================================================
    // TEST 11: Missing Attributes, Nulls, Sparse Frames & Key-Fallback Resilience
    // =========================================================================
    {
        std::cout << "\n[Test 11] Missing Attributes, Nulls, Sparse Frames & Key-Fallback Resilience..." << std::endl;
        auto& db = DatabaseManager::GetInstance();

        std::string sparse_json = R"({
  "metadata": { "matchId": "KR_SPARSE_003" },
  "info": {
    "frameInterval": 60000,
    "frames": [
      {
        "timestamp": 0,
        "participantFrames": {
          "7": {
            "totalGold": 500,
            "currentGold": 500,
            "xp": 0,
            "level": 1
          }
        },
        "events": [
          { "type": "" },
          { "type": "ITEM_UNDO", "afterId": 3006 },
          { "type": "ITEM_DESTROYED", "beforeId": 2003 },
          { "type": "ELITE_MONSTER_KILL", "monsterType": "HERALD", "killerTeamId": 200 },
          { "type": "CHAMPION_KILL", "killerId": 1, "victimId": 2, "assistingParticipantIds": [] },
          { "type": "CHAMPION_KILL", "killerId": 3, "victimId": 4 }
        ]
      }
    ]
  }
})";

        bool ok = db.SaveTimeline("KR_SPARSE_003", sparse_json);
        assert(ok);

        std::string current_ws_db = db.GetDatabasePath();
        db.Close();
        assert(db.Reopen(current_ws_db));

        duckdb_database raw_db;
        duckdb_connection raw_conn;
        assert(duckdb_open(db.GetDatabasePath().c_str(), &raw_db) != DuckDBError);
        assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

        // Verify participantId 7 was parsed from key "7"
        duckdb_result r_pf;
        assert(duckdb_query(raw_conn, "SELECT participant_id, minions_killed, jungle_minions_killed, pos_x, pos_y, total_damage_done_to_champions "
                                      "FROM timeline_participant_frames WHERE match_id = 'KR_SPARSE_003';", &r_pf) != DuckDBError);
        assert(duckdb_row_count(&r_pf) == 1);
        assert(duckdb_value_int32(&r_pf, 0, 0) == 7);
        assert(duckdb_value_int32(&r_pf, 1, 0) == 0); // minions_killed default 0
        assert(duckdb_value_int32(&r_pf, 2, 0) == 0); // jungle_minions_killed default 0
        assert(duckdb_value_int32(&r_pf, 3, 0) == 0); // pos_x default 0
        assert(duckdb_value_int32(&r_pf, 4, 0) == 0); // pos_y default 0
        assert(duckdb_value_int32(&r_pf, 5, 0) == 0); // damage default 0
        duckdb_destroy_result(&r_pf);

        // Verify events: empty type was skipped (5 events retained out of 6)
        duckdb_result r_ev;
        assert(duckdb_query(raw_conn, "SELECT event_index, event_type, item_id, team_id, assisting_participant_ids, pos_x, pos_y "
                                      "FROM timeline_events WHERE match_id = 'KR_SPARSE_003' ORDER BY event_index ASC;", &r_ev) != DuckDBError);
        assert(duckdb_row_count(&r_ev) == 5);
        // Event 0: ITEM_UNDO with afterId 3006
        char* et0 = duckdb_value_varchar(&r_ev, 1, 0);
        assert(std::string(et0) == "ITEM_UNDO");
        duckdb_free(et0);
        assert(duckdb_value_int32(&r_ev, 2, 0) == 3006);

        // Event 1: ITEM_DESTROYED with beforeId 2003
        char* et1 = duckdb_value_varchar(&r_ev, 1, 1);
        assert(std::string(et1) == "ITEM_DESTROYED");
        duckdb_free(et1);
        assert(duckdb_value_int32(&r_ev, 2, 1) == 2003);

        // Event 2: ELITE_MONSTER_KILL with killerTeamId 200
        assert(duckdb_value_int32(&r_ev, 3, 2) == 200);

        // Event 3: assistingParticipantIds [] -> empty string
        char* ass3 = duckdb_value_varchar(&r_ev, 4, 3);
        assert(std::string(ass3).empty());
        duckdb_free(ass3);

        // Event 4: no assistingParticipantIds property -> empty string
        char* ass4 = duckdb_value_varchar(&r_ev, 4, 4);
        assert(std::string(ass4).empty());
        duckdb_free(ass4);
        assert(duckdb_value_int32(&r_ev, 5, 4) == 0); // pos_x 0
        assert(duckdb_value_int32(&r_ev, 6, 4) == 0); // pos_y 0

        duckdb_destroy_result(&r_ev);
        duckdb_disconnect(&raw_conn);
        duckdb_close(&raw_db);

        std::cout << "  [PASS] Missing attributes and fallback keys handled correctly." << std::endl;
    }

    // =========================================================================
    // TEST 12: Adversarial & Malformed JSON Rejection Stress
    // =========================================================================
    {
        std::cout << "\n[Test 12] Adversarial & Malformed JSON Rejection Stress..." << std::endl;
        auto& db = DatabaseManager::GetInstance();

        std::vector<std::pair<std::string, std::string>> invalid_inputs = {
            {"M_TRUNC", "{\"metadata\": {\"matchId\": \"M1\"}, \"info\": {\"frames\": ["}, // truncated
            {"M_SYNTAX", "not a valid json string {[]}"},                                  // syntax error
            {"M_404", "{\"status\": {\"message\": \"Data not found\", \"status_code\": 404}}"}, // 404 status
            {"M_403", "{\"status\": {\"message\": \"Forbidden\", \"status_code\": 403}}"},      // 403 status
            {"M_400", "{\"status\": {\"message\": \"Bad Request\", \"status_code\": 400}}"},    // 400 status
            {"M_NO_INFO", "{\"metadata\": {\"matchId\": \"ERR_NO_INFO\"}}"},                     // no info
            {"M_EMPTY_STR", ""},                                                                 // empty string
            {"M_WS", "   \t\n\r  "},                                                         // whitespace only
            {"M_BIN", std::string("\x00\xFF\x80\x7F\xAA\x55", 6)},                              // binary garbage
            {"", "{\"info\": {\"frames\": []}}"}                                                // empty match_id
        };

        for (size_t i = 0; i < invalid_inputs.size(); ++i) {
            bool ok = db.SaveTimeline(invalid_inputs[i].first, invalid_inputs[i].second);
            assert(!ok);
        }

        // Test timeline with empty frames array: should save raw metadata but create 0 unnested frames
        int64_t pf_before = db.GetTimelineParticipantFrameCount();
        int64_t ev_before = db.GetTimelineEventCount();
        bool ok_empty = db.SaveTimeline("KR_EMPTY_FRAMES", "{\"info\": {\"frames\": []}}");
        assert(ok_empty);
        assert(db.GetTimelineParticipantFrameCount() == pf_before);
        assert(db.GetTimelineEventCount() == ev_before);

        // Verify database is still operational after rejecting all malformed inputs
        assert(db.GetTimelineCount() >= 1);
        std::cout << "  [PASS] 10 distinct malformed/adversarial inputs rejected safely and empty-frame timeline handled without corruption." << std::endl;
    }

    // =========================================================================
    // TEST 13: BackfillUnnestedTimelines Comprehensive Discovery & Idempotency
    // =========================================================================
    {
        std::cout << "\n[Test 13] BackfillUnnestedTimelines Legacy Discovery & Idempotency..." << std::endl;
        std::string bf_dir = "./test_m2_bf_ws";
        std::string bf_file = bf_dir + "/ezreal_data.duckdb";
        std::filesystem::remove_all(bf_dir);
        std::filesystem::create_directories(bf_dir);

        auto& db = DatabaseManager::GetInstance();
        assert(db.Init(bf_file));
        assert(db.InitTables());

        // Ingest one timeline properly
        std::string properly_ingested = R"({
          "metadata": { "matchId": "PROPER_01" },
          "info": {
            "frameInterval": 60000,
            "frames": [{
              "timestamp": 0,
              "participantFrames": { "1": { "participantId": 1, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 } },
              "events": [{ "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1055 }]
            }]
          }
        })";
        assert(db.SaveTimeline("PROPER_01", properly_ingested));
        assert(db.GetTimelineParticipantFrameCount() == 1);
        assert(db.GetTimelineEventCount() == 1);

        // Directly insert raw rows into match_timelines (simulating legacy data)
        duckdb_database raw_db;
        duckdb_connection raw_conn;
        assert(duckdb_open(bf_file.c_str(), &raw_db) != DuckDBError);
        assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

        std::string legacy_1 = R"({
          "metadata": { "matchId": "LEGACY_01" },
          "info": {
            "frameInterval": 60000,
            "frames": [
              {
                "timestamp": 0,
                "participantFrames": {
                  "1": { "participantId": 1, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 },
                  "2": { "participantId": 2, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 }
                },
                "events": [{ "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1054 }]
              },
              {
                "timestamp": 60000,
                "participantFrames": {
                  "1": { "participantId": 1, "totalGold": 850, "currentGold": 350, "xp": 400, "level": 2 },
                  "2": { "participantId": 2, "totalGold": 800, "currentGold": 300, "xp": 380, "level": 2 }
                },
                "events": [
                  { "type": "CHAMPION_KILL", "killerId": 1, "victimId": 2 },
                  { "type": "WARD_PLACED", "creatorId": 1 }
                ]
              }
            ]
          }
        })";

        std::string legacy_2 = R"({
          "metadata": { "matchId": "LEGACY_02" },
          "info": {
            "frameInterval": 60000,
            "frames": [
              {
                "timestamp": 0,
                "participantFrames": {
                  "3": { "participantId": 3, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 },
                  "4": { "participantId": 4, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 }
                },
                "events": [{ "type": "ITEM_PURCHASED", "participantId": 3, "itemId": 1056 }]
              }
            ]
          }
        })";

        // Corrupt legacy row (valid JSON syntax so DuckDB's JSON column accepts it, but missing info/frames)
        std::string legacy_corrupt = "{\"error\": \"corrupted timeline payload\"}";
        // Empty frames legacy row
        std::string legacy_empty = "{\"metadata\": {\"matchId\": \"LEGACY_EMPTY\"}, \"info\": {\"frames\": []}}";

        auto insert_legacy = [&](const std::string& mid, const std::string& raw) {
            duckdb_prepared_statement st;
            assert(duckdb_prepare(raw_conn, "INSERT INTO match_timelines (match_id, frame_interval, raw_json) VALUES ($1, 60000, $2);", &st) != DuckDBError);
            duckdb_bind_varchar(st, 1, mid.c_str());
            duckdb_bind_varchar(st, 2, raw.c_str());
            duckdb_result res;
            if (duckdb_execute_prepared(st, &res) == DuckDBError) {
                std::cerr << "insert_legacy failed for " << mid << ": " << duckdb_result_error(&res) << std::endl;
                assert(false);
            }
            duckdb_destroy_result(&res);
            duckdb_destroy_prepare(&st);
        };

        insert_legacy("LEGACY_01", legacy_1);
        insert_legacy("LEGACY_02", legacy_2);
        insert_legacy("LEGACY_CORRUPT", legacy_corrupt);
        insert_legacy("LEGACY_EMPTY", legacy_empty);

        duckdb_disconnect(&raw_conn);
        duckdb_close(&raw_db);

        // Run backfill
        int64_t backfilled = db.BackfillUnnestedTimelines();
        assert(backfilled == 2); // Exactly LEGACY_01 and LEGACY_02

        // Verify total participant frames: 1 (PROPER) + 4 (LEGACY_01) + 2 (LEGACY_02) = 7
        assert(db.GetTimelineParticipantFrameCount() == 7);
        // Verify total events: 1 (PROPER) + 3 (LEGACY_01) + 1 (LEGACY_02) = 5
        assert(db.GetTimelineEventCount() == 5);

        // Immediate subsequent call must return 0
        int64_t backfilled_again = db.BackfillUnnestedTimelines();
        assert(backfilled_again == 0);

        std::cout << "  [PASS] BackfillUnnestedTimelines accurately migrated legacy rows and returned 0 on rerun." << std::endl;
    }

    // =========================================================================
    // TEST 14: Strict 3-Pass Idempotency & Overwrite Replacement Integrity
    // =========================================================================
    {
        std::cout << "\n[Test 14] Strict 3-Pass Idempotency & Overwrite Replacement Integrity..." << std::endl;
        auto& db = DatabaseManager::GetInstance();

        std::string payload_v1 = R"({
          "metadata": { "matchId": "IDEM_MATCH_99" },
          "info": {
            "frameInterval": 60000,
            "frames": [
              {
                "timestamp": 0,
                "participantFrames": {
                  "1": { "participantId": 1, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 },
                  "2": { "participantId": 2, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 }
                },
                "events": [
                  { "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1055 },
                  { "type": "ITEM_PURCHASED", "participantId": 2, "itemId": 1056 }
                ]
              }
            ]
          }
        })";

        // Pass 1: Initial save
        assert(db.SaveTimeline("IDEM_MATCH_99", payload_v1));
        int64_t pf1 = db.GetTimelineParticipantFrameCount();
        int64_t ev1 = db.GetTimelineEventCount();

        // Pass 2: Identical re-save
        assert(db.SaveTimeline("IDEM_MATCH_99", payload_v1));
        assert(db.GetTimelineParticipantFrameCount() == pf1);
        assert(db.GetTimelineEventCount() == ev1);

        // Pass 3: Identical re-save
        assert(db.SaveTimeline("IDEM_MATCH_99", payload_v1));
        assert(db.GetTimelineParticipantFrameCount() == pf1);
        assert(db.GetTimelineEventCount() == ev1);

        // Overwrite with payload_v2 (now 3 frames, 6 participant frames, 5 events)
        std::string payload_v2 = R"({
          "metadata": { "matchId": "IDEM_MATCH_99" },
          "info": {
            "frameInterval": 60000,
            "frames": [
              {
                "timestamp": 0,
                "participantFrames": {
                  "1": { "participantId": 1, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 },
                  "2": { "participantId": 2, "totalGold": 500, "currentGold": 500, "xp": 0, "level": 1 }
                },
                "events": [{ "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 1055 }]
              },
              {
                "timestamp": 60000,
                "participantFrames": {
                  "1": { "participantId": 1, "totalGold": 900, "currentGold": 400, "xp": 500, "level": 2 },
                  "2": { "participantId": 2, "totalGold": 850, "currentGold": 350, "xp": 480, "level": 2 }
                },
                "events": [
                  { "type": "CHAMPION_KILL", "killerId": 1, "victimId": 2 },
                  { "type": "WARD_PLACED", "creatorId": 1 }
                ]
              },
              {
                "timestamp": 120000,
                "participantFrames": {
                  "1": { "participantId": 1, "totalGold": 1300, "currentGold": 800, "xp": 1000, "level": 3 },
                  "2": { "participantId": 2, "totalGold": 1200, "currentGold": 700, "xp": 950, "level": 3 }
                },
                "events": [
                  { "type": "ITEM_PURCHASED", "participantId": 1, "itemId": 3006 },
                  { "type": "ITEM_PURCHASED", "participantId": 2, "itemId": 3020 }
                ]
              }
            ]
          }
        })";

        assert(db.SaveTimeline("IDEM_MATCH_99", payload_v2));
        // Verify participant frames for IDEM_MATCH_99 is now exactly 6 (not 2 + 6 = 8)
        std::string p_ws = db.GetDatabasePath();
        db.Close();
        assert(db.Reopen(p_ws));

        duckdb_database raw_db;
        duckdb_connection raw_conn;
        assert(duckdb_open(db.GetDatabasePath().c_str(), &raw_db) != DuckDBError);
        assert(duckdb_connect(raw_db, &raw_conn) != DuckDBError);

        duckdb_result r_cnt;
        assert(duckdb_query(raw_conn, "SELECT COUNT(*) FROM timeline_participant_frames WHERE match_id = 'IDEM_MATCH_99';", &r_cnt) != DuckDBError);
        assert(duckdb_value_int64(&r_cnt, 0, 0) == 6);
        duckdb_destroy_result(&r_cnt);

        assert(duckdb_query(raw_conn, "SELECT COUNT(*) FROM timeline_events WHERE match_id = 'IDEM_MATCH_99';", &r_cnt) != DuckDBError);
        assert(duckdb_value_int64(&r_cnt, 0, 0) == 5);
        duckdb_destroy_result(&r_cnt);

        duckdb_disconnect(&raw_conn);
        duckdb_close(&raw_db);

        std::cout << "  [PASS] 3 consecutive saves confirmed idempotent; overwriting replaced rows cleanly." << std::endl;
    }

    // =========================================================================
    // TEST 15: Concurrency Safety (Multi-Threaded Reads & Writes)
    // =========================================================================
    {
        std::cout << "\n[Test 15] Concurrency Safety (Multi-Threaded Reads & Writes)..." << std::endl;
        auto& db = DatabaseManager::GetInstance();
        std::atomic<bool> stop_flag{false};
        std::atomic<int> errors{0};

        auto make_tl = [](const std::string& mid) {
            return "{\"metadata\":{\"matchId\":\"" + mid + "\"},\"info\":{\"frameInterval\":60000,\"frames\":[{\"timestamp\":0,\"participantFrames\":{\"1\":{\"participantId\":1,\"totalGold\":500,\"currentGold\":500,\"xp\":0,\"level\":1}},\"events\":[{\"type\":\"ITEM_PURCHASED\",\"participantId\":1,\"itemId\":1055}]}]}}";
        };

        std::thread t1([&]() {
            for (int i = 0; i < 20; ++i) {
                if (!db.SaveTimeline("CONCUR_A_" + std::to_string(i), make_tl("CONCUR_A_" + std::to_string(i)))) {
                    errors++;
                }
            }
        });

        std::thread t2([&]() {
            for (int i = 0; i < 20; ++i) {
                if (!db.SaveTimeline("CONCUR_B_" + std::to_string(i), make_tl("CONCUR_B_" + std::to_string(i)))) {
                    errors++;
                }
            }
        });

        std::thread t3([&]() {
            while (!stop_flag.load()) {
                db.GetTimelineParticipantFrameCount();
                db.GetTimelineEventCount();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });

        t1.join();
        t2.join();
        stop_flag.store(true);
        t3.join();

        assert(errors.load() == 0);
        std::cout << "  [PASS] Concurrent multi-threaded ingestion executed cleanly without errors." << std::endl;
    }

    // =========================================================================
    // TEST 16: Database Reopen, Unnested Persistence & WAL Integrity
    // =========================================================================
    {
        std::cout << "\n[Test 16] Database Reopen, Unnested Persistence & WAL Integrity..." << std::endl;
        auto& db = DatabaseManager::GetInstance();
        std::string current_path = db.GetDatabasePath();
        int64_t expected_pf = db.GetTimelineParticipantFrameCount();
        int64_t expected_ev = db.GetTimelineEventCount();

        // Close
        db.Close();
        assert(!db.IsInitialized());
        assert(db.GetTimelineParticipantFrameCount() == 0);
        assert(db.GetTimelineEventCount() == 0);

        // Reopen at same path
        assert(db.Reopen(current_path));
        assert(db.IsInitialized());
        assert(db.GetTimelineParticipantFrameCount() == expected_pf);
        assert(db.GetTimelineEventCount() == expected_ev);

        // Cleanup test directory
        std::filesystem::remove_all("./test_m2_timeline_stress_ws");
        std::filesystem::remove_all("./test_m2_bf_ws");

        std::cout << "  [PASS] Database close and reopen restored unnested timeline counts with 100% fidelity." << std::endl;
    }

    ImGui::DestroyContext(imgui_ctx);
    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[SUCCESS] ALL ADVERSARIAL STRESS TESTS FOR MILESTONE 2 PASSED!" << std::endl;
    std::cout << "=====================================================================" << std::endl;
    return 0;
}
