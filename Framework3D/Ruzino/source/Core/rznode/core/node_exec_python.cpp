#include "nodes/core/node_exec_python.hpp"

#include <algorithm>
#include <iomanip>
#include <queue>
#include <sstream>

#include "nodes/core/node_link.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE

PythonCodeGenerator::PythonCodeGenerator() : options_{}
{
}

PythonCodeGenerator::PythonCodeGenerator(const Options& opts) : options_(opts)
{
}

std::string PythonCodeGenerator::generate(
    const NodeTree* tree,
    Node* required_node)
{
    if (!tree) {
        return "# Error: null tree provided\n";
    }

    reset();

    if (options_.include_imports) {
        generate_imports();
        write_blank_line();
    }

    if (options_.include_comments) {
        generate_header_comment(tree);
        write_blank_line();
    }

    // Collect nodes to generate
    collect_required_nodes(tree, required_node);

    if (nodes_to_generate_.empty()) {
        write_line("# No nodes to generate");
        return code_.str();
    }

    // Determine execution order
    determine_execution_order(tree);

    if (options_.use_graph_api) {
        // Generate using RuzinoGraph API (cleaner, more Pythonic)
        generate_graph_setup(tree);
        write_blank_line();
        generate_node_creation(tree);
        write_blank_line();
        generate_connections(tree);
        write_blank_line();
        generate_input_assignments(tree);
        write_blank_line();
        generate_execution();
        write_blank_line();
        generate_output_retrieval(tree, required_node);
    }
    else {
        // Generate raw node operations (more verbose)
        write_line("# Raw node operations not implemented yet");
        write_line("# Use use_graph_api=true option");
    }

    return code_.str();
}

void PythonCodeGenerator::reset()
{
    code_.str("");
    code_.clear();
    indent_level_ = 0;
    node_variable_names_.clear();
    nodes_to_generate_.clear();
    execution_order_.clear();
}

void PythonCodeGenerator::write_line(const std::string& line)
{
    if (line.empty()) {
        code_ << "\n";
    }
    else {
        code_ << current_indent() << line << "\n";
    }
}

void PythonCodeGenerator::write_blank_line()
{
    code_ << "\n";
}

void PythonCodeGenerator::indent()
{
    indent_level_++;
}

void PythonCodeGenerator::dedent()
{
    if (indent_level_ > 0) {
        indent_level_--;
    }
}

std::string PythonCodeGenerator::current_indent() const
{
    std::string result;
    for (int i = 0; i < indent_level_; i++) {
        result += options_.indent;
    }
    return result;
}

void PythonCodeGenerator::generate_imports()
{
    write_line("from ruzino_graph import RuzinoGraph");
    write_line("import os");
}

void PythonCodeGenerator::generate_header_comment(const NodeTree* tree)
{
    write_line("# Auto-generated Python code from NodeTree");
    write_line("# This script recreates the node graph and executes it");
}

void PythonCodeGenerator::collect_required_nodes(
    const NodeTree* tree,
    Node* required_node)
{
    if (required_node) {
        // Only collect nodes needed for this output
        std::set<Node*> visited;
        std::queue<Node*> to_visit;
        to_visit.push(required_node);
        visited.insert(required_node);

        while (!to_visit.empty()) {
            Node* current = to_visit.front();
            to_visit.pop();
            nodes_to_generate_.insert(current);

            // Add all input dependencies
            auto inputs = current->getInputConnections();
            for (Node* input_node : inputs) {
                if (visited.find(input_node) == visited.end()) {
                    visited.insert(input_node);
                    to_visit.push(input_node);
                }
            }
        }
    }
    else {
        // Include all nodes
        for (const auto& node : tree->nodes) {
            nodes_to_generate_.insert(node.get());
        }
    }
}

void PythonCodeGenerator::determine_execution_order(const NodeTree* tree)
{
    // Use the tree's topological sort if available
    const auto& topo_order = tree->get_toposort_left_to_right();

    for (Node* node : topo_order) {
        if (nodes_to_generate_.find(node) != nodes_to_generate_.end()) {
            execution_order_.push_back(node);
        }
    }
}

void PythonCodeGenerator::generate_graph_setup(const NodeTree* tree)
{
    if (options_.include_comments) {
        write_line("# Create graph");
    }
    write_line("g = RuzinoGraph(\"GeneratedGraph\")");
    write_line("binary_dir = os.getcwd()");
    write_line("config_path = os.path.join(binary_dir, \"test_nodes.json\")");
    write_line("g.loadConfiguration(config_path)");
}

void PythonCodeGenerator::generate_node_creation(const NodeTree* tree)
{
    if (options_.include_comments) {
        write_line("# Create nodes");
    }

    for (Node* node : execution_order_) {
        std::string var_name = get_node_variable_name(node);
        std::string node_type =
            node->typeinfo ? node->typeinfo->id_name : "unknown";
        std::string ui_name = node->ui_name.empty() ? var_name : node->ui_name;

        std::ostringstream line;
        line << var_name << " = g.createNode(\"" << node_type << "\", name=\""
             << ui_name << "\")";

        write_line(line.str());
    }
}

void PythonCodeGenerator::generate_connections(const NodeTree* tree)
{
    if (options_.include_comments) {
        write_line("# Create connections");
    }

    bool has_connections = false;
    for (const auto& link : tree->links) {
        NodeSocket* from_socket = link->from_sock;
        NodeSocket* to_socket = link->to_sock;

        if (!from_socket || !to_socket)
            continue;

        Node* from_node = from_socket->node;
        Node* to_node = to_socket->node;

        // Only generate links between nodes we're including
        if (nodes_to_generate_.find(from_node) == nodes_to_generate_.end() ||
            nodes_to_generate_.find(to_node) == nodes_to_generate_.end()) {
            continue;
        }

        has_connections = true;

        std::string from_var = get_node_variable_name(from_node);
        std::string to_var = get_node_variable_name(to_node);
        std::string from_identifier = from_socket->identifier;
        std::string to_identifier = to_socket->identifier;

        std::ostringstream line;
        line << "g.addEdge(" << from_var << ", \"" << from_identifier << "\", "
             << to_var << ", \"" << to_identifier << "\")";

        write_line(line.str());
    }

    if (!has_connections && options_.include_comments) {
        write_line("# No connections in this graph");
    }
}

void PythonCodeGenerator::generate_input_assignments(const NodeTree* tree)
{
    if (options_.include_comments) {
        write_line("# Set input values and mark outputs");
    }

    write_line("inputs = {");
    indent();

    bool has_inputs = false;
    for (Node* node : execution_order_) {
        const auto& input_sockets = node->get_inputs();

        for (NodeSocket* socket : input_sockets) {
            // Skip sockets that are connected (they get values from other
            // nodes)
            if (tree->is_pin_linked(socket)) {
                continue;
            }

            // Skip sockets with no value set
            if (!socket->dataField.value) {
                continue;
            }

            // Get the actual value from the socket and format it
            std::string var_name = get_node_variable_name(node);
            std::string value_str = format_value(socket->dataField.value);

            // Skip if value is None (empty meta_any or unknown type)
            if (value_str.find("None") == 0) {
                continue;
            }

            std::ostringstream line;
            line << "(" << var_name << ", \"" << socket->identifier
                 << "\"): " << value_str << ",";
            write_line(line.str());
            has_inputs = true;
        }
    }

    if (!has_inputs) {
        write_line("# Add your input values here");
        write_line("# Example: (node, \"socket_name\"): value,");
    }

    dedent();
    write_line("}");
    write_blank_line();

    // Mark outputs
    if (options_.include_comments) {
        write_line("# Mark output sockets");
    }

    bool has_outputs = false;
    for (Node* node : execution_order_) {
        const auto& output_sockets = node->get_outputs();

        for (NodeSocket* socket : output_sockets) {
            // Mark outputs that are either:
            // 1. Not connected to anything (terminal outputs)
            // 2. Explicitly marked as outputs in the tree
            bool is_terminal = !tree->is_pin_linked(socket);

            if (is_terminal) {
                std::string var_name = get_node_variable_name(node);
                std::ostringstream line;
                line << "g.markOutput(" << var_name << ", \""
                     << socket->identifier << "\")";
                write_line(line.str());
                has_outputs = true;
            }
        }
    }

    if (!has_outputs && options_.include_comments) {
        write_line("# g.markOutput(node, \"output_socket_name\")");
    }
}

void PythonCodeGenerator::generate_execution()
{
    if (options_.include_comments) {
        write_line("# Execute graph");
    }
    write_line("g.prepare_and_execute(inputs)");
}

void PythonCodeGenerator::generate_output_retrieval(
    const NodeTree* tree,
    Node* required_node)
{
    if (options_.include_comments) {
        write_line("# Get outputs");
    }

    if (required_node) {
        // Get outputs from the required node
        const auto& outputs = required_node->get_outputs();
        std::string var_name = get_node_variable_name(required_node);

        for (NodeSocket* socket : outputs) {
            std::string result_var =
                "result_" + sanitize_identifier(socket->identifier);
            std::ostringstream line;
            line << result_var << " = g.getOutput(" << var_name << ", \""
                 << socket->identifier << "\")";
            write_line(line.str());

            if (options_.include_comments) {
                line.str("");
                line << "print(f\"" << socket->identifier << " = {"
                     << result_var << "}\")";
                write_line(line.str());
            }
        }
    }
    else {
        // Get all terminal outputs
        for (Node* node : execution_order_) {
            const auto& outputs = node->get_outputs();
            std::string var_name = get_node_variable_name(node);

            for (NodeSocket* socket : outputs) {
                bool is_terminal = !tree->is_pin_linked(socket);
                if (is_terminal) {
                    std::string result_var =
                        var_name + "_" +
                        sanitize_identifier(socket->identifier);
                    std::ostringstream line;
                    line << result_var << " = g.getOutput(" << var_name
                         << ", \"" << socket->identifier << "\")";
                    write_line(line.str());

                    if (options_.include_comments) {
                        line.str("");
                        line << "print(f\"" << var_name << "."
                             << socket->identifier << " = {" << result_var
                             << "}\")";
                        write_line(line.str());
                    }
                }
            }
        }
    }
}

std::string PythonCodeGenerator::get_node_variable_name(Node* node)
{
    auto it = node_variable_names_.find(node);
    if (it != node_variable_names_.end()) {
        return it->second;
    }

    // Generate a variable name
    std::string base_name = sanitize_identifier(node->ui_name);
    if (base_name.empty()) {
        base_name = "node";
    }

    // Make it unique
    std::string var_name = base_name;
    int counter = 1;
    while (true) {
        bool collision = false;
        for (const auto& pair : node_variable_names_) {
            if (pair.second == var_name) {
                collision = true;
                break;
            }
        }

        if (!collision) {
            break;
        }

        var_name = base_name + "_" + std::to_string(counter++);
    }

    node_variable_names_[node] = var_name;
    return var_name;
}

std::string PythonCodeGenerator::sanitize_identifier(const std::string& name)
{
    if (name.empty()) {
        return "";
    }

    std::string result;
    result.reserve(name.size());

    for (char c : name) {
        if (std::isalnum(c) || c == '_') {
            result += c;
        }
        else if (c == ' ' || c == '-') {
            result += '_';
        }
        // Skip other characters
    }

    // Ensure it doesn't start with a number
    if (!result.empty() && std::isdigit(result[0])) {
        result = "n_" + result;
    }

    return result;
}

std::string PythonCodeGenerator::format_value(const entt::meta_any& value)
{
    // Handle common types and convert to Python literals
    if (!value) {
        return "None";
    }

    auto type = value.type();

    // Integer types
    if (type == entt::resolve<int>()) {
        return std::to_string(value.cast<int>());
    }
    if (type == entt::resolve<int64_t>()) {
        return std::to_string(value.cast<int64_t>());
    }
    if (type == entt::resolve<uint32_t>()) {
        return std::to_string(value.cast<uint32_t>());
    }
    if (type == entt::resolve<uint64_t>()) {
        return std::to_string(value.cast<uint64_t>());
    }

    // Float types
    if (type == entt::resolve<float>()) {
        float f = value.cast<float>();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << f;
        return oss.str();
    }
    if (type == entt::resolve<double>()) {
        double d = value.cast<double>();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << d;
        return oss.str();
    }

    // Boolean
    if (type == entt::resolve<bool>()) {
        return value.cast<bool>() ? "True" : "False";
    }

    // String
    if (type == entt::resolve<std::string>()) {
        std::string str = value.cast<std::string>();
        // Escape quotes and backslashes
        std::string escaped;
        for (char c : str) {
            if (c == '"' || c == '\\') {
                escaped += '\\';
            }
            escaped += c;
        }
        return "\"" + escaped + "\"";
    }

    // For unknown types, return None with a comment
    return "None  # Unknown type: " + std::string(type.info().name());
}

std::string PythonCodeGenerator::get_socket_identifier(NodeSocket* socket)
{
    return socket ? socket->identifier : "";
}

bool PythonCodeGenerator::is_simple_constant(Node* node)
{
    // Check if this is a constant node with no inputs
    return node->get_inputs().empty();
}

bool PythonCodeGenerator::has_upstream_connections(Node* node)
{
    for (NodeSocket* socket : node->get_inputs()) {
        // This would need to check if the socket is linked
        // For now, return false as placeholder
    }
    return false;
}

void PythonCodeGenerator::topological_sort(
    const NodeTree* tree,
    Node* start_node)
{
    // This is handled by determine_execution_order using tree's built-in
    // toposort
}

void PythonCodeGenerator::visit_node_dfs(
    Node* node,
    std::set<Node*>& visited,
    std::set<Node*>& recursion_stack)
{
    // Helper for topological sort - not needed since we use tree's toposort
}

// Convenience functions
std::string to_python_code(const NodeTree* tree, Node* required_node)
{
    PythonCodeGenerator generator;
    return generator.generate(tree, required_node);
}

std::string to_python_code(
    const NodeTree* tree,
    const PythonCodeGenerator::Options& options,
    Node* required_node)
{
    PythonCodeGenerator generator(options);
    return generator.generate(tree, required_node);
}

RUZINO_NAMESPACE_CLOSE_SCOPE
