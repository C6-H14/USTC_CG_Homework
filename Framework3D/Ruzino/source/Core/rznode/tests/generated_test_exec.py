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
left_add = g.createNode("add", name="left_add")
right_add = g.createNode("add", name="right_add")
final_add = g.createNode("add", name="final_add")

# Create connections
g.addEdge(left_add, "value", final_add, "value")
g.addEdge(right_add, "value", final_add, "value2")

# Set input values and mark outputs
inputs = {
    (left_add, "value"): 1,
    (left_add, "value2"): 1,
    (right_add, "value"): 1,
    (right_add, "value2"): 1,
}

# Mark output sockets
g.markOutput(left_add, "output_group")
g.markOutput(right_add, "output_group")
g.markOutput(final_add, "value")
g.markOutput(final_add, "output_group")

# Execute graph
g.prepare_and_execute(inputs)

# Get outputs
left_add_output_group = g.getOutput(left_add, "output_group")
print(f"left_add.output_group = {left_add_output_group}")
right_add_output_group = g.getOutput(right_add, "output_group")
print(f"right_add.output_group = {right_add_output_group}")
final_add_value = g.getOutput(final_add, "value")
print(f"final_add.value = {final_add_value}")
final_add_output_group = g.getOutput(final_add, "output_group")
print(f"final_add.output_group = {final_add_output_group}")
