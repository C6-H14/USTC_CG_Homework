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
Add = g.createNode("add", name="Add")
Add_1 = g.createNode("add", name="Add")
Add_2 = g.createNode("add", name="Add")

# Create connections
g.addEdge(Add, "value", Add_2, "value")
g.addEdge(Add_1, "value", Add_2, "value2")

# Set input values and mark outputs
inputs = {
    (Add, "value"): 1,
    (Add, "value2"): 1,
    (Add_1, "value"): 1,
    (Add_1, "value2"): 1,
}

# Mark output sockets
g.markOutput(Add, "output_group")
g.markOutput(Add_1, "output_group")
g.markOutput(Add_2, "value")
g.markOutput(Add_2, "output_group")

# Execute graph
g.prepare_and_execute(inputs)

# Get outputs
Add_output_group = g.getOutput(Add, "output_group")
print(f"Add.output_group = {Add_output_group}")
Add_1_output_group = g.getOutput(Add_1, "output_group")
print(f"Add_1.output_group = {Add_1_output_group}")
Add_2_value = g.getOutput(Add_2, "value")
print(f"Add_2.value = {Add_2_value}")
Add_2_output_group = g.getOutput(Add_2, "output_group")
print(f"Add_2.output_group = {Add_2_output_group}")
