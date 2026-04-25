#pragma once
#include <map>
#include <set>
#include <vector>

#include "entt/meta/meta.hpp"
#include "nodes/core/node_exec.hpp"
#include "nodes/core/node_tree.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
struct NodeSocket;
struct Node;

struct RuntimeInputState {
    entt::meta_any value;
    bool is_forwarded = false;
    bool is_last_used = false;
    bool keep_alive = false;
    bool is_cached = false;  // Cache validity flag
};

struct RuntimeOutputState {
    entt::meta_any value;
    bool is_last_used = false;
    bool is_cached = false;  // Cache validity flag
};

// Provide single threaded execution. The aim of this executor is simplicity and
// robustness.

class NODES_CORE_API EagerNodeTreeExecutor : public NodeTreeExecutor {
   public:
    void compile(NodeTree* tree, Node* required_node = nullptr);
    void prepare_memory();
    void prepare_tree(NodeTree* tree, Node* required_node = nullptr) override;
    void execute_tree(NodeTree* tree) override;

    entt::meta_any* FindPtr(NodeSocket* socket);
    void sync_node_from_external_storage(
        NodeSocket* socket,
        const entt::meta_any& data) override;
    void sync_node_to_external_storage(NodeSocket* socket, entt::meta_any& data)
        override;

    std::shared_ptr<NodeTreeExecutor> clone_empty() const override;

    // Override base class notification methods
    void notify_node_dirty(Node* node) override;
    void notify_socket_dirty(NodeSocket* socket) override;
    entt::meta_any* get_socket_value(NodeSocket* socket) override;

    // Dirty tracking and cache management
    void mark_node_dirty(Node* node);
    void mark_socket_dirty(NodeSocket* socket);
    void mark_tree_structure_changed()
        override;                          // Call when links added/removed
    bool is_node_dirty(Node* node) const;  // For debugging/testing

    // Get and set dirty nodes for simulation persistence
    std::set<Node*> get_dirty_nodes() const;
    void set_nodes_dirty(const std::set<Node*>& nodes);

   protected:
    virtual ExeParams prepare_params(NodeTree* tree, Node* node);
    virtual bool execute_node(NodeTree* tree, Node* node);
    virtual void remove_storage(const std::set<std::string>::value_type& key);
    void forward_output_to_input(Node* node);
    void clear();

    // Cache management
    void propagate_dirty_downstream(Node* node, NodeTree* tree);
    void collect_required_upstream(Node* node);
    void invalidate_cache_for_node(Node* node);
    void mark_node_clean(Node* node);

    std::vector<RuntimeInputState> input_states;
    std::vector<RuntimeOutputState> output_states;
    std::map<NodeSocket*, size_t> index_cache;
    std::vector<Node*> nodes_to_execute;
    std::vector<NodeSocket*> input_of_nodes_to_execute;
    std::vector<NodeSocket*> output_of_nodes_to_execute;
    ptrdiff_t nodes_to_execute_count = 0;

    // Persistent cache - survives across prepare_memory() calls
    std::map<NodeSocket*, RuntimeInputState> persistent_input_cache;
    std::map<NodeSocket*, RuntimeOutputState> persistent_output_cache;

    // Dirty tracking
    std::set<Node*> dirty_nodes;
    std::map<Node*, bool> node_dirty_cache;  // Cache dirty state per node

    // Storage related
    virtual void refresh_storage();
    virtual void try_storage();

    // Return value stands for 'this node needs be filled'. If it wants to be
    // filled but connected to wrong storage_in, then successfully_filled_data
    // is set to false.
    virtual bool try_fill_storage_to_node(
        Node* node,
        bool& successfully_filled_data);

   public:
    ~EagerNodeTreeExecutor() override;

   protected:
    std::map<std::string, entt::meta_any> storage;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
