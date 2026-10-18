#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <sstream>
#include <filesystem>
#include <iomanip>

#define private public
#define protected public
#include "imgui.h"
#include "QueryForm.h"
#include "BatchRequests.h"
#undef private
#undef protected

using namespace restfulEz;

int main() {
    std::cout << "[Adversarial Test] Starting Empirical Stress-Testing for Milestone 1..." << std::endl;

    // Initialize headless ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext* imgui_ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);

    // =========================================================================
    // TEST 1: Root Node Deletion and Reversion of Child to Unlinked Root
    // =========================================================================
    {
        std::cout << "\n[Test 1] Root Node Deletion in 2-node chain (A -> B)..." << std::endl;
        // Node A: Account By Riot ID (N=3: Routing, Game Name, Tagline)
        auto nodeA = std::dynamic_pointer_cast<LinkedForm<3>>(LinkedInterface::make_linked(0, 0, 1));
        assert(nodeA != nullptr);
        nodeA->set_id(0);
        strncpy(nodeA->_params_in_form[0].param, "ASIA", 255);
        strncpy(nodeA->_params_in_form[1].param, "Faker", 255);
        strncpy(nodeA->_params_in_form[2].param, "KR1", 255);
        nodeA->on_close_popup();
        assert(nodeA->check_ready() == true);
        assert(nodeA->check_dependent() == false);

        // Node B: Account By Puuid (N=2: Routing, Puuid)
        auto nodeB = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(0, 0, 0));
        assert(nodeB != nullptr);
        nodeB->set_id(1);
        strncpy(nodeB->_params_in_form[0].param, "ASIA", 255);
        // Set manual fallback PUUID
        strncpy(nodeB->_params_in_form[1].param, "MANUAL_FALLBACK_PUUID_999", 255);

        // Link B parameter 1 (Puuid) to Node A
        nodeB->next_index = 1;
        nodeB->insert_parent(nodeA);
        nodeA->insert_child(nodeB);
        nodeB->iter_info[0]->keys.push_back(KEY_CONT("puuid"));
        nodeB->on_close_popup();

        // Verify pre-conditions
        assert(nodeB->check_dependent() == true);
        assert(nodeB->linked[1] == true);
        assert(nodeB->parents[0] == nodeA);
        assert(nodeA->children.size() == 1);
        assert(nodeA->children[0] == nodeB);
        assert(nodeB->check_ready() == true);

        // Trigger Root Node Deletion via disconnect_all()
        nodeA->disconnect_all();

        // Verify post-conditions
        assert(nodeA->children.empty() == true);
        assert(nodeB->parents[0] == nullptr);
        assert(nodeB->linked[1] == false);
        assert(nodeB->check_dependent() == false); // Cleanly reverted to independent root node!
        assert(nodeB->iter_info[0]->keys.empty() == true); // Extraction keys cleared
        
        // CRITICAL CHECK: Verify manual parameter inputs intact
        assert(std::string(nodeB->_params_in_form[1].param) == "MANUAL_FALLBACK_PUUID_999");
        assert(nodeB->display_form[1].find("MANUAL_FALLBACK_PUUID_999") != std::string::npos);
        assert(nodeB->check_ready() == true); // Ready using manual input!

        // Verify base request construction on unlinked node B
        nodeB->construct_base();
        assert(nodeB->get_base_request() != nullptr);
        auto& base_b = nodeB->get_base_request()->_node->unsent_request;
        assert(base_b != nullptr);
        assert(base_b->dependencies.empty());
        assert(base_b->iter_dependencies.empty());
        assert(std::string(base_b->params[1].param) == "MANUAL_FALLBACK_PUUID_999");

        std::cout << "  [PASS] Root deletion cleanly unlinked child and preserved manual input." << std::endl;
    }

    // =========================================================================
    // TEST 2: Middle Node Deletion in 3-Node Chain (A -> B -> C)
    // =========================================================================
    {
        std::cout << "\n[Test 2] Middle Node Deletion in 3-Node Chain (A -> B -> C)..." << std::endl;
        // A: Account By Riot ID (N=3)
        auto nodeA = std::dynamic_pointer_cast<LinkedForm<3>>(LinkedInterface::make_linked(0, 0, 1));
        nodeA->set_id(0);
        strncpy(nodeA->_params_in_form[0].param, "ASIA", 255);
        strncpy(nodeA->_params_in_form[1].param, "Faker", 255);
        strncpy(nodeA->_params_in_form[2].param, "KR1", 255);
        nodeA->on_close_popup();

        // B: Match By Puuid (N=2, Game 1, Endpoint 6, Method 2)
        auto nodeB = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(1, 6, 2));
        nodeB->set_id(1);
        strncpy(nodeB->_params_in_form[0].param, "ASIA", 255);
        strncpy(nodeB->_params_in_form[1].param, "MANUAL_PUUID_B", 255);
        nodeB->next_index = 1;
        nodeB->insert_parent(nodeA);
        nodeA->insert_child(nodeB);
        nodeB->iter_info[0]->keys.push_back(KEY_CONT("puuid"));
        nodeB->on_close_popup();

        // C: Match Details By Match ID (N=2, Game 1, Endpoint 6, Method 0)
        auto nodeC = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(1, 6, 0));
        nodeC->set_id(2);
        strncpy(nodeC->_params_in_form[0].param, "ASIA", 255);
        strncpy(nodeC->_params_in_form[1].param, "KR_MANUAL_12345", 255);
        nodeC->next_index = 1;
        nodeC->insert_parent(nodeB);
        nodeB->insert_child(nodeC);
        nodeC->iterative[1] = true;
        strncpy(nodeC->iter_limits[0], "5", 7);
        nodeC->on_close_popup();

        assert(nodeB->check_dependent() == true);
        assert(nodeC->check_dependent() == true);
        assert(nodeA->children.size() == 1);
        assert(nodeB->children.size() == 1);

        // Delete Middle Node B
        nodeB->disconnect_all();

        // Post-deletion verification:
        // 1. A should have no children pointing to B
        assert(nodeA->children.empty() == true);
        // 2. B should have no parents or children
        assert(nodeB->parents[0] == nullptr);
        assert(nodeB->children.empty() == true);
        // 3. C should be completely disconnected from B
        assert(nodeC->parents[0] == nullptr);
        assert(nodeC->linked[1] == false);
        assert(nodeC->iterative[1] == false);
        assert(nodeC->check_dependent() == false);
        // 4. C manual inputs intact
        assert(std::string(nodeC->_params_in_form[1].param) == "KR_MANUAL_12345");
        assert(nodeC->display_form[1].find("KR_MANUAL_12345") != std::string::npos);
        assert(nodeC->check_ready() == true);

        std::cout << "  [PASS] Middle node deletion cleanly disconnected upstream and downstream." << std::endl;
    }

    // =========================================================================
    // TEST 3: Child Node Deletion
    // =========================================================================
    {
        std::cout << "\n[Test 3] Child Node Deletion..." << std::endl;
        auto parent = std::dynamic_pointer_cast<LinkedForm<3>>(LinkedInterface::make_linked(0, 0, 1));
        auto child = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(0, 0, 0));
        parent->set_id(0);
        child->set_id(1);

        child->next_index = 1;
        child->insert_parent(parent);
        parent->insert_child(child);
        assert(parent->children.size() == 1);

        // Delete Child
        child->disconnect_all();
        assert(parent->children.empty() == true);
        assert(child->parents[0] == nullptr);
        assert(child->linked[1] == false);

        std::cout << "  [PASS] Child node deletion safely pruned parent's child list." << std::endl;
    }

    // =========================================================================
    // TEST 4: Diamond DAG Node Deletion
    // =========================================================================
    {
        std::cout << "\n[Test 4] Diamond DAG Node Deletion (A -> B, A -> C, (B, C) -> D)..." << std::endl;
        // Node A: Root
        auto nodeA = std::dynamic_pointer_cast<LinkedForm<3>>(LinkedInterface::make_linked(0, 0, 1));
        nodeA->set_id(0);
        strncpy(nodeA->_params_in_form[0].param, "ASIA", 255);
        strncpy(nodeA->_params_in_form[1].param, "Faker", 255);
        strncpy(nodeA->_params_in_form[2].param, "KR1", 255);
        nodeA->on_close_popup();

        // Node B: Left branch
        auto nodeB = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(1, 6, 2));
        nodeB->set_id(1);
        strncpy(nodeB->_params_in_form[0].param, "ASIA", 255);
        nodeB->next_index = 1;
        nodeB->insert_parent(nodeA);
        nodeA->insert_child(nodeB);
        nodeB->on_close_popup();

        // Node C: Right branch
        auto nodeC = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(1, 6, 2));
        nodeC->set_id(2);
        strncpy(nodeC->_params_in_form[0].param, "ASIA", 255);
        nodeC->next_index = 1;
        nodeC->insert_parent(nodeA);
        nodeA->insert_child(nodeC);
        nodeC->on_close_popup();

        // Node D: Sink (N=3: Routing, Param1 from B, Param2 from C)
        auto nodeD = std::dynamic_pointer_cast<LinkedForm<3>>(LinkedInterface::make_linked(0, 0, 1));
        nodeD->set_id(3);
        strncpy(nodeD->_params_in_form[0].param, "ASIA", 255);
        strncpy(nodeD->_params_in_form[1].param, "FALLBACK_NAME", 255);
        strncpy(nodeD->_params_in_form[2].param, "FALLBACK_TAG", 255);

        // Link D param 1 to B
        nodeD->next_index = 1;
        nodeD->insert_parent(nodeB);
        nodeB->insert_child(nodeD);

        // Link D param 2 to C
        nodeD->next_index = 2;
        nodeD->insert_parent(nodeC);
        nodeC->insert_child(nodeD);

        nodeD->on_close_popup();

        assert(nodeD->linked[1] == true);
        assert(nodeD->linked[2] == true);
        assert(nodeD->parents[0] == nodeB);
        assert(nodeD->parents[1] == nodeC);
        assert(nodeB->children.size() == 1);
        assert(nodeC->children.size() == 1);
        assert(nodeA->children.size() == 2);

        // Case 4a: Delete Node B (Left parent of D)
        nodeB->disconnect_all();

        // Post-condition:
        // B is disconnected
        assert(nodeA->children.size() == 1);
        assert(nodeA->children[0] == nodeC);
        assert(nodeB->children.empty() == true);
        
        // D's slot 1 (from B) is unlinked and restored
        assert(nodeD->parents[0] == nullptr);
        assert(nodeD->linked[1] == false);
        assert(std::string(nodeD->_params_in_form[1].param) == "FALLBACK_NAME");

        // D's slot 2 (from C) REMAINS LINKED and valid!
        assert(nodeD->parents[1] == nodeC);
        assert(nodeD->linked[2] == true);
        assert(nodeC->children.size() == 1);
        assert(nodeC->children[0] == nodeD);
        assert(nodeD->check_dependent() == true); // Still dependent on C!

        // Case 4b: Now delete Node C (Right parent of D)
        nodeC->disconnect_all();

        // Now D should have NO parents left
        assert(nodeD->parents[1] == nullptr);
        assert(nodeD->linked[2] == false);
        assert(nodeD->check_dependent() == false); // D cleanly becomes root!
        assert(std::string(nodeD->_params_in_form[2].param) == "FALLBACK_TAG");
        assert(nodeD->check_ready() == true);

        std::cout << "  [PASS] Diamond DAG node deletion preserved remaining links and cleanly detached." << std::endl;
    }

    // =========================================================================
    // TEST 5: Canvas Clear (0 nodes, 1 node, and 10 linked nodes)
    // =========================================================================
    {
        std::cout << "\n[Test 5] Canvas Clear Verification..." << std::endl;

        // 5a. Clear canvas with 0 nodes
        BatchForm bf0;
        assert(bf0.get_forms().empty());
        assert(bf0.get_lifecycle_state() == BatchLifecycleState::IDLE);
        bf0.clear_canvas();
        assert(bf0.get_forms().empty());
        assert(bf0.get_lifecycle_state() == BatchLifecycleState::IDLE);
        std::cout << "  [PASS] Clear canvas with 0 nodes succeeded." << std::endl;

        // 5b. Clear canvas with 1 node
        BatchForm bf1;
        bf1.forms.push_back(LinkedInterface::make_linked(0, 0, 1));
        bf1.current_ID = 1;
        assert(bf1.get_forms().size() == 1);
        bf1.clear_canvas();
        assert(bf1.get_forms().empty());
        assert(bf1.current_ID == 0);
        assert(bf1.get_lifecycle_state() == BatchLifecycleState::IDLE);
        std::cout << "  [PASS] Clear canvas with 1 node succeeded." << std::endl;

        // 5c. Clear canvas with 10 linked nodes in a complex graph
        BatchForm bf10;
        struct EndpointSpec { int game; int ep; int method; };
        std::vector<EndpointSpec> specs = {
            {0, 0, 1}, // 0: Account By Riot ID
            {0, 0, 0}, // 1: Account By Puuid
            {0, 0, 2}, // 2: Account By Game
            {1, 6, 2}, // 3: Match By Puuid
            {1, 6, 0}, // 4: Match By Match ID
            {1, 6, 1}, // 5: Timeline
            {1, 6, 2}, // 6: Match By Puuid
            {1, 6, 0}, // 7: Match By Match ID
            {1, 6, 1}, // 8: Timeline
            {0, 0, 1}  // 9: Account By Riot ID
        };
        for (int i = 0; i < 10; ++i) {
            auto form = LinkedInterface::make_linked(specs[i].game, specs[i].ep, specs[i].method);
            form->set_id(i);
            bf10.forms.push_back(form);
            bf10.current_ID++;
        }
        assert(bf10.forms.size() == 10);
        assert(bf10.current_ID == 10);

        // Create dense interconnected links among the 10 nodes:
        // Forms 0..2 are roots. Forms 3..5 link to forms 0..2. Forms 6..9 link to 3..5
        for (int i = 3; i <= 5; ++i) {
            auto child = std::dynamic_pointer_cast<LinkedForm<2>>(bf10.forms[i]);
            auto parent = bf10.forms[i - 3];
            if (child && parent) {
                child->next_index = 1;
                child->insert_parent(parent);
                parent->insert_child(child);
            }
        }
        for (int i = 6; i <= 8; ++i) {
            auto child = std::dynamic_pointer_cast<LinkedForm<2>>(bf10.forms[i]);
            auto parent = bf10.forms[3 + (i % 3)];
            if (child && parent) {
                child->next_index = 1;
                child->insert_parent(parent);
                parent->insert_child(child);
            }
        }


        // Now clear canvas
        bf10.clear_canvas();

        // Verify clean teardown
        assert(bf10.forms.empty() == true);
        assert(bf10.current_ID == 0);
        assert(bf10.linking_mode == false);
        assert(bf10.parent == nullptr);
        assert(bf10.child == nullptr);
        assert(bf10.active_batch == nullptr);
        assert(bf10.get_lifecycle_state() == BatchLifecycleState::IDLE);

        // Push new form after clear: ID must restart cleanly at 0
        bf10.forms.push_back(LinkedInterface::make_linked(0, 0, 1));
        bf10.forms.back()->set_id(bf10.current_ID++);
        assert(bf10.forms.size() == 1);
        assert(bf10.forms[0]->get_form_num() == 0);

        std::cout << "  [PASS] Clear canvas with 10 densely linked nodes cleanly tore down all graph state." << std::endl;
    }

    // =========================================================================
    // TEST 6: Execution Tracker States & Reset Lifecycle
    // =========================================================================
    {
        std::cout << "\n[Test 6] Execution Tracker & Pipeline Reset Lifecycle..." << std::endl;
        BatchForm bf;
        auto form1 = std::dynamic_pointer_cast<LinkedForm<3>>(LinkedInterface::make_linked(0, 0, 1));
        form1->set_id(0);
        strncpy(form1->_params_in_form[0].param, "ASIA", 255);
        strncpy(form1->_params_in_form[1].param, "Faker", 255);
        strncpy(form1->_params_in_form[2].param, "KR1", 255);
        form1->on_close_popup();

        auto form2 = std::dynamic_pointer_cast<LinkedForm<2>>(LinkedInterface::make_linked(1, 6, 2));
        form2->set_id(1);
        strncpy(form2->_params_in_form[0].param, "ASIA", 255);
        form2->next_index = 1;
        form2->insert_parent(form1);
        form1->insert_child(form2);
        form2->iter_info[0]->keys.push_back(KEY_CONT("puuid"));
        form2->on_close_popup();

        bf.forms.push_back(form1);
        bf.forms.push_back(form2);

        // Pre-execution tracker states
        form1->construct_base();
        form2->construct_base();
        assert(form1->get_tracker()->state.load() == NodeExecState::READY);
        assert(form2->get_tracker()->state.load() == NodeExecState::WAITING_FOR_PARENTS);

        // Simulate execution progression
        form1->get_tracker()->state.store(NodeExecState::EXECUTING);
        form1->get_tracker()->processed_count.store(1);
        form1->get_tracker()->total_count.store(1);
        form1->get_tracker()->state.store(NodeExecState::COMPLETED);

        form2->get_tracker()->state.store(NodeExecState::READY);
        form2->get_tracker()->state.store(NodeExecState::EXECUTING);
        form2->get_tracker()->processed_count.store(5);
        form2->get_tracker()->total_count.store(5);
        form2->get_tracker()->state.store(NodeExecState::COMPLETED);

        bf.lifecycle = BatchLifecycleState::COMPLETED;

        // Trigger reset_pipeline()
        bf.reset_pipeline();

        // Verify post-reset state
        assert(bf.get_lifecycle_state() == BatchLifecycleState::IDLE);
        assert(bf.active_batch == nullptr);
        // Form 1 should be READY (inputs are valid)
        assert(form1->get_tracker()->state.load() == NodeExecState::READY);
        assert(form1->get_tracker()->processed_count.load() == 0);
        // Form 2 should be READY (or IDLE; when inputs valid)
        assert(form2->get_tracker()->processed_count.load() == 0);
        assert(form1->get_base_request() == nullptr);
        assert(form2->get_base_request() == nullptr);

        std::cout << "  [PASS] Pipeline reset successfully restored all node trackers and lifecycle state." << std::endl;
    }

    // =========================================================================
    // TEST 7: Concurrency Mutual Exclusion Guard
    // =========================================================================
    {
        std::cout << "\n[Test 7] Concurrency Mutual Exclusion Guard in execute_request()..." << std::endl;
        BatchForm bf;
        bf.lifecycle = BatchLifecycleState::RUNNING;
        // Attempting to execute should reject without throwing or altering active_batch
        bf.execute_request();
        assert(bf.lifecycle == BatchLifecycleState::RUNNING);
        assert(bf.active_batch == nullptr); // Sender was not invoked
        std::cout << "  [PASS] Mutual exclusion guard correctly rejected execution while RUNNING." << std::endl;
    }

    ImGui::DestroyContext(imgui_ctx);
    std::cout << "\n[ALL ADVERSARIAL STRESS TESTS PASSED SUCCESSFULLY!]" << std::endl;
    return 0;
}
