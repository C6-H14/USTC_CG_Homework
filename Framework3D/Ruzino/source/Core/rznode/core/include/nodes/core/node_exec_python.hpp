#pragma once

#include <sstream>
#include <string>
#include <map>
#include <set>

#include "nodes/core/api.h"
#include "nodes/core/node_tree.hpp"
#include "nodes/core/node.hpp"
#include "nodes/core/socket.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE

/**
 * Python code generator for NodeTree
 * 
 * This class generates executable Python code from a NodeTree structure.
 * It traverses the node graph and generates code that:
 * - Creates all nodes with their configuration
 * - Sets up connections between nodes
 * - Executes nodes in topological order
 * - Returns output values
 * 
 * Example usage:
 *   PythonCodeGenerator generator;
 *   std::string python_code = generator.generate(tree);
 *   // Now python_code contains executable Python script
 */
class NODES_CORE_API PythonCodeGenerator {
public:
    struct Options {
        bool include_imports = true;       // Include import statements
        bool include_comments = true;      // Add explanatory comments
        bool use_graph_api = true;        // Use RuzinoGraph API (vs raw node operations)
        bool inline_simple_values = true; // Inline simple constant values
        std::string indent = "    ";      // Indentation string (4 spaces default)
    };

    PythonCodeGenerator();
    explicit PythonCodeGenerator(const Options& opts);

    /**
     * Generate Python code from a NodeTree
     * @param tree The node tree to convert to Python code
     * @param required_node Optional: only generate code for nodes needed to compute this node
     * @return Complete Python script as a string
     */
    std::string generate(const NodeTree* tree, Node* required_node = nullptr);

    /**
     * Set generation options
     */
    void set_options(const Options& opts) { options_ = opts; }
    const Options& get_options() const { return options_; }

private:
    Options options_;
    std::ostringstream code_;
    int indent_level_ = 0;
    
    // Tracking for code generation
    std::map<Node*, std::string> node_variable_names_;
    std::set<Node*> nodes_to_generate_;
    std::vector<Node*> execution_order_;
    
    // Helper methods
    void reset();
    void write_line(const std::string& line);
    void write_blank_line();
    void indent();
    void dedent();
    std::string current_indent() const;
    
    // Code generation stages
    void generate_imports();
    void generate_header_comment(const NodeTree* tree);
    void collect_required_nodes(const NodeTree* tree, Node* required_node);
    void determine_execution_order(const NodeTree* tree);
    void generate_graph_setup(const NodeTree* tree);
    void generate_node_creation(const NodeTree* tree);
    void generate_connections(const NodeTree* tree);
    void generate_input_assignments(const NodeTree* tree);
    void generate_execution();
    void generate_output_retrieval(const NodeTree* tree, Node* required_node);
    
    // Utility methods
    std::string get_node_variable_name(Node* node);
    std::string sanitize_identifier(const std::string& name);
    std::string format_value(const entt::meta_any& value);
    std::string get_socket_identifier(NodeSocket* socket);
    bool is_simple_constant(Node* node);
    bool has_upstream_connections(Node* node);
    
    // Topological sort for execution order
    void topological_sort(const NodeTree* tree, Node* start_node);
    void visit_node_dfs(Node* node, std::set<Node*>& visited, std::set<Node*>& recursion_stack);
};

/**
 * Convenience function to generate Python code from a NodeTree
 * @param tree The node tree to convert
 * @param required_node Optional: only generate code for this node and its dependencies
 * @return Python code as string
 */
NODES_CORE_API std::string to_python_code(
    const NodeTree* tree, 
    Node* required_node = nullptr);

/**
 * Convenience function with custom options
 */
NODES_CORE_API std::string to_python_code(
    const NodeTree* tree,
    const PythonCodeGenerator::Options& options,
    Node* required_node = nullptr);

RUZINO_NAMESPACE_CLOSE_SCOPE
