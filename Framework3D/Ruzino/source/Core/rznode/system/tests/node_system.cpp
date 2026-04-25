#include "nodes/system/node_system.hpp"

#include <gtest/gtest.h>

#include "spdlog/spdlog.h"

using namespace Ruzino;

class MyNodeSystem : public NodeSystem {
   public:
    bool load_configuration(const std::string& config) override
    {
        return true;
    }

   private:
    std::shared_ptr<NodeTreeDescriptor> node_tree_descriptor() override
    {
        return std::make_shared<NodeTreeDescriptor>();
    }
};

TEST(NodeSystem, CreateSystem)
{
    MyNodeSystem system;
    system.init();
    ASSERT_TRUE(system.get_node_tree());
    // ASSERT_TRUE(system.get_node_tree_executor());
}

TEST(NodeSystem, LoadDyLib)
{
    // Suppress verbose logging during tests
    spdlog::set_level(spdlog::level::warn);
    
    auto dl_load_system = create_dynamic_loading_system();

    auto loaded = dl_load_system->load_configuration("test_nodes.json");

    ASSERT_TRUE(loaded);
    dl_load_system->init();
    
    // Restore log level
    spdlog::set_level(spdlog::level::info);
}

TEST(NodeSystem, LoadDyLibExecution)
{
    auto dl_load_system = create_dynamic_loading_system();

    auto loaded = dl_load_system->load_configuration("test_nodes.json");

    ASSERT_TRUE(loaded);
    dl_load_system->init();
}

TEST(NodeSystem, DynamicSockets)
{
    // Suppress verbose logging during tests
    spdlog::set_level(spdlog::level::warn);
    
    auto dl_load_system = create_dynamic_loading_system();
    auto loaded = dl_load_system->load_configuration("test_nodes.json");
    ASSERT_TRUE(loaded);
    dl_load_system->init();

    auto tree = dl_load_system->get_node_tree();

    auto node = tree->add_node("add");

    ASSERT_TRUE(node);

    // Verify initial socket count
    EXPECT_EQ(tree->nodes.size(), 1);
    EXPECT_EQ(tree->links.size(), 0);
    size_t initial_socket_count = tree->socket_count();
    EXPECT_GT(initial_socket_count, 0);

    auto socket = node->group_add_socket(
        "input_group", type_name<int>().c_str(), "a", "a", PinKind::Input);

    // Verify socket added
    EXPECT_EQ(tree->socket_count(), initial_socket_count + 1);

    node->group_remove_socket("input_group", "a", PinKind::Input);

    // Verify socket removed
    EXPECT_EQ(tree->socket_count(), initial_socket_count);
    
    // Restore log level
    spdlog::set_level(spdlog::level::info);
}
