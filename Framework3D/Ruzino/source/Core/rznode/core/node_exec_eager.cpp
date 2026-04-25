#include "nodes/core/node_exec_eager.hpp"

#include <algorithm>
#include <set>

#include "entt/core/any.hpp"
#include "entt/meta/resolve.hpp"
#include "nodes/core/api.h"
#include "nodes/core/node_tree.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE

void EagerNodeTreeExecutor::mark_node_dirty(Node* node)
{
    dirty_nodes.insert(node);
    node_dirty_cache[node] = true;
}

void EagerNodeTreeExecutor::mark_socket_dirty(NodeSocket* socket)
{
    mark_node_dirty(socket->node);
}

void EagerNodeTreeExecutor::notify_node_dirty(Node* node)
{
    mark_node_dirty(node);
}

void EagerNodeTreeExecutor::notify_socket_dirty(NodeSocket* socket)
{
    mark_node_dirty(socket->node);
    invalidate_cache_for_node(socket->node);

    // Propagate dirty to all downstream nodes
    std::vector<Node*> to_visit;
    for (auto* output : socket->node->get_outputs()) {
        for (auto* linked_socket : output->directly_linked_sockets) {
            to_visit.push_back(linked_socket->node);
        }
    }

    while (!to_visit.empty()) {
        Node* current = to_visit.back();
        to_visit.pop_back();

        if (is_node_dirty(current)) {
            continue;  // Already marked
        }

        mark_node_dirty(current);
        invalidate_cache_for_node(current);

        // Add downstream nodes
        for (auto* output : current->get_outputs()) {
            for (auto* linked_socket : output->directly_linked_sockets) {
                to_visit.push_back(linked_socket->node);
            }
        }
    }
}

entt::meta_any* EagerNodeTreeExecutor::get_socket_value(NodeSocket* socket)
{
    return FindPtr(socket);
}

void EagerNodeTreeExecutor::mark_tree_structure_changed()
{
    // When tree structure changes, invalidate index cache and mark all as dirty
    // This forces a full recompilation
    index_cache.clear();
    for (auto& state : input_states) {
        state.is_cached = false;
    }
    for (auto& state : output_states) {
        state.is_cached = false;
    }
    node_dirty_cache.clear();
    dirty_nodes.clear();

    persistent_input_cache.clear();
    persistent_output_cache.clear();
}

bool EagerNodeTreeExecutor::is_node_dirty(Node* node) const
{
    auto it = node_dirty_cache.find(node);
    return it != node_dirty_cache.end() && it->second;
}

void EagerNodeTreeExecutor::mark_node_clean(Node* node)
{
    node_dirty_cache[node] = false;
}

void EagerNodeTreeExecutor::invalidate_cache_for_node(Node* node)
{
    // Invalidate in current execution states
    for (auto* input : node->get_inputs()) {
        if (index_cache.find(input) != index_cache.end()) {
            auto& state = input_states[index_cache[input]];
            state.is_cached = false;
        }
        // Also invalidate in persistent cache
        auto it = persistent_input_cache.find(input);
        if (it != persistent_input_cache.end()) {
            it->second.is_cached = false;
        }
    }
    for (auto* output : node->get_outputs()) {
        if (index_cache.find(output) != index_cache.end()) {
            output_states[index_cache[output]].is_cached = false;
        }
        // Also invalidate in persistent cache
        auto it = persistent_output_cache.find(output);
        if (it != persistent_output_cache.end()) {
            it->second.is_cached = false;
        }
    }
}

void EagerNodeTreeExecutor::propagate_dirty_downstream(
    Node* node,
    NodeTree* tree)
{
    std::vector<Node*> to_visit;
    to_visit.push_back(node);

    while (!to_visit.empty()) {
        Node* current = to_visit.back();
        to_visit.pop_back();

        if (is_node_dirty(current)) {
            continue;  // Already processed
        }

        mark_node_dirty(current);
        invalidate_cache_for_node(current);

        // Propagate to downstream nodes
        for (auto* output : current->get_outputs()) {
            for (auto* linked_socket : output->directly_linked_sockets) {
                Node* downstream = linked_socket->node;
                if (!is_node_dirty(downstream)) {
                    to_visit.push_back(downstream);
                }
            }
        }
    }
}

void EagerNodeTreeExecutor::collect_required_upstream(Node* node)
{
    // Mark upstream nodes as required
    for (auto* input : node->get_inputs()) {
        for (auto* linked_socket : input->directly_linked_sockets) {
            Node* upstream = linked_socket->node;
            if (!upstream->REQUIRED) {
                upstream->REQUIRED = true;
                collect_required_upstream(upstream);
            }
        }
    }
}

ExeParams EagerNodeTreeExecutor::prepare_params(NodeTree* tree, Node* node)
{
    node->MISSING_INPUT = false;

    ExeParams params{ *node, global_payload };
    for (auto&& input : node->get_inputs()) {
        if (input->is_placeholder()) {
            continue;
        }

        entt::meta_any* input_ptr;

        if (input_states[index_cache[input]].is_forwarded) {
            // Is set by previous node
            input_ptr = &input_states[index_cache[input]].value;
        }
        else if (
            input->directly_linked_sockets.empty() && input->dataField.value) {
            auto type = input_states[index_cache[input]].value.type();
            auto value = input->dataField.value;
            // Has default value
            input_states[index_cache[input]].value = value;
            input_ptr = &input_states[index_cache[input]].value;
        }
        else if (input->optional) {
            input_ptr = nullptr;
        }
        else {
            // Node not filled. Cannot run this node.
            input_ptr = &input_states[index_cache[input]].value;
            // input_ptr.type()->default_construct(input_ptr.get());

            node->MISSING_INPUT = true;
        }
        params.inputs_.push_back(input_ptr);
    }

    for (auto&& output : node->get_outputs()) {
        entt::meta_any* output_ptr = &output_states[index_cache[output]].value;
        params.outputs_.push_back(output_ptr);
    }
    params.executor = this;
    if (node->is_node_group())
        params.subtree = static_cast<NodeGroup*>(node)->sub_tree.get();
    return params;
}

bool EagerNodeTreeExecutor::execute_node(NodeTree* tree, Node* node)
{
    bool successfully_filled_data;
    if (try_fill_storage_to_node(node, successfully_filled_data))
        return successfully_filled_data;

    ExeParams params = prepare_params(tree, node);
    if (node->MISSING_INPUT) {
        return false;
    }
    auto typeinfo = node->typeinfo;
    if (!typeinfo->node_execute(params)) {
        node->execution_failed = "Execution failed";
        return false;
    }
    node->execution_failed = {};
    return true;
}

void EagerNodeTreeExecutor::forward_output_to_input(Node* node)
{
    for (auto&& output : node->get_outputs()) {
        if (output->directly_linked_sockets.empty()) {
            auto& output_state = output_states[index_cache[output]];
            assert(output_state.is_last_used == false);
            output_state.is_last_used = true;
        }
        else {
            int last_used_id = -1;

            bool need_to_keep_alive = false;

            for (int i = 0; i < output->directly_linked_sockets.size(); ++i) {
                auto directly_linked_input_socket =
                    output->directly_linked_sockets[i];

                if (std::string(directly_linked_input_socket->node->typeinfo
                                    ->id_name) == "func_storage_in") {
                    need_to_keep_alive = true;
                }

                // Check if the downstream socket is in current execution's
                // index_cache
                auto input_cache_it =
                    index_cache.find(directly_linked_input_socket);
                if (input_cache_it != index_cache.end()) {
                    // Downstream node is in current execution
                    if (directly_linked_input_socket->node->REQUIRED) {
                        last_used_id = std::max(
                            last_used_id,
                            int(index_cache[directly_linked_input_socket]));
                    }

                    auto& input_state =
                        input_states[index_cache[directly_linked_input_socket]];
                    auto& output_state = output_states[index_cache[output]];
                    auto is_last_target =
                        i == output->directly_linked_sockets.size() - 1;

                    auto& value_to_forward = output_state.value;

                    if (!value_to_forward.type()) {
                        input_state.is_forwarded = true;
                    }

                    else if (
                        input_state.value.type() &&
                        input_state.value.type() != value_to_forward.type()) {
                        auto unequal =
                            input_state.value.type() != value_to_forward.type();
                        directly_linked_input_socket->node->execution_failed =
                            "Type mismatch input, from type " +
                            std::string(value_to_forward.type().info().name()) +
                            " to type " +
                            std::string(input_state.value.type().info().name());
                        input_state.is_forwarded = false;
                    }
                    else {
                        directly_linked_input_socket->node
                            ->execution_failed = {};

                        // Check if the value actually changed
                        bool value_changed = false;
                        if (!input_state.value || input_state.value.type() !=
                                                      value_to_forward.type()) {
                            value_changed = true;
                        }
                        else if (
                            input_state.value.type() ==
                            value_to_forward.type()) {
                            // Simple inequality check - works for basic types
                            value_changed =
                                (input_state.value != value_to_forward);
                        }

                        // Always copy to preserve cache validity
                        // Move is better in efficiency, but it breaks caching
                        // because the output value becomes empty after the move
                        input_state.value = value_to_forward;
                        input_state.is_forwarded = true;

                        // CRITICAL FIX: If value changed, mark downstream node
                        // as dirty so it will execute even if it was previously
                        // cached
                        if (value_changed) {
                            mark_node_dirty(directly_linked_input_socket->node);
                            input_state.is_cached =
                                false;  // Input is no longer cached since it
                                        // changed
                        }
                        else if (output_state.is_cached) {
                            // If value didn't change and output is cached,
                            // input can remain cached
                            input_state.is_cached = true;
                        }
                    }
                }
            }

            if (need_to_keep_alive) {
                for (int i = 0; i < output->directly_linked_sockets.size();
                     ++i) {
                    auto directly_linked_input_socket =
                        output->directly_linked_sockets[i];

                    input_states[index_cache[directly_linked_input_socket]]
                        .keep_alive = true;
                }
            }

            if (last_used_id == -1) {
                output_states[index_cache[output]].is_last_used = true;
            }
            else {
                assert(input_states[last_used_id].is_last_used == false);

                input_states[last_used_id].is_last_used = true;
            }
        }
    }

    if (node->typeinfo->id_name == "simulation_out") {
        auto simulation_in = node->paired_node;
        simulation_in->storage = std::move(node->storage);
    }
}

void EagerNodeTreeExecutor::clear()
{
    // Don't clear cache data, only reset execution state
    nodes_to_execute.clear();
    nodes_to_execute_count = 0;
    input_of_nodes_to_execute.clear();
    output_of_nodes_to_execute.clear();

    // Reset forwarding and last_used flags, but keep cached values
    for (auto& state : input_states) {
        state.is_forwarded = false;
        state.is_last_used = false;
        state.keep_alive = false;
    }

    for (auto& state : output_states) {
        state.is_last_used = false;
    }
}

void EagerNodeTreeExecutor::compile(NodeTree* tree, Node* required_node)
{
    if (tree->has_available_link_cycle) {
        return;
    }

    nodes_to_execute = tree->get_toposort_left_to_right();

    // Reset REQUIRED flags
    for (auto node : nodes_to_execute) {
        node->REQUIRED = false;
    }

    // Mark required nodes and propagate upstream
    for (int i = nodes_to_execute.size() - 1; i >= 0; i--) {
        auto node = nodes_to_execute[i];

        // Check if node is required
        if (required_node == nullptr) {
            if (node->typeinfo->ALWAYS_REQUIRED) {
                node->REQUIRED = true;
            }
        }
        else {
            if (node == required_node) {
                node->REQUIRED = true;
            }
        }

        // Propagate requirement upstream
        if (node->REQUIRED) {
            collect_required_upstream(node);
        }
    }

    // Partition into required and not-required
    auto split = std::stable_partition(
        nodes_to_execute.begin(), nodes_to_execute.end(), [](Node* node) {
            return node->REQUIRED;
        });

    nodes_to_execute_count = std::distance(nodes_to_execute.begin(), split);

    // Collect sockets for required nodes only
    for (int i = 0; i < nodes_to_execute_count; ++i) {
        input_of_nodes_to_execute.insert(
            input_of_nodes_to_execute.end(),
            nodes_to_execute[i]->get_inputs().begin(),
            nodes_to_execute[i]->get_inputs().end());

        output_of_nodes_to_execute.insert(
            output_of_nodes_to_execute.end(),
            nodes_to_execute[i]->get_outputs().begin(),
            nodes_to_execute[i]->get_outputs().end());
    }
}

void EagerNodeTreeExecutor::prepare_memory()
{
    // DON'T save current states back - they will be updated after execution
    // We only READ from persistent cache here

    // CRITICAL: Clean up persistent cache entries for deleted sockets
    // Socket group removes sockets when links are deleted, but persistent cache
    // wasn't notified This causes type mismatch when new sockets are created at
    // same group position
    {
        // Collect all valid socket pointers in the current tree
        std::unordered_set<NodeSocket*> valid_sockets;
        for (auto* socket : input_of_nodes_to_execute) {
            valid_sockets.insert(socket);
        }
        for (auto* socket : output_of_nodes_to_execute) {
            valid_sockets.insert(socket);
        }

        // Remove persistent cache entries for sockets that no longer exist
        for (auto it = persistent_input_cache.begin();
             it != persistent_input_cache.end();) {
            if (valid_sockets.find(it->first) == valid_sockets.end()) {
                it = persistent_input_cache.erase(it);
            }
            else {
                ++it;
            }
        }

        for (auto it = persistent_output_cache.begin();
             it != persistent_output_cache.end();) {
            if (valid_sockets.find(it->first) == valid_sockets.end()) {
                it = persistent_output_cache.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // Build NEW index cache and states for currently required nodes
    std::map<NodeSocket*, size_t> new_index_cache;
    std::vector<RuntimeInputState> new_input_states;
    std::vector<RuntimeOutputState> new_output_states;

    new_input_states.resize(input_of_nodes_to_execute.size());
    new_output_states.resize(output_of_nodes_to_execute.size());

    // Map inputs and preserve cached data from persistent cache
    for (int i = 0; i < input_of_nodes_to_execute.size(); ++i) {
        auto* socket = input_of_nodes_to_execute[i];
        new_index_cache[socket] = i;

        // Check if this socket existed in persistent cache
        auto old_it = persistent_input_cache.find(socket);
        if (old_it != persistent_input_cache.end()) {
            // CRITICAL: Check if cached value type matches socket's current
            // type_info This prevents type mismatch when socket is reconnected
            // to different type
            auto socket_type = socket->type_info;
            auto& cached_value = old_it->second.value;

            bool type_matches = false;
            if (socket_type && cached_value) {
                type_matches = (socket_type.id() == cached_value.type().id());
            }
            else if (!socket_type && !cached_value) {
                // Both empty, consider as matching
                type_matches = true;
            }

            if (type_matches) {
                // Type matches, safe to reuse cached value
                new_input_states[i] = std::move(old_it->second);
                // Reset execution-specific flags
                new_input_states[i].is_forwarded = false;
                new_input_states[i].is_last_used = false;
                new_input_states[i].keep_alive = false;
            }
            else {
                // Type mismatch! Discard old cached value and reinitialize
                new_input_states[i] =
                    RuntimeInputState{};  // Zero-initialize all fields
                if (socket_type) {
                    new_input_states[i].value = socket_type.construct();
                }
                new_input_states[i].is_cached = false;
            }
        }
        else {
            // New socket, initialize
            auto type = socket->type_info;
            if (type) {
                new_input_states[i].value = type.construct();
            }
            new_input_states[i].is_cached = false;
        }
    }

    // Map outputs and preserve cached data from persistent cache
    for (int i = 0; i < output_of_nodes_to_execute.size(); ++i) {
        auto* socket = output_of_nodes_to_execute[i];
        new_index_cache[socket] = i;

        // Check if this socket existed in persistent cache
        auto old_it = persistent_output_cache.find(socket);
        if (old_it != persistent_output_cache.end()) {
            // CRITICAL: Check if cached value type matches socket's current
            // type_info
            auto socket_type = socket->type_info;
            auto& cached_value = old_it->second.value;

            bool type_matches = false;
            if (socket_type && cached_value) {
                type_matches = (socket_type.id() == cached_value.type().id());
            }
            else if (!socket_type && !cached_value) {
                // Both empty, consider as matching
                type_matches = true;
            }

            if (type_matches) {
                // Type matches, safe to reuse cached value
                new_output_states[i] = std::move(old_it->second);
                // Reset execution-specific flags
                new_output_states[i].is_last_used = false;
            }
            else {
                // Type mismatch! Discard old cached value and reinitialize
                new_output_states[i] =
                    RuntimeOutputState{};  // Zero-initialize all fields
                if (socket_type) {
                    new_output_states[i].value = socket_type.construct();
                }
                new_output_states[i].is_cached = false;
            }
        }
        else {
            // New socket, initialize
            auto type = socket->type_info;
            if (type) {
                new_output_states[i].value = type.construct();
            }
            new_output_states[i].is_cached = false;
        }
    }

    // Replace old state with new state
    index_cache = std::move(new_index_cache);
    input_states = std::move(new_input_states);
    output_states = std::move(new_output_states);
}

void EagerNodeTreeExecutor::remove_storage(
    const std::set<std::string>::value_type& key)
{
    storage.erase(key);
}

void EagerNodeTreeExecutor::refresh_storage()
{
    std::set<std::string> refreshed;

    // After executing the tree, storage all the required info
    for (int i = 0; i < input_of_nodes_to_execute.size(); ++i) {
        auto socket = input_of_nodes_to_execute[i];
        if (!socket->type_info) {
            if (std::string(socket->node->typeinfo->id_name) ==
                "func_storage_in") {
                auto node = socket->node;
                entt::meta_any data;
                if (!socket->directly_linked_sockets.empty()) {
                    auto input = node->get_inputs()[0];
                    std::string name =
                        input->default_value_typed<std::string>();
                    if (storage.find(name) == storage.end()) {
                        data = socket->directly_linked_sockets[0]
                                   ->type_info.construct();
                        storage[name] = data;
                    }
                    refreshed.emplace(name);
                }
            }
        }
    }

    std::set<std::string> keysToDelete;
    for (auto&& value : storage) {
        if (!refreshed.contains(value.first)) {
            keysToDelete.emplace(value.first);
        }
    }
    for (auto& key : keysToDelete) {
        remove_storage(key);
    }
    refreshed.clear();
}

void EagerNodeTreeExecutor::try_storage()
{
    // After executing the tree, storage all the required info
    for (int i = 0; i < input_of_nodes_to_execute.size(); ++i) {
        auto socket = input_of_nodes_to_execute[i];
        if (!socket->type_info) {
            if (std::string(socket->node->typeinfo->id_name) ==
                "func_storage_in") {
                auto node = socket->node;
                entt::meta_any data;
                sync_node_to_external_storage(
                    input_of_nodes_to_execute[i], data);

                auto input = node->get_inputs()[0];
                std::string name = input->default_value_typed<std::string>();
                storage[name] = data;
            }
        }
    }
}

bool EagerNodeTreeExecutor::try_fill_storage_to_node(
    Node* node,
    bool& successfully_filled_data)
{
    // Identify the special storage node, and do a special execution here.

    if (node->REQUIRED) {  // requirement info is valid.
        if (std::string(node->typeinfo->id_name) == "func_storage_out") {
            auto input = node->get_inputs()[0];
            std::string name = input->default_value_typed<std::string>();
            if (storage.find(name) != storage.end()) {
                auto& storaged_value = storage.at(name);

                // Check all the connected input type

                for (auto input :
                     node->get_outputs()[0]->directly_linked_sockets) {
                    if (storaged_value.type() &&
                        storaged_value.type() !=
                            input_states[index_cache[input]].value.type()) {
                        node->execution_failed =
                            "Type Mismatch, filling default value.";
                        successfully_filled_data = false;
                        return true;
                    }
                }

                output_states[index_cache[node->get_outputs()[0]]].value =
                    storaged_value;

                node->execution_failed = {};
                successfully_filled_data = true;
                return true;
            }
            else {
                node->execution_failed =
                    "No cache can be found with name " + name + " (yet).";
                successfully_filled_data = false;
                return true;
            }
        }
    }
    return false;
}

EagerNodeTreeExecutor::~EagerNodeTreeExecutor()
{
    storage.clear();
}

void EagerNodeTreeExecutor::prepare_tree(NodeTree* tree, Node* required_node)
{
    tree->ensure_topology_cache();

    // Only clear execution state, not cache
    clear();

    compile(tree, required_node);

    // prepare_memory will now handle resizing and cache preservation
    prepare_memory();

    refresh_storage();
}

void EagerNodeTreeExecutor::execute_tree(NodeTree* tree)
{
    for (int i = 0; i < nodes_to_execute_count; ++i) {
        auto node = nodes_to_execute[i];

        // ALWAYS_DIRTY nodes must always execute and propagate dirty state
        // downstream
        bool force_execute = node->typeinfo->ALWAYS_DIRTY;

        // Skip execution if node is clean and has valid cache (unless
        // ALWAYS_DIRTY)
        if (!force_execute && !is_node_dirty(node)) {
            int cached_inputs = 0, total_inputs = 0;
            int cached_outputs = 0, total_outputs = 0;

            for (auto* input : node->get_inputs()) {
                if (index_cache.find(input) != index_cache.end()) {
                    total_inputs++;
                    if (input_states[index_cache[input]].is_cached) {
                        cached_inputs++;
                    }
                }
            }

            for (auto* output : node->get_outputs()) {
                if (index_cache.find(output) != index_cache.end()) {
                    total_outputs++;
                    if (output_states[index_cache[output]].is_cached) {
                        cached_outputs++;
                    }
                }
            }

            bool all_cached = (cached_inputs == total_inputs) &&
                              (cached_outputs == total_outputs);

            if (all_cached && total_inputs > 0 && total_outputs > 0) {
                // Node is clean and cached, forward cached outputs
                forward_output_to_input(node);
                continue;
            }
        }

        // Execute node
        auto result = execute_node(tree, node);
        if (result) {
            forward_output_to_input(node);

            // ALWAYS_DIRTY nodes should invalidate downstream nodes
            if (node->typeinfo->ALWAYS_DIRTY) {
                // Mark all downstream nodes as dirty
                for (auto* output : node->get_outputs()) {
                    for (auto* linked_socket :
                         output->directly_linked_sockets) {
                        mark_node_dirty(linked_socket->node);
                        invalidate_cache_for_node(linked_socket->node);
                    }
                }
            }

            // Mark node as clean and cache as valid (unless ALWAYS_DIRTY)
            if (!node->typeinfo->ALWAYS_DIRTY) {
                mark_node_clean(node);
            }
            for (auto* input : node->get_inputs()) {
                if (index_cache.find(input) != index_cache.end()) {
                    input_states[index_cache[input]].is_cached = true;
                }
            }
            for (auto* output : node->get_outputs()) {
                if (index_cache.find(output) != index_cache.end()) {
                    output_states[index_cache[output]].is_cached = true;
                }
            }
        }
    }

    try_storage();

    // Save all current states back to persistent cache for next execution
    // IMPORTANT: Copy, not move! We need the values to remain accessible for
    // sync_node_to_external_storage
    for (const auto& [socket, index] : index_cache) {
        if (socket->in_out == PinKind::Input && index < input_states.size()) {
            persistent_input_cache[socket] = input_states[index];  // Copy
        }
        else if (
            socket->in_out == PinKind::Output && index < output_states.size()) {
            persistent_output_cache[socket] = output_states[index];  // Copy
        }
    }

    // Clean up dirty nodes that were executed, but DON'T clear nodes marked
    // dirty during execution (e.g., downstream nodes that got updated values
    // via forward_output_to_input)
    std::set<Node*> nodes_to_keep_dirty;
    for (auto* dirty_node : dirty_nodes) {
        // If the node was not in the execution list, keep it dirty for next
        // time
        bool was_executed = false;
        for (int i = 0; i < nodes_to_execute_count; ++i) {
            if (nodes_to_execute[i] == dirty_node) {
                was_executed = true;
                break;
            }
        }
        if (!was_executed) {
            nodes_to_keep_dirty.insert(dirty_node);
        }
    }
    dirty_nodes = nodes_to_keep_dirty;
}

entt::meta_any* EagerNodeTreeExecutor::FindPtr(NodeSocket* socket)
{
    entt::meta_any* ptr;
    if (socket->in_out == PinKind::Input) {
        if (index_cache.find(socket) != index_cache.end()) {
            ptr = &input_states[index_cache[socket]].value;
        }
        else {
            // Check persistent cache
            auto it = persistent_input_cache.find(socket);
            if (it != persistent_input_cache.end()) {
                return &it->second.value;
            }
            static entt::meta_any default_any;
            return &default_any;
        }
    }
    else {
        if (index_cache.find(socket) != index_cache.end()) {
            ptr = &output_states[index_cache[socket]].value;
        }
        else {
            // Check persistent cache
            auto it = persistent_output_cache.find(socket);
            if (it != persistent_output_cache.end()) {
                return &it->second.value;
            }
            static entt::meta_any default_any;
            return &default_any;
        }
    }
    return ptr;
}

void EagerNodeTreeExecutor::sync_node_from_external_storage(
    NodeSocket* socket,
    const entt::meta_any& data)
{
    if (index_cache.find(socket) != index_cache.end()) {
        entt::meta_any* ptr = FindPtr(socket);

        // Check if data actually changed
        bool data_changed =
            !(*ptr) || (*ptr).type() != data.type() || *ptr != data;

        *ptr = data;

        // if it has dataField, fill it
        if (socket->in_out == PinKind::Input) {
            if (socket->dataField.value) {
                socket->dataField.value = data;
            }
            input_states[index_cache[socket]].is_forwarded = true;
            input_states[index_cache[socket]].is_cached = false;

            // Mark node and downstream nodes dirty if data changed
            if (data_changed) {
                mark_node_dirty(socket->node);
                invalidate_cache_for_node(socket->node);

                // Propagate dirty to downstream nodes
                for (auto* output : socket->node->get_outputs()) {
                    for (auto* linked_socket :
                         output->directly_linked_sockets) {
                        Node* downstream = linked_socket->node;
                        if (!is_node_dirty(downstream)) {
                            mark_node_dirty(downstream);
                            invalidate_cache_for_node(downstream);

                            // Recursively propagate downstream
                            std::vector<Node*> to_visit = { downstream };
                            while (!to_visit.empty()) {
                                Node* current = to_visit.back();
                                to_visit.pop_back();

                                for (auto* out : current->get_outputs()) {
                                    for (auto* linked :
                                         out->directly_linked_sockets) {
                                        Node* next = linked->node;
                                        if (!is_node_dirty(next)) {
                                            mark_node_dirty(next);
                                            invalidate_cache_for_node(next);
                                            to_visit.push_back(next);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void EagerNodeTreeExecutor::sync_node_to_external_storage(
    NodeSocket* socket,
    entt::meta_any& data)
{
    if (index_cache.find(socket) != index_cache.end()) {
        const entt::meta_any* ptr = FindPtr(socket);
        data = *ptr;
    }
}

std::shared_ptr<NodeTreeExecutor> EagerNodeTreeExecutor::clone_empty() const
{
    return std::make_shared<EagerNodeTreeExecutor>();
}

std::set<Node*> EagerNodeTreeExecutor::get_dirty_nodes() const
{
    return dirty_nodes;
}

void EagerNodeTreeExecutor::set_nodes_dirty(const std::set<Node*>& nodes)
{
    for (auto* node : nodes) {
        mark_node_dirty(node);
        invalidate_cache_for_node(node);
    }
}

RUZINO_NAMESPACE_CLOSE_SCOPE
