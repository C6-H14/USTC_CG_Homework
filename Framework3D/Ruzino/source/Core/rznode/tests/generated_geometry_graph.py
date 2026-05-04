from ruzino_graph import RuzinoGraph
import os

# Auto-generated Python code from NodeTree
# This script recreates the node graph and executes it

# Create graph
g = RuzinoGraph("GeneratedGraph")
binary_dir = os.getcwd()
config_path = os.path.join(binary_dir, "geometry_nodes.json")
g.loadConfiguration(config_path)

# Create nodes
create_uv_sphere = g.createNode("create_uv_sphere", name="create_uv_sphere")
triangulate = g.createNode("triangulate", name="triangulate")

# Create connections
g.addEdge(create_uv_sphere, "Geometry", triangulate, "Input")

# Set input values and mark outputs
inputs = {
    (create_uv_sphere, "segments"): 32,
    (create_uv_sphere, "rings"): 16,
    (create_uv_sphere, "radius"): 1.000000,
}

# Mark output sockets
g.markOutput(triangulate, "Ouput")

# Execute graph
g.prepare_and_execute(inputs)

# Get outputs
triangulate_Ouput = g.getOutput(triangulate, "Ouput")
print(f"triangulate.Ouput = {triangulate_Ouput}")
