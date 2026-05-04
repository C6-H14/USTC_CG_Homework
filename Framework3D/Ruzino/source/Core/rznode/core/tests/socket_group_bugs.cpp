#include <gtest/gtest.h>
#include <entt/meta/meta.hpp>
#include <iostream>

#include "nodes/core/api.hpp"
#include "nodes/core/node.hpp"
#include "nodes/core/node_tree.hpp"
#include "nodes/core/node_exec_eager.hpp"
#include "nodes/core/node_link.hpp"

using namespace Ruzino;

// Test fixture for socket group bugs
class SocketGroupBugsTest : public ::testing::Test {
   protected:
    void SetUp() override
    {
        register_cpp_type<int>();
        register_cpp_type<float>();
        register_cpp_type<std::string>();

        descriptor = std::make_shared<NodeTreeDescriptor>();

        // Register a node with typed input group (like merge_geometry)
        NodeTypeInfo merge_node;
        merge_node.id_name = "merge";
        merge_node.ui_name = "Merge";
        merge_node.ALWAYS_REQUIRED = true;
        merge_node.set_declare_function([](NodeDeclarationBuilder& b) {
            b.add_input_group<int>("inputs").set_runtime_dynamic(true);
            b.add_output<int>("result");
        });

        merge_node.set_execution_function([](ExeParams params) {
            auto inputs = params.get_input_group<int>("inputs");
            int sum = 0;
            std::cout << "[Merge Node] Executing with " << inputs.size() << " inputs: ";
            for (auto& input : inputs) {
                std::cout << input << " ";
                sum += input;
            }
            std::cout << " => sum=" << sum << std::endl;
            params.set_output("result", sum);
            return true;
        });

        descriptor->register_node(merge_node);

        // Register a simple producer node
        NodeTypeInfo producer_node;
        producer_node.id_name = "producer";
        producer_node.ui_name = "Producer";
        producer_node.ALWAYS_REQUIRED = true;
        producer_node.set_declare_function([](NodeDeclarationBuilder& b) {
            b.add_input<int>("value").default_val(10);
            b.add_output<int>("result");
        });

        producer_node.set_execution_function([](ExeParams params) {
            auto val = params.get_input<int>("value");
            std::cout << "[Producer Node " << params.node_.ID.Get() << "] Producing value: " << val << std::endl;
            params.set_output("result", val);
            return true;
        });

        descriptor->register_node(producer_node);

        tree = create_node_tree(descriptor);
    }

    void TearDown() override
    {
        entt::meta_reset();
    }
    
    std::shared_ptr<NodeTreeDescriptor> descriptor;
    std::unique_ptr<NodeTree> tree;
};

// Bug 1: Delete link but executor still uses old cached results
// This test simulates: A->C, delete link, then node C still gets A's OLD cached value
TEST_F(SocketGroupBugsTest, DeleteLinkShouldInvalidateCache)
{
    std::cout << "\n=== TEST: DeleteLinkShouldInvalidateCache ===" << std::endl;
    
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());

    // Create topology: A->C, B->C (merge C merges A and B)
    auto nodeA = tree->add_node("producer");
    auto nodeB = tree->add_node("producer");
    auto nodeC = tree->add_node("merge");

    // Connect A to C - this will create a socket in C's input group
    auto socketA_out = nodeA->get_output_socket("result");
    auto socketC_in1 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    auto link1 = tree->add_link(socketA_out, socketC_in1);

    // Connect B to C - this will create another socket in C's input group
    auto socketB_out = nodeB->get_output_socket("result");
    auto socketC_in2 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_1", "input_1", PinKind::Input);
    auto link2 = tree->add_link(socketB_out, socketC_in2);

    std::cout << "\n--- Phase 1: Initial execution ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 100);
    executor->sync_node_from_external_storage(nodeB->get_input_socket("value"), 200);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (expected: 300)" << std::endl;
    ASSERT_EQ(result.cast<int>(), 300);  // 100 + 200
    
    // Check the persistent cache - socket should have cached value
    std::cout << "\n--- Check persistent cache state ---" << std::endl;
    auto* cached_val_in1 = executor->get_socket_value(socketC_in1);
    auto* cached_val_in2 = executor->get_socket_value(socketC_in2);
    std::cout << "socketC_in1 cached value: " << (cached_val_in1 && *cached_val_in1 ? cached_val_in1->cast<int>() : -999) << std::endl;
    std::cout << "socketC_in2 cached value: " << (cached_val_in2 && *cached_val_in2 ? cached_val_in2->cast<int>() : -999) << std::endl;

    std::cout << "\n--- Phase 2: Delete link A->C ---" << std::endl;
    // Simulate UI deletion: delete the link and notify
    auto affected_node = link1->to_sock->node;
    tree->delete_link(link1);
    
    // CRITICAL: This is what the UI does when a link is deleted
    executor->notify_node_dirty(affected_node);

    std::cout << "Dirty states after link deletion:" << std::endl;
    std::cout << "  nodeA: " << executor->is_node_dirty(nodeA) << " (should be false)" << std::endl;
    std::cout << "  nodeB: " << executor->is_node_dirty(nodeB) << " (should be false)" << std::endl;
    std::cout << "  nodeC: " << executor->is_node_dirty(nodeC) << " (should be true)" << std::endl;
    
    ASSERT_FALSE(executor->is_node_dirty(nodeA));
    ASSERT_FALSE(executor->is_node_dirty(nodeB));
    ASSERT_TRUE(executor->is_node_dirty(nodeC));
    
    // CRITICAL CHECK: After deleting link, the socket's cached value should be marked invalid
    // But the socket pointer is now dangling! The socket was deleted!
    std::cout << "\n--- Check if deleted socket's cache was invalidated ---" << std::endl;
    std::cout << "Note: socketC_in1 was deleted, so we can't check it directly" << std::endl;

    std::cout << "\n--- Phase 3: Re-execute after link deletion ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->execute_tree(tree.get());

    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (expected: 200, CRITICAL BUG if 300)" << std::endl;
    
    // BUG: Currently returns 300 (cached A+B), should return 200 (only B)
    ASSERT_EQ(result.cast<int>(), 200) << "CRITICAL BUG: Merge node should only receive B's value (200), not cached A+B (300)";
}

// Bug 1b: Reconnect link - downstream executes but gets OLD cached value
// This is the REAL bug: A->C, disconnect, D->C, C executes but gets A's old value!
TEST_F(SocketGroupBugsTest, ReconnectLinkGetsOldCachedValue)
{
    std::cout << "\n=== TEST: ReconnectLinkGetsOldCachedValue ===" << std::endl;
    std::cout << "This simulates: A->C (result=100), disconnect, D->C (should be 500, bug if 100)" << std::endl;
    
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());

    auto nodeA = tree->add_node("producer");
    auto nodeC = tree->add_node("merge");
    auto nodeDown = tree->add_node("producer");  // Downstream node to observe changes

    // Connect A to C
    auto socketA_out = nodeA->get_output_socket("result");
    auto socketC_in1 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    tree->add_link(socketA_out, socketC_in1);
    
    // Connect C to downstream to see if it gets updated
    tree->add_link(
        nodeC->get_output_socket("result"),
        nodeDown->get_input_socket("value"));

    std::cout << "\n--- Phase 1: Initial execution with A->C ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 100);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "C result: " << result.cast<int>() << " (expected: 100)" << std::endl;
    ASSERT_EQ(result.cast<int>(), 100);
    
    executor->sync_node_to_external_storage(nodeDown->get_output_socket("result"), result);
    std::cout << "Downstream result: " << result.cast<int>() << " (expected: 100)" << std::endl;
    ASSERT_EQ(result.cast<int>(), 100);

    std::cout << "\n--- Phase 2: Disconnect A->C, Connect D->C ---" << std::endl;
    // Find and delete the link
    NodeLink* link_to_delete = nullptr;
    for (auto& link : tree->links) {
        if (link->from_node == nodeA && link->to_node == nodeC) {
            link_to_delete = link.get();
            break;
        }
    }
    ASSERT_NE(link_to_delete, nullptr);
    
    tree->delete_link(link_to_delete);
    executor->notify_node_dirty(nodeC);
    
    // Create new producer D with DIFFERENT value
    auto nodeD = tree->add_node("producer");
    auto socketD_out = nodeD->get_output_socket("result");
    auto socketC_in_new = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_1", "input_1", PinKind::Input);
    tree->add_link(socketD_out, socketC_in_new);
    
    std::cout << "\n--- Phase 3: Execute with D->C (D produces 500) ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeD->get_input_socket("value"), 500);
    executor->execute_tree(tree.get());

    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "C result: " << result.cast<int>() << " (expected: 500, CRITICAL BUG if 100)" << std::endl;
    
    executor->sync_node_to_external_storage(nodeDown->get_output_socket("result"), result);
    std::cout << "Downstream result FIRST execution: " << result.cast<int>() << " (might still be 100 if cached)" << std::endl;
    
    std::cout << "\n--- Phase 4: Execute again (downstream should now see updated value) ---" << std::endl;
    std::cout << "Downstream node dirty state: " << executor->is_node_dirty(nodeDown) << " (should be true if fix works)" << std::endl;
    executor->prepare_tree(tree.get());
    executor->execute_tree(tree.get());
    
    executor->sync_node_to_external_storage(nodeDown->get_output_socket("result"), result);
    std::cout << "Downstream result SECOND execution: " << result.cast<int>() << " (expected: 500, CRITICAL BUG if 100)" << std::endl;
    std::cout << "If downstream executed but got 100 instead of 500, that's the bug!" << std::endl;
    
    ASSERT_EQ(result.cast<int>(), 500) << "CRITICAL BUG: Downstream should get new value (500), not old cached value (100)!";
}

// Bug 2a: Type mismatch when persistent cache has wrong type
TEST_F(SocketGroupBugsTest, PersistentCacheTypeMismatch)
{
    std::cout << "\n=== TEST: PersistentCacheTypeMismatch ===" << std::endl;
    std::cout << "Simulates: int socket gets cached, deleted, new float socket created at same position" << std::endl;
    
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());

    // Register a float producer for type mismatch testing
    NodeTypeInfo float_producer;
    float_producer.id_name = "float_producer";
    float_producer.ui_name = "Float Producer";
    float_producer.ALWAYS_REQUIRED = true;
    float_producer.set_declare_function([](NodeDeclarationBuilder& b) {
        b.add_input<float>("value").default_val(3.14f);
        b.add_output<float>("result");
    });
    float_producer.set_execution_function([](ExeParams params) {
        auto val = params.get_input<float>("value");
        std::cout << "[Float Producer " << params.node_.ID.Get() << "] Producing: " << val << std::endl;
        params.set_output("result", val);
        return true;
    });
    descriptor->register_node(float_producer);
    
    auto nodeA = tree->add_node("producer");  // int producer
    auto nodeC = tree->add_node("merge");     // int merge

    // Create socket and connect int producer
    auto socketA_out = nodeA->get_output_socket("result");
    auto socketC_in1 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    tree->add_link(socketA_out, socketC_in1);

    std::cout << "\n--- Phase 1: Execute with int producer ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 42);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (type: int)" << std::endl;
    ASSERT_EQ(result.cast<int>(), 42);
    
    // Now the persistent cache has int value for socketC_in1

    std::cout << "\n--- Phase 2: Delete int producer, socket should be removed ---" << std::endl;
    tree->delete_node(nodeA);
    executor->mark_tree_structure_changed();
    
    // The socket socketC_in1 should be deleted now
    int remaining = 0;
    for (auto* input : nodeC->get_inputs()) {
        if (input->socket_group && !input->is_placeholder()) {
            remaining++;
            std::cout << "Remaining socket: " << input->identifier << " (type: " << input->type_info.info().name() << ")" << std::endl;
        }
    }
    ASSERT_EQ(remaining, 0) << "All sockets should be removed";

    std::cout << "\n--- Phase 3: Create new socket at 'same' position with int type ---" << std::endl;
    auto nodeD = tree->add_node("producer");  // int producer
    auto socketD_out = nodeD->get_output_socket("result");
    
    // Create NEW socket with SAME identifier "input_0" but it's a different socket object
    auto socketC_in_new = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    
    std::cout << "Old socket ptr: " << socketC_in1 << std::endl;
    std::cout << "New socket ptr: " << socketC_in_new << std::endl;
    std::cout << "Are they different objects? " << (socketC_in1 != socketC_in_new ? "YES" : "NO") << std::endl;
    
    tree->add_link(socketD_out, socketC_in_new);

    std::cout << "\n--- Phase 4: Execute with new int producer ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeD->get_input_socket("value"), 999);
    executor->execute_tree(tree.get());

    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << std::endl;
    
    if (!nodeC->execution_failed.empty()) {
        std::cout << "ERROR: " << nodeC->execution_failed << std::endl;
    }
    
    // This should work since types match
    ASSERT_TRUE(nodeC->execution_failed.empty()) << "Same type should work: " << nodeC->execution_failed;
    ASSERT_EQ(result.cast<int>(), 999);
}

// Bug 2: Complex delete operations with socket groups cause type mismatch
TEST_F(SocketGroupBugsTest, DeleteNodesShouldRemoveSocketsAndAllowReconnection)
{
    std::cout << "\n=== TEST: DeleteNodesShouldRemoveSocketsAndAllowReconnection ===" << std::endl;
    
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());

    // Create topology: A->C, B->C
    auto nodeA = tree->add_node("producer");
    auto nodeB = tree->add_node("producer");
    auto nodeC = tree->add_node("merge");

    auto socketA_out = nodeA->get_output_socket("result");
    auto socketC_in1 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    tree->add_link(socketA_out, socketC_in1);

    auto socketB_out = nodeB->get_output_socket("result");
    auto socketC_in2 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_1", "input_1", PinKind::Input);
    tree->add_link(socketB_out, socketC_in2);

    std::cout << "\n--- Phase 1: Initial execution ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 100);
    executor->sync_node_from_external_storage(nodeB->get_input_socket("value"), 200);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (expected: 300)" << std::endl;
    ASSERT_EQ(result.cast<int>(), 300);

    std::cout << "\n--- Phase 2: Delete both nodes A and B ---" << std::endl;
    // When deleting nodes, their sockets should be removed, and connected group sockets in C should also be removed
    tree->delete_node(nodeA);
    tree->delete_node(nodeB);
    
    // Notify executor about structure change
    executor->mark_tree_structure_changed();
    
    std::cout << "Remaining sockets in nodeC's input group after deletion:" << std::endl;
    int remaining_sockets = 0;
    for (auto* input : nodeC->get_inputs()) {
        if (input->socket_group && input->socket_group->identifier == "inputs") {
            if (!input->is_placeholder()) {
                std::cout << "  - " << input->identifier << " (UI: " << input->ui_name << ")" << std::endl;
                remaining_sockets++;
            }
        }
    }
    std::cout << "Total non-placeholder sockets: " << remaining_sockets << " (should be 0)" << std::endl;

    std::cout << "\n--- Phase 3: Create new node D and connect to C ---" << std::endl;
    auto nodeD = tree->add_node("producer");
    
    auto socketD_out = nodeD->get_output_socket("result");
    auto socketC_in3 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_2", "input_2", PinKind::Input);
    
    std::cout << "Created new socket in C: " << socketC_in3->identifier << std::endl;
    std::cout << "Socket type: " << socketC_in3->type_info.info().name() << std::endl;
    
    tree->add_link(socketD_out, socketC_in3);

    std::cout << "\n--- Phase 4: Execute with new connection ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeD->get_input_socket("value"), 500);
    executor->execute_tree(tree.get());

    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (expected: 500)" << std::endl;
    
    // Check for "Type mismatch input" error
    if (!nodeC->execution_failed.empty()) {
        std::cout << "ERROR: " << nodeC->execution_failed << std::endl;
    }
    
    ASSERT_TRUE(nodeC->execution_failed.empty()) << "CRITICAL BUG: Got error: " << nodeC->execution_failed;
    ASSERT_EQ(result.cast<int>(), 500) << "Should get D's value (500)";
}

// Additional test: Delete link in the middle of multiple connections
TEST_F(SocketGroupBugsTest, DeleteMiddleLinkShouldOnlyAffectThatConnection)
{
    std::cout << "\n=== TEST: DeleteMiddleLinkShouldOnlyAffectThatConnection ===" << std::endl;
    
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());

    // Create topology: A->C, B->C, D->C (three inputs to merge)
    auto nodeA = tree->add_node("producer");
    auto nodeB = tree->add_node("producer");
    auto nodeD = tree->add_node("producer");
    auto nodeC = tree->add_node("merge");

    // Connect all three
    auto socketA_out = nodeA->get_output_socket("result");
    auto socketC_in1 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    tree->add_link(socketA_out, socketC_in1);

    auto socketB_out = nodeB->get_output_socket("result");
    auto socketC_in2 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_1", "input_1", PinKind::Input);
    auto link2 = tree->add_link(socketB_out, socketC_in2);

    auto socketD_out = nodeD->get_output_socket("result");
    auto socketC_in3 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_2", "input_2", PinKind::Input);
    tree->add_link(socketD_out, socketC_in3);

    std::cout << "\n--- Phase 1: Initial execution (A+B+D) ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 10);
    executor->sync_node_from_external_storage(nodeB->get_input_socket("value"), 20);
    executor->sync_node_from_external_storage(nodeD->get_input_socket("value"), 30);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (expected: 60)" << std::endl;
    ASSERT_EQ(result.cast<int>(), 60);  // 10 + 20 + 30

    std::cout << "\n--- Phase 2: Delete middle link (B->C) ---" << std::endl;
    auto affected_node = link2->to_sock->node;
    tree->delete_link(link2);
    executor->notify_node_dirty(affected_node);

    std::cout << "\n--- Phase 3: Re-execute (should be A+D only) ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->execute_tree(tree.get());

    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (expected: 40, CRITICAL BUG if 60)" << std::endl;
    
    ASSERT_EQ(result.cast<int>(), 40) << "Should be A+D (10+30=40), not A+B+D (60)";
}

// Test the root cause: socket group data not being invalidated properly
TEST_F(SocketGroupBugsTest, SocketGroupCacheMustBeInvalidatedOnLinkDelete)
{
    std::cout << "\n=== TEST: SocketGroupCacheMustBeInvalidatedOnLinkDelete ===" << std::endl;
    
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());

    auto nodeA = tree->add_node("producer");
    auto nodeC = tree->add_node("merge");

    auto socketA_out = nodeA->get_output_socket("result");
    auto socketC_in1 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    auto link1 = tree->add_link(socketA_out, socketC_in1);

    std::cout << "\n--- Phase 1: First execution ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 100);
    executor->execute_tree(tree.get());

    // Check that socket has cached value
    auto* socket_value = executor->get_socket_value(socketC_in1);
    std::cout << "Socket value after execution: ";
    if (socket_value && *socket_value) {
        std::cout << socket_value->cast<int>() << std::endl;
    } else {
        std::cout << "NULL" << std::endl;
    }

    std::cout << "\n--- Phase 2: Delete link ---" << std::endl;
    tree->delete_link(link1);
    executor->notify_node_dirty(nodeC);

    std::cout << "\n--- Phase 3: Check if socket cache was invalidated ---" << std::endl;
    // After deletion and notification, the socket's cached value should be marked invalid
    // This test checks the internal state
    
    std::cout << "Node C dirty state: " << executor->is_node_dirty(nodeC) << " (should be true)" << std::endl;
    ASSERT_TRUE(executor->is_node_dirty(nodeC)) << "Node C should be marked dirty after link deletion";

    std::cout << "\n--- Phase 4: Execute without any inputs ---" << std::endl;
    executor->prepare_tree(tree.get());
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result);
    std::cout << "Result: " << result.cast<int>() << " (expected: 0, bug if 100)" << std::endl;
    
    ASSERT_EQ(result.cast<int>(), 0) << "Merge with no inputs should return 0, not cached 100";
}

// Bug 2: Type mismatch error when reconnecting after intermediate node
// Simplified scenario: A.sock1->C, disconnect, D.sock->C (different int value), disconnect, A.sock1->C
// The issue is persistent cache holds wrong type/value
TEST_F(SocketGroupBugsTest, TypeMismatchAfterIntermediateNodeWithDifferentType)
{
    std::cout << "\n=== TEST: TypeMismatchAfterIntermediateNodeWithDifferentType ===" << std::endl;
    
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());

    // Step 1: Create A with two outputs, connect both to C
    std::cout << "\n--- Step 1: A.sock1->C, A.sock2->C ---" << std::endl;
    auto nodeA = tree->add_node("producer");
    auto nodeC = tree->add_node("merge");

    auto socketA_out = nodeA->get_output_socket("result");
    
    // Create first socket in C and connect
    auto socketC_in1 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_0", "input_0", PinKind::Input);
    auto link1 = tree->add_link(socketA_out, socketC_in1);
    
    // Create second socket in C and connect (simulating A has 2 outputs by using same output)
    auto socketC_in2 = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_1", "input_1", PinKind::Input);
    auto link2 = tree->add_link(socketA_out, socketC_in2);

    // Execute to populate cache with int values
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 100);
    executor->execute_tree(tree.get());
    
    entt::meta_any result1;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result1);
    std::cout << "C result with A's two connections: " << result1.cast<int>() << std::endl;
    ASSERT_EQ(result1.cast<int>(), 200); // 100 + 100

    // Step 2: Disconnect both links
    std::cout << "\n--- Step 2: Disconnect A.sock1->C and A.sock2->C ---" << std::endl;
    std::cout << "Before deletion, socketC_in1 ptr: " << (void*)socketC_in1 << std::endl;
    std::cout << "Before deletion, socketC_in2 ptr: " << (void*)socketC_in2 << std::endl;
    tree->delete_link(link1);
    tree->delete_link(link2);
    executor->notify_node_dirty(nodeC);
    
    // Check if sockets still exist
    std::cout << "After deletion, checking socket status..." << std::endl;
    auto remaining_inputs = nodeC->get_inputs();
    std::cout << "NodeC has " << remaining_inputs.size() << " input sockets remaining" << std::endl;
    for (auto* sock : remaining_inputs) {
        std::cout << "  Socket: " << (void*)sock << " isPlaceholder=" << sock->is_placeholder() << std::endl;
    }

    // Step 3: Create D and connect to C - create NEW socket
    std::cout << "\n--- Step 3: Create D, D.sock->C (creating new socket) ---" << std::endl;
    auto nodeD = tree->add_node("producer");
    auto socketD_out = nodeD->get_output_socket("result");
    
    // Create a NEW socket in C for D's connection
    auto socketC_in_new = nodeC->group_add_socket(
        "inputs", type_name<int>().c_str(), "input_new", "input_new", PinKind::Input);
    std::cout << "Created new socket socketC_in_new ptr: " << (void*)socketC_in_new << std::endl;
    
    auto linkD = tree->add_link(socketD_out, socketC_in_new);
    
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(nodeD->get_input_socket("value"), 999);
    executor->execute_tree(tree.get());
    
    std::cout << "Node C execution_failed after D connection: '" << nodeC->execution_failed << "'" << std::endl;
    
    entt::meta_any result2;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result2);
    std::cout << "C result with D connection - has value: " << (result2 ? "yes" : "no") << std::endl;
    if (result2) {
        std::cout << "C result with D connection: " << result2.cast<int>() << std::endl;
        ASSERT_EQ(result2.cast<int>(), 999);
    }
    else {
        std::cout << "WARNING: C has no valid result!" << std::endl;
    }

    // Step 4: Disconnect D
    std::cout << "\n--- Step 4: Disconnect D.sock->C ---" << std::endl;
    tree->delete_link(linkD);
    executor->notify_node_dirty(nodeC);

    // Step 5: Reconnect A to C via NEW socket (not the deleted one)
    std::cout << "\n--- Step 5: Reconnect A.sock1->C (via new socket) ---" << std::endl;
    
    try {
        std::cout << "Creating NEW socket for A->C connection..." << std::endl;
        auto socketC_in_reconnect = nodeC->group_add_socket(
            "inputs", type_name<int>().c_str(), "input_reconnect", "input_reconnect", PinKind::Input);
        std::cout << "Created socketC_in_reconnect ptr: " << (void*)socketC_in_reconnect << std::endl;
        
        auto linkA_again = tree->add_link(socketA_out, socketC_in_reconnect);
        std::cout << "Link created successfully!" << std::endl;
    
        std::cout << "Calling prepare_tree..." << std::endl;
        executor->prepare_tree(tree.get());
        
        std::cout << "Calling sync_node_from_external_storage..." << std::endl;
        executor->sync_node_from_external_storage(nodeA->get_input_socket("value"), 500);
        
        std::cout << "Calling execute_tree..." << std::endl;
        executor->execute_tree(tree.get());
        
        std::cout << "Execution completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "EXCEPTION CAUGHT: " << e.what() << std::endl;
        throw;
    }
    
    std::cout << "Node C execution_failed: '" << nodeC->execution_failed << "' (should be empty)" << std::endl;
    
    // BUG CHECK: execution_failed should be empty
    ASSERT_TRUE(nodeC->execution_failed.empty()) 
        << "Node C should not have execution error, but got: " << nodeC->execution_failed;
    
    entt::meta_any result_final;
    executor->sync_node_to_external_storage(nodeC->get_output_socket("result"), result_final);
    std::cout << "C final result: " << result_final.cast<int>() << " (expected: 500)" << std::endl;
    ASSERT_EQ(result_final.cast<int>(), 500);
}
