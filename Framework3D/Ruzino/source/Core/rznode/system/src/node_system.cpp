#include "nodes/system/node_system.hpp"

#include "entt/meta/meta.hpp"
#include "nodes/core/node_exec_eager.hpp"
#include "nodes/system/node_system_dl.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
void NodeSystem::init()
{
    this->node_tree = create_node_tree(node_tree_descriptor());
    if (!node_tree_executor)
        this->node_tree_executor = create_node_tree_executor({});
}

void NodeSystem::init(std::unique_ptr<NodeTree> tree)
{
    this->node_tree = std::move(tree);
    if (!node_tree_executor)
        this->node_tree_executor = create_node_tree_executor({});
}

void NodeSystem::set_node_tree_executor(
    std::unique_ptr<NodeTreeExecutor> executor)
{
    node_tree_executor = std::move(executor);
}

NodeSystem::~NodeSystem()
{
}

void NodeSystem::finalize()
{
    if (node_tree_executor) {
        node_tree_executor->finalize(node_tree.get());
    }
}

void NodeSystem::execute(bool is_ui_execution, Node* required_node) const
{
    if (is_ui_execution && !allow_ui_execution) {
        return;
    }
    if (node_tree_executor) {
        return node_tree_executor->execute(node_tree.get(), required_node);
    }
}

NodeTree* NodeSystem::get_node_tree() const
{
    return node_tree.get();
}

NodeTreeExecutor* NodeSystem::get_node_tree_executor() const
{
    return node_tree_executor.get();
}

const std::vector<std::string>& NodeSystem::get_loaded_configs() const
{
    return loaded_config_files;
}

void NodeSystem::set_global_params_any(const entt::meta_any& params)
{
    // The params meta_any already contains the actual type (e.g., GeomPayload)
    // We need to assign it directly to the executor's global_payload
    // Use the new setter method to avoid access protection
    node_tree_executor->set_global_payload(params);
}

std::shared_ptr<NodeSystem> create_dynamic_loading_system()
{
    return std::make_shared<NodeDynamicLoadingSystem>();
}

RUZINO_NAMESPACE_CLOSE_SCOPE
