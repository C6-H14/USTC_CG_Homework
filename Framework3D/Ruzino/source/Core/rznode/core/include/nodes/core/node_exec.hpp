#pragma once

#include "entt/meta/meta.hpp"
#include "node.hpp"
#include "nodes/core/api.h"

RUZINO_NAMESPACE_OPEN_SCOPE
struct NodeTreeExecutor;
struct NodeSocket;
struct Node;
class NodeTree;

struct NODES_CORE_API ExeParams {
    const Node& node_;

    explicit ExeParams(const Node& node, entt::meta_any& g_param)
        : node_(node),
          global_param(g_param)
    {
    }

    /**
     * Get the input value for the input socket with the given identifier.
     */
    template<typename T>
    T get_input(const char* identifier) const
    {
        if constexpr (std::is_same_v<T, entt::meta_any>) {
            const int index = this->get_input_index(identifier);
            return *inputs_[index];
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            const int index = this->get_input_index(identifier);
            return std::string(inputs_[index]->cast<std::string>().c_str());
        }
        else {
            const int index = this->get_input_index(identifier);
            return inputs_[index]->cast<T>();
        }
    }

    /**
     * Get the output value for the output socket with the given identifier.
     */

    template<typename T>
    std::vector<T> get_input_group(const char* group_identifier) const
    {
        static_assert(!std::is_same_v<T, entt::meta_any>);

        std::vector<size_t> indices =
            this->get_input_group_indices(group_identifier);

        std::vector<T> values;

        for (int index : indices) {
            values.push_back(inputs_[index]->cast<T>());
        }

        return values;
    }

    std::vector<entt::meta_any*> get_input_group(
        const char* group_identifier) const
    {
        std::vector<size_t> indices =
            this->get_input_group_indices(group_identifier);
        std::vector<entt::meta_any*> values;
        for (auto index : indices) {
            values.push_back(inputs_[index]);
        }
        return values;
    }

    /**
     * Store the output value for the given socket identifier.
     */
    template<typename T>
    void set_output(const char* identifier, T&& value)
    {
        using DecayT = std::decay_t<T>;

        const int index = this->get_output_index(identifier);

        if (outputs_[index]->type()) {
            outputs_[index]->cast<DecayT&>() = std::forward<T>(value);
        }
        else {
            // CRITICAL: Use get_entt_ctx() to ensure the meta_any is created
            // with the correct context Otherwise the context might be different
            // and cause type mismatch errors
            *outputs_[index] =
                entt::meta_any{ get_entt_ctx(), std::forward<T>(value) };
        }
    }

    template<typename T>
    T get_storage()
    {
        if (!node_.storage) {
            node_.storage = get_socket_type<T>().construct();
            if constexpr (std::decay_t<T>::has_storage) {
                if (!node_.storage_info.empty()) {
                    node_.storage.cast<T&>().deserialize(node_.storage_info);
                }
            }
        }

        return node_.storage.cast<T>();
    }

    template<typename T>
    void set_storage(T&& value)
    {
        node_.storage.cast<T&>() = value;
        if constexpr (std::decay_t<T>::has_storage) {
            node_.storage_info = value.serialize();
        }
    }

    template<typename T>
    T get_global_payload()
    {
        if (!global_param) {
            throw std::runtime_error("Global payload is not set");
        }
        return global_param.cast<T>();
    }

    NodeTreeExecutor* get_executor() const
    {
        return executor;
    }

    NodeTree* get_subtree() const
    {
        return subtree;
    }

    void set_output_group(
        const char* identifier,
        const std::vector<entt::meta_any>& outputs) const
    {
        const auto indices = get_output_group_indices(identifier);
        assert(indices.size() == outputs.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            *outputs_[indices[i]] = outputs[i];
        }
    }

    void set_output_group(
        const char* identifier,
        std::vector<entt::meta_any>&& outputs) const
    {
        const auto indices = get_output_group_indices(identifier);
        assert(indices.size() == outputs.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            *outputs_[indices[i]] = std::move(outputs[i]);
        }
    }

    void set_error(const char* str) const;

    bool has_input(const char* str) const
    {
        const int index = get_input_index(str);

        auto ptr = inputs_[index];
        if (ptr) {
            return true;
        }
        return false;
    }

   private:
    int get_input_index(const char* identifier) const;
    std::vector<size_t> get_input_group_indices(
        const char* group_identifier) const;

    int get_output_index(const char* identifier);
    std::vector<size_t> get_output_group_indices(
        const char* group_identifier) const;

    friend class EagerNodeTreeExecutor;
    friend class EagerNodeTreeExecutorGeom;
    friend class EagerNodeTreeExecutorRender;

    template<typename T>
    friend T& force_get_output_to_execute(
        ExeParams& params,
        const char* identifier);

   private:
    entt::meta_any& global_param;
    std::vector<entt::meta_any*> inputs_;
    std::vector<entt::meta_any*> outputs_;

    // Subtree execution
    NodeTreeExecutor* executor;  // For node group execution
    NodeTree* subtree;
};

template<typename T>
T& force_get_output_to_execute(ExeParams& params, const char* identifier)
{
    if constexpr (std::is_same_v<T, entt::meta_any>) {
        const int index = params.get_output_index(identifier);
        return *params.outputs_[index];
    }
    else {
        const int index = params.get_output_index(identifier);
        return params.outputs_[index]->cast<T&>();
    }
}

// This executes a tree. The execution strategy is left to its children.
struct NODES_CORE_API NodeTreeExecutor {
   public:
    NodeTreeExecutor()
    {
    }

    virtual ~NodeTreeExecutor() = default;
    virtual void prepare_tree(
        NodeTree* tree,
        Node* required_node = nullptr) = 0;
    virtual void execute_tree(NodeTree* tree) = 0;
    virtual void finalize(NodeTree* tree)
    {
    }
    virtual void sync_node_from_external_storage(
        NodeSocket* socket,
        const entt::meta_any& data)
    {
    }

    virtual std::shared_ptr<NodeTreeExecutor> clone_empty() const = 0;

    virtual void sync_node_to_external_storage(
        NodeSocket* socket,
        entt::meta_any& data)
    {
    }

    // Notify executor that a node or socket has been modified
    virtual void notify_node_dirty(Node* node)
    {
        // Default: do nothing (for executors without caching)
    }

    virtual void notify_socket_dirty(NodeSocket* socket)
    {
        // Default: do nothing (for executors without caching)
    }

    // Get runtime value of a socket (for debugging/visualization)
    virtual entt::meta_any* get_socket_value(NodeSocket* socket)
    {
        return nullptr;  // Default: not supported
    }

    // Get and set dirty nodes for simulation persistence
    virtual std::set<Node*> get_dirty_nodes() const
    {
        return {};  // Default: no dirty tracking
    }

    virtual void set_nodes_dirty(const std::set<Node*>& nodes)
    {
        // Default: do nothing (for executors without dirty tracking)
    }

    void execute(NodeTree* tree, Node* required_node = nullptr)
    {
        prepare_tree(tree, required_node);
        execute_tree(tree);
    }

    template<typename T>
    T get_global_payload()
    {
        if (!global_payload) {
            global_payload = get_socket_type<T>().construct();
            if (!global_payload) {
                throw std::runtime_error(
                    "The global payload must be default constructable");
            }
        }
        return global_payload.cast<T>();
    }

    // Set global payload directly (for type-erased setting)
    void set_global_payload(const entt::meta_any& payload)
    {
        global_payload = payload;
    }

    virtual void mark_tree_structure_changed() { };

    // Reset resource allocator (for render executors)
    virtual void reset_allocator()
    {
    }

   protected:
    entt::meta_any global_payload;
};

struct NodeTreeExecutorDesc {
    enum class Policy {
        Eager,
        Lazy,
    } policy = Policy::Eager;
};
RUZINO_NAMESPACE_CLOSE_SCOPE
