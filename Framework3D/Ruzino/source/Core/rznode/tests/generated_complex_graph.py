from ruzino_graph import RuzinoGraph
import os

# Auto-generated Python code from NodeTree
# This script recreates the node graph and executes it

# Create graph
g = RuzinoGraph("GeneratedGraph")
binary_dir = os.getcwd()
config_path = os.path.join(binary_dir, "test_nodes.json")
g.loadConfiguration(config_path)

# Create nodes
left_branch = g.createNode("add", name="left_branch")
right_branch = g.createNode("add", name="right_branch")
combiner = g.createNode("add", name="combiner")

# Create connections
g.addEdge(left_branch, "value", combiner, "value")
g.addEdge(right_branch, "value", combiner, "value2")

# Set input values and mark outputs
inputs = {
    (left_branch, "value"): 1,
    (left_branch, "value2"): 1,
    (right_branch, "value"): 1,
    (right_branch, "value2"): 1,
}

# Mark output sockets
g.markOutput(left_branch, "output_group")
g.markOutput(right_branch, "output_group")
g.markOutput(combiner, "value")
g.markOutput(combiner, "output_group")

# Execute graph
g.prepare_and_execute(inputs)

# Get outputs
left_branch_output_group = g.getOutput(left_branch, "output_group")
print(f"left_branch.output_group = {left_branch_output_group}")
right_branch_output_group = g.getOutput(right_branch, "output_group")
print(f"right_branch.output_group = {right_branch_output_group}")
combiner_value = g.getOutput(combiner, "value")
print(f"combiner.value = {combiner_value}")
combiner_output_group = g.getOutput(combiner, "output_group")
print(f"combiner.output_group = {combiner_output_group}")
