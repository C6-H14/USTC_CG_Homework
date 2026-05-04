"""
Example: Creating and executing a geometry node graph with RuzinoGraph

This example demonstrates:
1. Creating a geometry node graph
2. Connecting geometry processing nodes
3. Executing the graph
4. Serializing the graph to JSON
"""

import os
import sys

# Make sure we can import the modules
# This assumes you're running from Binaries/Debug or Release
try:
    from ruzino_graph import RuzinoGraph
    import rzgeometry_py as geom
except ImportError as e:
    print(f"Error importing modules: {e}")
    print("Make sure you're running from the Binaries/Debug or Release directory")
    print("and that the modules have been built successfully.")
    sys.exit(1)


def example_create_simple_geometry():
    """Example: Create a simple geometry node graph"""
    print("\n" + "="*60)
    print("Example 1: Simple Geometry Creation")
    print("="*60)
    
    # Create graph
    g = RuzinoGraph("SimpleGeometry")
    
    # Load geometry node definitions
    g.loadConfiguration("geometry_nodes.json")
    
    # Create a UV sphere
    sphere = g.createNode("create_uv_sphere", name="MySphere")
    
    # Mark output
    g.markOutput(sphere, "Geometry")
    
    print(f"Created graph: {g}")
    print(f"Nodes: {[n.ui_name for n in g.nodes]}")
    
    # Execute
    print("\nExecuting graph...")
    g.prepare_and_execute()
    
    # Get result
    result = g.getOutput(sphere, "Geometry")
    print(f"Result type: {type(result)}")
    
    if isinstance(result, geom.Geometry):
        mesh = result.get_mesh_component()
        if mesh:
            verts = mesh.get_vertices()
            faces = mesh.get_face_vertex_counts()
            print(f"Geometry: {len(verts)} vertices, {len(faces)} faces")


def example_geometry_pipeline():
    """Example: Multi-node geometry processing pipeline"""
    print("\n" + "="*60)
    print("Example 2: Geometry Processing Pipeline")
    print("="*60)
    
    g = RuzinoGraph("GeometryPipeline")
    g.loadConfiguration("geometry_nodes.json")
    
    # Create base geometry
    sphere = g.createNode("create_uv_sphere", name="BaseSphere")
    
    # Process it
    triangulate = g.createNode("triangulate", name="MakeTriangles")
    
    # Extract components
    decompose = g.createNode("mesh_decompose", name="ExtractData")
    
    # Connect nodes
    g.addEdge(sphere, "Geometry", triangulate, "Geometry")
    g.addEdge(triangulate, "Geometry", decompose, "Geometry")
    
    # Mark outputs
    g.markOutput(decompose, "Position")
    
    print(f"Created pipeline: {sphere.ui_name} -> {triangulate.ui_name} -> {decompose.ui_name}")
    print(f"Graph: {len(g.nodes)} nodes, {len(g.links)} links")
    
    # Execute
    print("\nExecuting pipeline...")
    g.prepare_and_execute()
    
    # Get result
    positions = g.getOutput(decompose, "Position")
    print(f"Extracted positions: {type(positions)}")


def example_serialize_graph():
    """Example: Serialize a graph to JSON"""
    print("\n" + "="*60)
    print("Example 3: Graph Serialization")
    print("="*60)
    
    g = RuzinoGraph("SerializeExample")
    g.loadConfiguration("geometry_nodes.json")
    
    # Create a simple graph
    cube = g.createNode("create_cube", name="Cube")
    transform = g.createNode("transform_geom", name="Transform")
    g.addEdge(cube, "Geometry", transform, "Geometry")
    g.markOutput(transform, "Geometry")
    
    print(f"Created graph: {len(g.nodes)} nodes, {len(g.links)} links")
    
    # Serialize
    json_str = g.serialize()
    print(f"Serialized to JSON: {len(json_str)} characters")
    
    # Save to file
    output_file = "example_geometry_graph.json"
    with open(output_file, "w") as f:
        f.write(json_str)
    print(f"Saved to: {output_file}")
    
    # Load it back
    g2 = RuzinoGraph("LoadedGraph")
    g2.loadConfiguration("geometry_nodes.json")
    
    with open(output_file, "r") as f:
        loaded_json = f.read()
    
    g2.deserialize(loaded_json)
    print(f"Loaded graph: {len(g2.nodes)} nodes, {len(g2.links)} links")
    
    # Verify
    assert len(g2.nodes) == len(g.nodes), "Node count mismatch!"
    assert len(g2.links) == len(g.links), "Link count mismatch!"
    print("✓ Serialization/deserialization successful!")


def example_python_geometry():
    """Example: Create geometry data in Python"""
    print("\n" + "="*60)
    print("Example 4: Python-Created Geometry")
    print("="*60)
    
    # Create a triangle in Python
    vertices = [
        geom.vec3(0.0, 0.0, 0.0),
        geom.vec3(1.0, 0.0, 0.0),
        geom.vec3(0.5, 1.0, 0.0)
    ]
    
    triangle = geom.create_mesh_from_arrays(
        vertices,
        [3],        # One triangular face
        [0, 1, 2]   # Indices
    )
    
    print(f"Created triangle: {triangle.to_string()}")
    
    # Access mesh data
    mesh = triangle.get_mesh_component()
    if mesh:
        verts = mesh.get_vertices()
        print(f"Vertices: {len(verts)}")
        for i, v in enumerate(verts):
            print(f"  v{i}: ({v.x}, {v.y}, {v.z})")
        
        # Add some vertex colors
        colors = [
            geom.vec3(1.0, 0.0, 0.0),  # Red
            geom.vec3(0.0, 1.0, 0.0),  # Green
            geom.vec3(0.0, 0.0, 1.0)   # Blue
        ]
        mesh.set_display_color(colors)
        print(f"Added colors to vertices")


def example_complex_graph():
    """Example: More complex geometry graph with branching"""
    print("\n" + "="*60)
    print("Example 5: Complex Graph with Multiple Paths")
    print("="*60)
    
    g = RuzinoGraph("ComplexGraph")
    g.loadConfiguration("geometry_nodes.json")
    
    # Create base geometry
    sphere = g.createNode("create_uv_sphere", name="Source")
    
    # Branch 1: Triangulate
    tri = g.createNode("triangulate", name="Triangulate")
    decompose1 = g.createNode("mesh_decompose", name="ExtractTri")
    
    # Branch 2: Transform
    transform = g.createNode("transform_geom", name="Transform")
    decompose2 = g.createNode("mesh_decompose", name="ExtractTransform")
    
    # Connect branches
    g.addEdge(sphere, "Geometry", tri, "Geometry")
    g.addEdge(tri, "Geometry", decompose1, "Geometry")
    
    g.addEdge(sphere, "Geometry", transform, "Geometry")
    g.addEdge(transform, "Geometry", decompose2, "Geometry")
    
    # Mark outputs
    g.markOutput(decompose1, "Position")
    g.markOutput(decompose2, "Position")
    
    print(f"Created complex graph:")
    print(f"  Branch 1: {sphere.ui_name} -> {tri.ui_name} -> {decompose1.ui_name}")
    print(f"  Branch 2: {sphere.ui_name} -> {transform.ui_name} -> {decompose2.ui_name}")
    print(f"  Total: {len(g.nodes)} nodes, {len(g.links)} links")
    
    # Serialize to inspect structure
    json_str = g.serialize()
    print(f"Serialized: {len(json_str)} characters")


def main():
    """Run all examples"""
    print("\n" + "="*70)
    print("  GEOMETRY NODE GRAPH EXAMPLES")
    print("="*70)
    
    try:
        example_create_simple_geometry()
        example_geometry_pipeline()
        example_serialize_graph()
        example_python_geometry()
        example_complex_graph()
        
        print("\n" + "="*70)
        print("  ALL EXAMPLES COMPLETED SUCCESSFULLY!")
        print("="*70 + "\n")
        
    except Exception as e:
        print(f"\n✗ Example failed with error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
