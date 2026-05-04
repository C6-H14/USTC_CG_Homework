"""
Test script for RuzinoGraph - A clean, Falcor-inspired Python API.

This test demonstrates the usage of the new RuzinoGraph API and validates
that nodes can be created, connected, executed, and their outputs retrieved.

Note: This script can be run from any directory. It will automatically change
the working directory to Binaries/Debug to ensure node DLLs can be loaded.
"""

import os

# Import modules - environment setup is handled by conftest.py
from ruzino_graph import RuzinoGraph
import nodes_core_py as core
import nodes_system_py as system

# Get binary directory for test configuration files
# conftest.py has already changed working directory to binary_dir
binary_dir = os.getcwd()


def test_basic_graph():
    """Test basic graph creation and execution."""
    print("\n" + "="*60)
    print("TEST: Basic Graph Creation and Execution")
    print("="*60)
    
    # Create graph
    g = RuzinoGraph("TestGraph")
    print(f"✓ Created graph: {g}")
    
    # Load configuration (assuming test_nodes.json exists)
    config_path = os.path.join(binary_dir, "test_nodes.json")
    assert os.path.exists(config_path), f"Config file not found: {config_path}"
    
    g.loadConfiguration(config_path)
    print(f"✓ Loaded configuration from: {config_path}")
    print(f"  Graph state: {g}")


def test_node_creation():
    """Test node creation with parameters."""
    print("\n" + "="*60)
    print("TEST: Node Creation")
    print("="*60)
    
    g = RuzinoGraph("NodeCreationTest")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create nodes
    add1 = g.createNode("add", name="add_node_1")
    print(f"✓ Created node: {add1.ui_name} (ID: {add1.ID})")
    
    add2 = g.createNode("add", name="add_node_2")
    print(f"✓ Created node: {add2.ui_name} (ID: {add2.ID})")
    
    # Verify nodes are in the graph
    assert len(g.nodes) == 2, f"Expected 2 nodes, got {len(g.nodes)}"
    print(f"✓ Graph contains {len(g.nodes)} nodes")


def test_edge_creation():
    """Test creating edges between nodes."""
    print("\n" + "="*60)
    print("TEST: Edge Creation")
    print("="*60)
    
    g = RuzinoGraph("EdgeCreationTest")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create nodes
    add1 = g.createNode("add", name="add1")
    add2 = g.createNode("add", name="add2")
    print(f"✓ Created nodes: {add1.ui_name}, {add2.ui_name}")
    
    # Create edge (add node has: inputs "value", "value2"; output "value")
    link = g.addEdge(add1, "value", add2, "value")
    print(f"✓ Created edge: {add1.ui_name}.value -> {add2.ui_name}.value")
    
    # Verify link exists
    assert len(g.links) == 1, f"Expected 1 link, got {len(g.links)}"
    print(f"✓ Graph contains {len(g.links)} link(s)")
    
    # Test edge creation by name
    add3 = g.createNode("add", name="add3")
    g.addEdge("add2", "value", "add3", "value")
    print(f"✓ Created edge by name: add2.value -> add3.value")
    
    assert len(g.links) == 2, f"Expected 2 links, got {len(g.links)}"
    print(f"✓ Graph contains {len(g.links)} links")


def test_execution():
    """Test graph execution."""
    print("\n" + "="*60)
    print("TEST: Graph Execution")
    print("="*60)
    
    g = RuzinoGraph("ExecutionTest")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a simple computation graph: (5 + 3) -> second add with value2=10
    add1 = g.createNode("add", name="add1")
    add2 = g.createNode("add", name="add2")
    g.addEdge(add1, "value", add2, "value")
    print("✓ Created graph: add1.value -> add2.value")
    
    # Mark output
    g.markOutput(add2, "value")
    print("✓ Marked output: add2.value")
    
    # Execute with inputs in correct order (prepare -> set inputs -> execute)
    g.prepare_and_execute({
        (add1, "value"): 5,
        (add1, "value2"): 3,
        (add2, "value2"): 10
    })
    print("✓ Executed graph with inputs: add1(value=5, value2=3), add2(value2=10)")
    
    # Get output
    result = g.getOutput(add2, "value")
    expected = 18
    
    # Validate the result
    assert result == expected, f"Expected {expected}, got {result}"
    print(f"✓ Got correct output: {result} (expected: {expected})")
    print(f"  Computation: (5 + 3) + 10 = {result}")


def test_falcor_style_api():
    """Test Falcor-style API usage."""
    print("\n" + "="*60)
    print("TEST: Falcor-Style API")
    print("="*60)
    
    # Falcor-style graph construction
    g = RuzinoGraph("FalcorStyleTest")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Chain method calls (Falcor style)
    add1 = g.createNode("add", name="Add1")
    add2 = g.createNode("add", name="Add2")
    add3 = g.createNode("add", name="Add3")
    
    (g.addEdge(add1, "value", add2, "value")
      .addEdge(add2, "value", add3, "value")
      .markOutput(add3, "value"))
    
    print("✓ Built graph with method chaining (Falcor style)")
    print(f"  Graph: {g}")
    print(f"  Computation: ((1+2)+3)+4 = 10")
    
    # Execute with inputs
    g.prepare_and_execute({
        (add1, "value"): 1,
        (add1, "value2"): 2,
        (add2, "value2"): 3,
        (add3, "value2"): 4
    })
    print("✓ Executed graph")
    
    result = g.getOutput(add3, "value")
    expected = 10  # (1+2) + 3 + 4 = 10
    
    assert result == expected, f"Expected {expected}, got {result}"
    print(f"✓ Got correct output: {result} (expected: {expected})")


def test_complex_graph():
    """Test more complex graph with multiple branches."""
    print("\n" + "="*60)
    print("TEST: Complex Multi-Branch Graph")
    print("="*60)
    
    g = RuzinoGraph("ComplexGraph")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a graph like:
    #     add1 (5+3=8)
    #    /    \
    # add2    add3
    # (+10)   (+20)
    #   |       |
    #  (18)    (28)
    
    add1 = g.createNode("add", name="source")
    add2 = g.createNode("add", name="branch1")
    add3 = g.createNode("add", name="branch2")
    
    # Connect
    g.addEdge(add1, "value", add2, "value")
    g.addEdge(add1, "value", add3, "value")
    
    # Mark both outputs
    g.markOutput(add2, "value")
    g.markOutput(add3, "value")
    
    print("✓ Created multi-branch graph")
    print("  source(5+3) -> branch1(+10) = 18")
    print("  source(5+3) -> branch2(+21) = 29")
    
    # For multi-branch graphs, we need to execute to each output separately
    # since prepare_tree only accepts ONE required_node
    inputs = {
        (add1, "value"): 5,
        (add1, "value2"): 3,
        (add2, "value2"): 10,
        (add3, "value2"): 21
    }
    
    # Execute to branch1
    g.prepare_and_execute(inputs, required_node=add2)
    result1 = g.getOutput(add2, "value")
    
    # Execute to branch2
    g.prepare_and_execute(inputs, required_node=add3)
    result2 = g.getOutput(add3, "value")
    
    print("✓ Executed graph (to each branch)")
    
    expected1 = 18  # (5 + 3) + 10 = 18
    expected2 = 29  # (5 + 3) + 21 = 29
    
    assert result1 == expected1, f"Branch 1: expected {expected1}, got {result1}"
    assert result2 == expected2, f"Branch 2: expected {expected2}, got {result2}"
    print(f"✓ Got correct outputs:")
    print(f"  Branch 1: {result1} (expected: {expected1})")
    print(f"  Branch 2: {result2} (expected: {expected2})")


# Tests are automatically discovered and run by pytest
# No need for a custom test runner
