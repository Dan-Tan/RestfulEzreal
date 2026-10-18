#include "BatchRequests.h"
#include "QueryForm.h"
#include "RequestQueue.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

using namespace restfulEz;

// Helper to construct a raw JSON response buffer
static json_ptr make_json_response(const std::string& json_str) {
    auto buf = std::make_unique<std::vector<char>>(json_str.begin(), json_str.end());
    buf->push_back('\0');
    return buf;
}

// =============================================================================
// Test 1: NodeExecutionTracker Unit & Reset Semantics
// =============================================================================
void test_tracker_unit() {
    std::cout << "[Test 1] NodeExecutionTracker Unit & Reset Semantics..." << std::endl;
    NodeExecutionTracker tracker;

    // Default construction
    assert(tracker.state.load() == NodeExecState::IDLE);
    assert(tracker.processed_count.load() == 0);
    assert(tracker.items_dispatched.load() == 0);
    assert(tracker.total_count.load() == 0);
    assert(tracker.items_total.load() == 0);
    assert(tracker.error_message.empty());

    // Advance counters and change state
    tracker.set_total(10);
    assert(tracker.total_count.load() == 10);
    assert(tracker.items_total.load() == 10);

    tracker.increment_dispatched();
    tracker.increment_dispatched();
    tracker.increment_dispatched();
    assert(tracker.processed_count.load() == 3);
    assert(tracker.items_dispatched.load() == 3);

    tracker.state.store(NodeExecState::EXECUTING);
    assert(tracker.state.load() == NodeExecState::EXECUTING);

    tracker.error_message = "Network timeout";

    // Reset with default (IDLE)
    tracker.reset();
    assert(tracker.state.load() == NodeExecState::IDLE);
    assert(tracker.processed_count.load() == 0);
    assert(tracker.items_dispatched.load() == 0);
    assert(tracker.total_count.load() == 0);
    assert(tracker.items_total.load() == 0);
    assert(tracker.error_message.empty());

    // Reset with explicit initial state (READY)
    tracker.increment_dispatched();
    tracker.set_total(5);
    tracker.state.store(NodeExecState::COMPLETED);
    tracker.reset(NodeExecState::READY);
    assert(tracker.state.load() == NodeExecState::READY);
    assert(tracker.processed_count.load() == 0);
    assert(tracker.total_count.load() == 0);
    assert(tracker.error_message.empty());

    std::cout << "  [PASS] NodeExecutionTracker Unit & Reset Semantics" << std::endl;
}

// =============================================================================
// Test 2: NodeExecState Lifecycle Transitions in BatchRequest
// (IDLE -> READY -> WAITING_FOR_PARENTS -> EXECUTING -> COMPLETED)
// =============================================================================
void test_lifecycle_transitions() {
    std::cout << "[Test 2] NodeExecState Lifecycle Transitions in BatchRequest..." << std::endl;

    // Stage 1: Root request (Account by PUUID)
    auto s1_req = std::make_unique<LinkedRequest>();
    s1_req->_game = 0;
    s1_req->_endpoint = 0;
    s1_req->_endpoint_method = 0;
    s1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_ROOT_1")};

    auto tracker1 = std::make_shared<NodeExecutionTracker>();
    tracker1->state.store(NodeExecState::READY);
    s1_req->tracker = tracker1;

    auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));
    stage1->tracker = tracker1;

    // Stage 2: Child request (Match details by Match ID, linked to Stage 1)
    auto s2_req = std::make_unique<IterativeRequest>();
    s2_req->_game = 1;
    s2_req->_endpoint = 6;
    s2_req->_endpoint_method = 0;
    s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
    s2_req->param_indices = {1};

    iter_request_link link_1_to_2;
    link_1_to_2.parent = stage1;
    link_1_to_2.param_indices = {1};
    iter_access_info match_access;
    match_access.keys = {KEY_CONT("matches")};
    link_1_to_2.iter_link_info.push_back(match_access);
    s2_req->iter_dependencies.push_back(link_1_to_2);

    auto tracker2 = std::make_shared<NodeExecutionTracker>();
    tracker2->state.store(NodeExecState::WAITING_FOR_PARENTS);
    s2_req->tracker = tracker2;

    auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
    stage2->tracker = tracker2;
    stage1->_node->unsent_request->children.push_back(stage2);

    // Initial check
    assert(tracker1->state.load() == NodeExecState::READY);
    assert(tracker2->state.load() == NodeExecState::WAITING_FOR_PARENTS);

    // Batch initialization
    BatchRequest batch({stage1});
    assert(!batch.is_completed());
    assert(tracker1->state.load() == NodeExecState::READY);
    assert(tracker2->state.load() == NodeExecState::WAITING_FOR_PARENTS);

    // Step 1: get_next() starts root request -> transitions tracker1 to EXECUTING
    request r1 = batch.get_next();
    assert(!r1.same_endpoint(BatchRequest::FINISHED));
    assert(tracker1->state.load() == NodeExecState::EXECUTING);
    assert(tracker2->state.load() == NodeExecState::WAITING_FOR_PARENTS);

    // Root request produces matches array: ["KR_1001", "KR_1002", "KR_1003"]
    std::string root_resp = R"({"puuid":"PUUID_ROOT_1","matches":["KR_1001","KR_1002","KR_1003"]})";
    bool more_root = batch.insert_result(make_json_response(root_resp));
    assert(!more_root); // Non-iterative root completed
    assert(tracker1->state.load() == NodeExecState::COMPLETED);
    assert(tracker1->processed_count.load() == 1);

    // Unblocking child: Stage 2 tracker should now be transitioned to READY
    assert(tracker2->state.load() == NodeExecState::READY);

    // Step 2: Dispatch child iteration 1
    request r2_1 = batch.get_next();
    assert(!r2_1.same_endpoint(BatchRequest::FINISHED));
    assert(std::string(r2_1.params[1].param) == "KR_1001");
    assert(tracker2->state.load() == NodeExecState::EXECUTING);
    assert(tracker2->total_count.load() == 3);

    // Insert result for KR_1001
    bool more_child_1 = batch.insert_result(make_json_response(R"({"metadata":{"matchId":"KR_1001"}})"));
    assert(more_child_1); // Still has KR_1002 and KR_1003
    assert(tracker2->processed_count.load() == 1);
    assert(tracker2->state.load() == NodeExecState::EXECUTING);

    // Step 3: Dispatch child iteration 2
    request r2_2 = batch.get_current();
    assert(std::string(r2_2.params[1].param) == "KR_1002");
    bool more_child_2 = batch.insert_result(make_json_response(R"({"metadata":{"matchId":"KR_1002"}})"));
    assert(more_child_2);
    assert(tracker2->processed_count.load() == 2);
    assert(tracker2->state.load() == NodeExecState::EXECUTING);

    // Step 4: Dispatch child iteration 3
    request r2_3 = batch.get_current();
    assert(std::string(r2_3.params[1].param) == "KR_1003");
    bool more_child_3 = batch.insert_result(make_json_response(R"({"metadata":{"matchId":"KR_1003"}})"));
    assert(!more_child_3); // Exhausted pool
    assert(tracker2->processed_count.load() == 3);
    assert(tracker2->state.load() == NodeExecState::COMPLETED);

    // Step 5: Finished
    request r_end = batch.get_next();
    assert(r_end.same_endpoint(BatchRequest::FINISHED));
    assert(batch.is_completed());

    std::cout << "  [PASS] NodeExecState Lifecycle Transitions in BatchRequest" << std::endl;
}

// =============================================================================
// Test 3: Failure State Transitions on Malformed JSON / Extraction Errors
// =============================================================================
void test_failure_transitions() {
    std::cout << "[Test 3] Failure State Transitions on JSON Extraction Errors..." << std::endl;

    auto s1_req = std::make_unique<LinkedRequest>();
    s1_req->_game = 0;
    s1_req->_endpoint = 0;
    s1_req->_endpoint_method = 0;
    s1_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("PUUID_FAIL")};

    auto tracker1 = std::make_shared<NodeExecutionTracker>();
    tracker1->state.store(NodeExecState::READY);
    s1_req->tracker = tracker1;

    auto stage1 = std::make_shared<RequestNode>(std::move(s1_req));
    stage1->tracker = tracker1;

    auto s2_req = std::make_unique<IterativeRequest>();
    s2_req->_game = 1;
    s2_req->_endpoint = 6;
    s2_req->_endpoint_method = 0;
    s2_req->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
    s2_req->param_indices = {1};

    iter_request_link link_1_to_2;
    link_1_to_2.parent = stage1;
    link_1_to_2.param_indices = {1};
    // Non-existent key path
    iter_access_info invalid_access;
    invalid_access.keys = {KEY_CONT("nonexistent_array")};
    link_1_to_2.iter_link_info.push_back(invalid_access);
    s2_req->iter_dependencies.push_back(link_1_to_2);

    auto tracker2 = std::make_shared<NodeExecutionTracker>();
    tracker2->state.store(NodeExecState::WAITING_FOR_PARENTS);
    s2_req->tracker = tracker2;

    auto stage2 = std::make_shared<RequestNode>(std::move(s2_req));
    stage2->tracker = tracker2;
    stage1->_node->unsent_request->children.push_back(stage2);

    BatchRequest batch({stage1});
    request r1 = batch.get_next();
    assert(!r1.same_endpoint(BatchRequest::FINISHED));

    // Provide response that does not contain 'nonexistent_array' -> extraction results in 0 items -> fill_request returns false
    batch.insert_result(make_json_response(R"({"puuid":"PUUID_FAIL"})"));

    // Stage 1 completed
    assert(tracker1->state.load() == NodeExecState::COMPLETED);

    // Stage 2 could not be filled / failed readiness
    // The pipeline finishes cleanly without hanging
    request next = batch.get_next();
    assert(next.same_endpoint(BatchRequest::FINISHED));
    assert(batch.is_completed());

    std::cout << "  [PASS] Failure State Transitions on JSON Extraction Errors" << std::endl;
}

// =============================================================================
// Test 4: Pipeline Reset & Re-run Fidelity Without Graph Reconstruction
// =============================================================================
void test_pipeline_reset_and_rerun() {
    std::cout << "[Test 4] Pipeline Reset & Re-run Fidelity (3 Iterations)..." << std::endl;

    int query_count = 0;
    auto tracker1 = std::make_shared<NodeExecutionTracker>();
    auto tracker2 = std::make_shared<NodeExecutionTracker>();

    // We simulate 3 full run -> reset -> re-run cycles
    for (int cycle = 1; cycle <= 3; cycle++) {
        // Build nodes for the cycle (as BatchForm::construct_request() does on each execute)
        auto s1 = std::make_unique<LinkedRequest>();
        s1->_game = 0; s1->_endpoint = 0; s1->_endpoint_method = 0;
        s1->params = {PARAM_CONT("ASIA"), PARAM_CONT("P_100")};
        s1->tracker = tracker1;

        auto node1 = std::make_shared<RequestNode>(std::move(s1));
        node1->tracker = tracker1;

        auto s2 = std::make_unique<IterativeRequest>();
        s2->_game = 1; s2->_endpoint = 6; s2->_endpoint_method = 0;
        s2->params = {PARAM_CONT("ASIA"), PARAM_CONT("")};
        s2->param_indices = {1};
        s2->tracker = tracker2;

        iter_request_link link;
        link.parent = node1;
        link.param_indices = {1};
        iter_access_info acc;
        acc.keys = {KEY_CONT("matches")};
        link.iter_link_info.push_back(acc);
        s2->iter_dependencies.push_back(link);

        auto node2 = std::make_shared<RequestNode>(std::move(s2));
        node2->tracker = tracker2;
        node1->_node->unsent_request->children.push_back(node2);

        // Pre-run state
        tracker1->state.store(NodeExecState::READY);
        tracker2->state.store(NodeExecState::WAITING_FOR_PARENTS);

        auto batch = std::make_shared<BatchRequest>(std::vector<std::shared_ptr<RequestNode>>{node1});
        int pre_queries = query_count;

        // Execute batch via RequestSender worker thread
        {
            RequestSender sender;
            sender.set_custom_handler([&query_count](const request& req) -> json_ptr {
                query_count++;
                if (req._game == 0 && req._endpoint == 0) {
                    return make_json_response(R"({"puuid":"P_100","matches":["M101","M102"]})");
                } else if (req._game == 1 && req._endpoint == 6) {
                    std::string mid(req.params[1].param);
                    return make_json_response(R"({"metadata":{"matchId":")" + mid + R"("}})");
                }
                return make_json_response("{}");
            });
            sender.add_batch_request(batch);
        }

        // Post-execution checks
        assert(batch->is_completed());
        assert(query_count - pre_queries == 3); // 1 root + 2 child matches
        assert(tracker1->state.load() == NodeExecState::COMPLETED);
        assert(tracker1->processed_count.load() == 1);
        assert(tracker2->state.load() == NodeExecState::COMPLETED);
        assert(tracker2->processed_count.load() == 2);

        // Reset pipeline tracker state (as BatchForm::reset_pipeline() does)
        tracker1->reset(NodeExecState::READY);
        tracker2->reset(NodeExecState::IDLE);

        assert(tracker1->state.load() == NodeExecState::READY);
        assert(tracker1->processed_count.load() == 0);
        assert(tracker1->total_count.load() == 0);

        assert(tracker2->state.load() == NodeExecState::IDLE);
        assert(tracker2->processed_count.load() == 0);
        assert(tracker2->total_count.load() == 0);
    }

    std::cout << "  [PASS] Pipeline Reset & Re-run Fidelity (3 Iterations)" << std::endl;
}

// =============================================================================
// Test 5: Mutual Exclusion Guard & Concurrency Stress Test
// =============================================================================
void test_mutual_exclusion_stress() {
    std::cout << "[Test 5] Mutual Exclusion Guard & Concurrency Stress..." << std::endl;

    enum class LifecycleState { IDLE, RUNNING, COMPLETED, FAILED };
    std::atomic<LifecycleState> lifecycle{LifecycleState::IDLE};
    std::atomic<int> executions_accepted{0};
    std::atomic<int> executions_rejected{0};

    // Simulated execute_request logic matching BatchForm::execute_request()
    auto try_execute = [&]() -> bool {
        LifecycleState expected = LifecycleState::IDLE;
        if (lifecycle.compare_exchange_strong(expected, LifecycleState::RUNNING)) {
            executions_accepted++;
            return true;
        } else {
            executions_rejected++;
            return false;
        }
    };

    // 1. Single-threaded verify rejection when already running
    assert(try_execute() == true);
    assert(lifecycle.load() == LifecycleState::RUNNING);

    assert(try_execute() == false);
    assert(try_execute() == false);
    assert(executions_accepted.load() == 1);
    assert(executions_rejected.load() == 2);

    // Reset back to IDLE
    lifecycle.store(LifecycleState::IDLE);
    executions_accepted.store(0);
    executions_rejected.store(0);

    // 2. High-concurrency race: 16 threads simultaneously trying to execute
    std::vector<std::thread> threads;
    for (int i = 0; i < 16; i++) {
        threads.emplace_back([&try_execute]() {
            try_execute();
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    // Invariant: Exactly 1 thread must be accepted, 15 must be rejected
    assert(executions_accepted.load() == 1);
    assert(executions_rejected.load() == 15);
    assert(lifecycle.load() == LifecycleState::RUNNING);

    std::cout << "  [PASS] Mutual Exclusion Guard & Concurrency Stress" << std::endl;
}

// =============================================================================
// Test 6: Node Deletion and Dependency Unlinking via disconnect_all()
// =============================================================================
void test_node_deletion_and_unlinking() {
    std::cout << "[Test 6] Node Deletion and Dependency Unlinking..." << std::endl;

    BaseForm bf_account = BaseForm::make_form(0, 0, 0);
    auto parent = std::make_shared<LinkedForm<2>>(bf_account);
    parent->set_id(0);

    // Initial state: not marked for removal
    assert(!parent->check_remove());
    parent->mark_for_removal();
    assert(parent->check_remove());

    // Call disconnect_all() on parent: resets all dependencies, child links, and popups
    parent->disconnect_all();
    assert(parent->check_remove());

    // Reset tracker
    parent->reset_tracker();
    assert(parent->get_tracker()->state.load() == NodeExecState::IDLE);

    std::cout << "  [PASS] Node Deletion and Dependency Unlinking" << std::endl;
}

// =============================================================================
// Test 7: BatchForm Canvas Clear and Reset Semantics
// =============================================================================
void test_batch_form_canvas_clear_and_reset() {
    std::cout << "[Test 7] BatchForm Canvas Clear and Reset Semantics..." << std::endl;

    BatchForm form;
    assert(form.get_forms().empty());
    assert(form.get_lifecycle_state() == BatchLifecycleState::IDLE);

    // Call reset_pipeline on empty form -> must not crash
    form.reset_pipeline();
    assert(form.get_lifecycle_state() == BatchLifecycleState::IDLE);

    // Call clear_canvas on empty form -> must not crash
    form.clear_canvas();
    assert(form.get_forms().empty());
    assert(form.get_lifecycle_state() == BatchLifecycleState::IDLE);

    std::cout << "  [PASS] BatchForm Canvas Clear and Reset Semantics" << std::endl;
}

// =============================================================================
// Main Entrypoint
// =============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Milestone 1 Empirical Challenger 2 Verification Suite   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // Initialize ImGui context (required for LinkedForm and BatchForm)
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    assert(ctx != nullptr);

    test_tracker_unit();
    test_lifecycle_transitions();
    test_failure_transitions();
    test_pipeline_reset_and_rerun();
    test_mutual_exclusion_stress();
    test_node_deletion_and_unlinking();
    test_batch_form_canvas_clear_and_reset();

    ImGui::DestroyContext(ctx);

    std::cout << "==========================================================" << std::endl;
    std::cout << "  ALL EMPIRICAL CHALLENGER TESTS PASSED SUCCESSFULLY!     " << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
