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

    ImGui::DestroyContext(imgui_ctx);
    std::cout << "\n=====================================================================" << std::endl;
    std::cout << "[SUCCESS] ALL ADVERSARIAL STRESS TESTS FOR MILESTONE 2 PASSED!" << std::endl;
    std::cout << "=====================================================================" << std::endl;
    return 0;
}
