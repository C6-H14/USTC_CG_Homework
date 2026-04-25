#include <gtest/gtest.h>

#include <entt/meta/meta.hpp>

#include "nodes/core/api.hpp"
#include "nodes/core/node.hpp"
#include "nodes/core/node_tree.hpp"
#include "nodes/core/node_exec_eager.hpp"
#include "nodes/core/node_link.hpp"

using namespace Ruzino;

class NodeExecTest : public ::testing::Test {
   protected:
    void SetUp() override
    {
        register_cpp_type<int>();
        register_cpp_type<float>();
        register_cpp_type<std::string>();

        std::shared_ptr<NodeTreeDescriptor> descriptor =
            std::make_shared<NodeTreeDescriptor>();

        // register adding node

        NodeTypeInfo add_node;
        add_node.id_name = "add";
        add_node.ui_name = "Add";
        add_node.ALWAYS_REQUIRED = true;
        add_node.set_declare_function([](NodeDeclarationBuilder& b) {
            b.add_input<int>("a");
            b.add_input<int>("b").default_val(1).min(0).max(10);
            b.add_output<int>("result");
        });

        add_node.set_execution_function([](ExeParams params) {
            auto a = params.get_input<int>("a");
            auto b = params.get_input<int>("b");
            params.set_output("result", a + b);
            return true;
        });

        descriptor->register_node(add_node);

        tree = create_node_tree(descriptor);
    }

    void TearDown() override
    {
        entt::meta_reset();
    }
    std::unique_ptr<NodeTree> tree;
};

 TEST_F(NodeExecTest, NodeExecSimple)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(desc);

    auto add_node = tree->add_node("add");

    executor->prepare_tree(tree.get());

    auto a = add_node->get_input_socket("a");
    auto b = add_node->get_input_socket("b");
    executor->sync_node_from_external_storage(a, 1);
    executor->sync_node_from_external_storage(b, 2);

    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(
        add_node->get_output_socket("result"), result);

    // Type is int
    ASSERT_EQ(result.type().info().name(), "int");
    ASSERT_EQ(result.cast<int>(), 3);
}

 TEST_F(NodeExecTest, NodeExecWithLink)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(desc);

    std::vector<Node*> add_nodes;

    for (int i = 0; i < 20; i++) {
        auto add_node = tree->add_node("add");
        add_nodes.push_back(add_node);
    }

    for (int i = 0; i < add_nodes.size() - 1; i++) {
        auto link = tree->add_link(
            add_nodes[i]->get_output_socket("result"),
            add_nodes[i + 1]->get_input_socket("a"));
    }

    executor->prepare_tree(tree.get());

    // Set the first node.a to 1

    auto a = add_nodes[0]->get_input_socket("a");
    executor->sync_node_from_external_storage(a, 1);

    // Set all the node.b to 2
    for (auto node : add_nodes) {
        auto b = node->get_input_socket("b");
        executor->sync_node_from_external_storage(b, 2);
    }

    executor->execute_tree(tree.get());

    // Get the last node result

    entt::meta_any result;
    executor->sync_node_to_external_storage(
        add_nodes.back()->get_output_socket("result"), result);

    // Type is int
    ASSERT_EQ(result.type().info().name(), "int");
    ASSERT_EQ(result.cast<int>(), 41);
}

TEST_F(NodeExecTest, NodeExecWithLinkAndNodeGroup)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(desc);

    std::vector<Node*> add_nodes;
    std::vector<Node*> nodes_to_group;

    auto add_node_0 = tree->add_node("add");

    auto add_node_1 = tree->add_node("add");
    auto add_node_2 = tree->add_node("add");

    tree->add_link(
        add_node_0->get_output_socket("result"),
        add_node_1->get_input_socket("a"));
    tree->add_link(
        add_node_1->get_output_socket("result"),
        add_node_2->get_input_socket("a"));

    tree->group_up({ add_node_1 });

    auto input_0_a = add_node_0->get_input_socket("a");
    auto input_0_b = add_node_0->get_input_socket("b");

    executor->prepare_tree(tree.get());

    executor->sync_node_from_external_storage(input_0_a, 1);
    executor->sync_node_from_external_storage(input_0_b, 2);

    executor->execute_tree(tree.get());

    entt::meta_any value_out = 0;

    executor->sync_node_to_external_storage(
        add_node_2->get_output_socket("result"), value_out);

    std::cout << value_out.cast<int>() << std::endl;
}

TEST_F(NodeExecTest, CacheTest)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(desc);

    // Create a chain: node0 -> node1 -> node2
    auto node0 = tree->add_node("add");
    auto node1 = tree->add_node("add");
    auto node2 = tree->add_node("add");

    tree->add_link(
        node0->get_output_socket("result"),
        node1->get_input_socket("a"));
    tree->add_link(
        node1->get_output_socket("result"),
        node2->get_input_socket("a"));

    // First execution: set inputs and execute
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(node0->get_input_socket("a"), 1);
    executor->sync_node_from_external_storage(node0->get_input_socket("b"), 2);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(
        node2->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 5);  // 1+2=3, 3+1=4, 4+1=5

    // Second execution: no changes, should use cache
    executor->prepare_tree(tree.get());
    executor->execute_tree(tree.get());
    
    executor->sync_node_to_external_storage(
        node2->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 5);  // Same result from cache

    // Third execution: change input of node1, should invalidate node1 and node2
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(node1->get_input_socket("b"), 10);
    executor->execute_tree(tree.get());
    
    executor->sync_node_to_external_storage(
        node2->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 14);  // node0: 1+2=3, node1: 3+10=13, node2: 13+1=14
}

TEST_F(NodeExecTest, CacheWithUpstreamChange)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor = create_node_tree_executor(desc);

    // Create a chain: node0 -> node1 -> node2
    auto node0 = tree->add_node("add");
    auto node1 = tree->add_node("add");
    auto node2 = tree->add_node("add");

    tree->add_link(
        node0->get_output_socket("result"),
        node1->get_input_socket("a"));
    tree->add_link(
        node1->get_output_socket("result"),
        node2->get_input_socket("a"));

    // First execution
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(node0->get_input_socket("a"), 5);
    executor->sync_node_from_external_storage(node0->get_input_socket("b"), 5);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(
        node2->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 12);  // 5+5=10, 10+1=11, 11+1=12

    // Change upstream node0, should invalidate all downstream
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(node0->get_input_socket("a"), 10);
    executor->execute_tree(tree.get());
    
    executor->sync_node_to_external_storage(
        node2->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 17);  // 10+5=15, 15+1=16, 16+1=17
}

TEST_F(NodeExecTest, UISocketDirtyPropagation)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());
    
    auto node0 = tree->add_node("add");
    auto node1 = tree->add_node("add");
    
    tree->add_link(
        node0->get_output_socket("result"),
        node1->get_input_socket("a"));

    // First execution
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(node0->get_input_socket("a"), 1);
    executor->sync_node_from_external_storage(node0->get_input_socket("b"), 2);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(
        node1->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 4);  // (1+2)+1=4

    // Simulate UI slider change: mark socket dirty directly
    executor->mark_socket_dirty(node0->get_input_socket("a"));
    
    // Update value and re-execute
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(node0->get_input_socket("a"), 10);
    executor->execute_tree(tree.get());
    
    executor->sync_node_to_external_storage(
        node1->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 13);  // (10+2)+1=13
}

TEST_F(NodeExecTest, LinkChangeOnlyAffectsDownstream)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());
    
    // Create chain: node0 -> node1 -> node2
    auto node0 = tree->add_node("add");
    auto node1 = tree->add_node("add");
    auto node2 = tree->add_node("add");
    
    tree->add_link(
        node0->get_output_socket("result"),
        node1->get_input_socket("a"));
    tree->add_link(
        node1->get_output_socket("result"),
        node2->get_input_socket("a"));

    // First execution
    std::cout << "\n=== First Execution ===" << std::endl;
    executor->prepare_tree(tree.get());
    executor->sync_node_from_external_storage(node0->get_input_socket("a"), 5);
    executor->sync_node_from_external_storage(node0->get_input_socket("b"), 5);
    executor->execute_tree(tree.get());

    entt::meta_any result;
    executor->sync_node_to_external_storage(
        node2->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 12);  // 5+5=10, 10+1=11, 11+1=12
    
    std::cout << "After first execution:" << std::endl;
    std::cout << "  node0 dirty: " << executor->is_node_dirty(node0) << std::endl;
    std::cout << "  node1 dirty: " << executor->is_node_dirty(node1) << std::endl;
    std::cout << "  node2 dirty: " << executor->is_node_dirty(node2) << std::endl;

    // Delete link between node0 and node1
    std::cout << "\n=== Deleting Link ===" << std::endl;
    NodeLink* link_to_delete = nullptr;
    for (auto& link : tree->links) {
        if (link->from_node == node0 && link->to_node == node1) {
            link_to_delete = link.get();
            break;
        }
    }
    ASSERT_NE(link_to_delete, nullptr);
    
    auto input_socket = link_to_delete->to_sock;
    tree->delete_link(link_to_delete->ID);
    
    // Simulate the notification that would come from UI
    executor->notify_socket_dirty(input_socket);
    
    std::cout << "After deleting link:" << std::endl;
    std::cout << "  node0 dirty: " << executor->is_node_dirty(node0) << std::endl;
    std::cout << "  node1 dirty: " << executor->is_node_dirty(node1) << std::endl;
    std::cout << "  node2 dirty: " << executor->is_node_dirty(node2) << std::endl;
    
    // node0 should be clean (upstream), node1 and node2 should be dirty (downstream)
    ASSERT_FALSE(executor->is_node_dirty(node0));
    ASSERT_TRUE(executor->is_node_dirty(node1));
    ASSERT_TRUE(executor->is_node_dirty(node2));
    
    // Re-add the same link
    std::cout << "\n=== Re-adding Link ===" << std::endl;
    tree->add_link(
        node0->get_output_socket("result"),
        node1->get_input_socket("a"));
    
    // Simulate notification from UI
    executor->notify_socket_dirty(node1->get_input_socket("a"));
    
    std::cout << "After re-adding link:" << std::endl;
    std::cout << "  node0 dirty: " << executor->is_node_dirty(node0) << std::endl;
    std::cout << "  node1 dirty: " << executor->is_node_dirty(node1) << std::endl;
    std::cout << "  node2 dirty: " << executor->is_node_dirty(node2) << std::endl;
    
    // Re-execute - should only execute node1 and node2, node0 uses cache
    std::cout << "\n=== Re-execution ===" << std::endl;
    executor->prepare_tree(tree.get());
    std::cout << "After prepare_tree:" << std::endl;
    std::cout << "  node0 dirty: " << executor->is_node_dirty(node0) << std::endl;
    std::cout << "  node1 dirty: " << executor->is_node_dirty(node1) << std::endl;
    std::cout << "  node2 dirty: " << executor->is_node_dirty(node2) << std::endl;
    
    executor->execute_tree(tree.get());
    
    std::cout << "After execute_tree:" << std::endl;
    std::cout << "  node0 dirty: " << executor->is_node_dirty(node0) << std::endl;
    std::cout << "  node1 dirty: " << executor->is_node_dirty(node1) << std::endl;
    std::cout << "  node2 dirty: " << executor->is_node_dirty(node2) << std::endl;
    
    executor->sync_node_to_external_storage(
        node2->get_output_socket("result"), result);
    ASSERT_EQ(result.cast<int>(), 12);  // Same result, node0 used cache
}

// Critical test: The issue described by the user
// up->A->B->down, then disconnect A->B and connect up->B->down
// B should get up's data, not A's old data
// Extended version with larger subgraphs to test dirty propagation
TEST_F(NodeExecTest, ReconnectUsesNewUpstreamData)
{
    NodeTreeExecutorDesc desc;
    desc.policy = NodeTreeExecutorDesc::Policy::Eager;
    auto executor_ptr = create_node_tree_executor(desc);
    auto executor = dynamic_cast<EagerNodeTreeExecutor*>(executor_ptr.get());
    
    // Create upstream subgraph: up1 -> up2 -> up3 (produces 100)
    auto up1 = tree->add_node("add");
    auto up2 = tree->add_node("add");
    auto up3 = tree->add_node("add");
    tree->add_link(up1->get_output_socket("result"), up2->get_input_socket("a"));
    tree->add_link(up2->get_output_socket("result"), up3->get_input_socket("a"));
    
    // Create middle nodes A and B
    auto A = tree->add_node("add");     // Will produce 200
    auto B = tree->add_node("add");     // Will process A's or up's output
    
    // Create downstream subgraph: down1 -> down2 -> down3 (final result)
    auto down1 = tree->add_node("add");
    auto down2 = tree->add_node("add");
    auto down3 = tree->add_node("add");
    tree->add_link(down1->get_output_socket("result"), down2->get_input_socket("a"));
    tree->add_link(down2->get_output_socket("result"), down3->get_input_socket("a"));
    
    // Initial connection: up3 -> A -> B -> down1
    tree->add_link(up3->get_output_socket("result"), A->get_input_socket("a"));
    tree->add_link(A->get_output_socket("result"), B->get_input_socket("a"));
    tree->add_link(B->get_output_socket("result"), down1->get_input_socket("a"));
    
    std::cout << "\n=== Phase 1: Initial Execution (up3->A->B->down1) ===" << std::endl;
    executor->prepare_tree(tree.get());
    
    // Set up1: 30 + 30 = 60
    executor->sync_node_from_external_storage(up1->get_input_socket("a"), 30);
    executor->sync_node_from_external_storage(up1->get_input_socket("b"), 30);
    // up2: 60 + 1 = 61
    // up3: 61 + 1 = 62
    
    // Set A.b: A will add 100 to whatever it receives, so A = 62 + 100 = 162
    executor->sync_node_from_external_storage(A->get_input_socket("b"), 100);
    
    executor->execute_tree(tree.get());
    
    // Verify initial results
    entt::meta_any result;
    executor->sync_node_to_external_storage(up1->get_output_socket("result"), result);
    std::cout << "up1 result: " << result.cast<int>() << std::endl;
    ASSERT_EQ(result.cast<int>(), 60);
    
    executor->sync_node_to_external_storage(up2->get_output_socket("result"), result);
    std::cout << "up2 result: " << result.cast<int>() << std::endl;
    ASSERT_EQ(result.cast<int>(), 61);
    
    executor->sync_node_to_external_storage(up3->get_output_socket("result"), result);
    int up3_result = result.cast<int>();
    std::cout << "up3 result: " << up3_result << std::endl;
    ASSERT_EQ(up3_result, 62);
    
    executor->sync_node_to_external_storage(A->get_output_socket("result"), result);
    int A_result = result.cast<int>();
    std::cout << "A result: " << A_result << std::endl;
    ASSERT_EQ(A_result, 162);  // 62 + 100
    
    executor->sync_node_to_external_storage(B->get_output_socket("result"), result);
    int B_result = result.cast<int>();
    std::cout << "B result: " << B_result << std::endl;
    ASSERT_EQ(B_result, 163);  // 162 + 1
    
    executor->sync_node_to_external_storage(down1->get_output_socket("result"), result);
    std::cout << "down1 result: " << result.cast<int>() << std::endl;
    ASSERT_EQ(result.cast<int>(), 164);  // 163 + 1
    
    executor->sync_node_to_external_storage(down2->get_output_socket("result"), result);
    std::cout << "down2 result: " << result.cast<int>() << std::endl;
    ASSERT_EQ(result.cast<int>(), 165);  // 164 + 1
    
    executor->sync_node_to_external_storage(down3->get_output_socket("result"), result);
    int down3_result = result.cast<int>();
    std::cout << "down3 result: " << down3_result << std::endl;
    ASSERT_EQ(down3_result, 166);  // 165 + 1
    
    std::cout << "\n=== Phase 2: Disconnect A->B ===" << std::endl;
    
    // Find and delete link A->B
    NodeLink* link_A_B = nullptr;
    for (auto& link : tree->links) {
        if (link->from_node == A && link->to_node == B) {
            link_A_B = link.get();
            break;
        }
    }
    ASSERT_NE(link_A_B, nullptr);
    tree->delete_link(link_A_B->ID);
    
    // Notify that B's input changed
    executor->notify_socket_dirty(B->get_input_socket("a"));
    
    // Check dirty state: upstream should be clean, B and downstream should be dirty
    std::cout << "Dirty states after disconnect:" << std::endl;
    std::cout << "  up1 (should be clean): " << executor->is_node_dirty(up1) << std::endl;
    std::cout << "  up2 (should be clean): " << executor->is_node_dirty(up2) << std::endl;
    std::cout << "  up3 (should be clean): " << executor->is_node_dirty(up3) << std::endl;
    std::cout << "  A (should be clean): " << executor->is_node_dirty(A) << std::endl;
    std::cout << "  B (should be dirty): " << executor->is_node_dirty(B) << std::endl;
    std::cout << "  down1 (should be dirty): " << executor->is_node_dirty(down1) << std::endl;
    std::cout << "  down2 (should be dirty): " << executor->is_node_dirty(down2) << std::endl;
    std::cout << "  down3 (should be dirty): " << executor->is_node_dirty(down3) << std::endl;
    
    ASSERT_FALSE(executor->is_node_dirty(up1));
    ASSERT_FALSE(executor->is_node_dirty(up2));
    ASSERT_FALSE(executor->is_node_dirty(up3));
    ASSERT_FALSE(executor->is_node_dirty(A));
    ASSERT_TRUE(executor->is_node_dirty(B));
    ASSERT_TRUE(executor->is_node_dirty(down1));
    ASSERT_TRUE(executor->is_node_dirty(down2));
    ASSERT_TRUE(executor->is_node_dirty(down3));
    
    std::cout << "\n=== Phase 3: Connect up3->B->down1 ===" << std::endl;
    // Now connect up3 directly to B
    tree->add_link(up3->get_output_socket("result"), B->get_input_socket("a"));
    
    // Notify that B's input changed again
    executor->notify_socket_dirty(B->get_input_socket("a"));
    
    // Check dirty state: B and downstream should still be dirty
    std::cout << "Dirty states after reconnect:" << std::endl;
    std::cout << "  up1 (should be clean): " << executor->is_node_dirty(up1) << std::endl;
    std::cout << "  up2 (should be clean): " << executor->is_node_dirty(up2) << std::endl;
    std::cout << "  up3 (should be clean): " << executor->is_node_dirty(up3) << std::endl;
    std::cout << "  A (should be clean): " << executor->is_node_dirty(A) << std::endl;
    std::cout << "  B (should be dirty): " << executor->is_node_dirty(B) << std::endl;
    std::cout << "  down1 (should be dirty): " << executor->is_node_dirty(down1) << std::endl;
    std::cout << "  down2 (should be dirty): " << executor->is_node_dirty(down2) << std::endl;
    std::cout << "  down3 (should be dirty): " << executor->is_node_dirty(down3) << std::endl;
    
    std::cout << "\n=== Phase 4: Execute with new topology ===" << std::endl;
    executor->prepare_tree(tree.get());
    executor->execute_tree(tree.get());
    
    std::cout << "\n=== Phase 5: Verify Results ===" << std::endl;
    // up chain should still be 60, 61, 62 (cached, not re-executed)
    executor->sync_node_to_external_storage(up3->get_output_socket("result"), result);
    up3_result = result.cast<int>();
    std::cout << "up3 result (should be cached 62): " << up3_result << std::endl;
    ASSERT_EQ(up3_result, 62);
    
    // A should still be 162 (cached, disconnected from B)
    executor->sync_node_to_external_storage(A->get_output_socket("result"), result);
    A_result = result.cast<int>();
    std::cout << "A result (should be cached 162): " << A_result << std::endl;
    ASSERT_EQ(A_result, 162);
    
    // B should now be 63 (up3's 62 + 1), NOT 163 (A's old 162 + 1)
    executor->sync_node_to_external_storage(B->get_output_socket("result"), result);
    B_result = result.cast<int>();
    std::cout << "B result (CRITICAL: should be 63, not 163): " << B_result << std::endl;
    ASSERT_EQ(B_result, 63) << "B should use up3's cached value (62), not A's old value (162)";
    
    // down chain should be 64, 65, 66 (B's 63 + 1, + 1, + 1)
    executor->sync_node_to_external_storage(down1->get_output_socket("result"), result);
    std::cout << "down1 result (should be 64): " << result.cast<int>() << std::endl;
    ASSERT_EQ(result.cast<int>(), 64);
    
    executor->sync_node_to_external_storage(down2->get_output_socket("result"), result);
    std::cout << "down2 result (should be 65): " << result.cast<int>() << std::endl;
    ASSERT_EQ(result.cast<int>(), 65);
    
    executor->sync_node_to_external_storage(down3->get_output_socket("result"), result);
    down3_result = result.cast<int>();
    std::cout << "down3 result (should be 66): " << down3_result << std::endl;
    ASSERT_EQ(down3_result, 66);
    
    std::cout << "\n=== Test Complete ===" << std::endl;
}
