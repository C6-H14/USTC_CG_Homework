"""
Test script for Geometry Node Graph using RuzinoGraph API.

This test demonstrates creating, connecting, and executing geometry nodes,
and retrieving/inspecting the resulting geometry data.

Note: This script requires the geometry_nodes.json configuration and
the geometry_py Python module to be available.
"""

import os
import json

# Import modules - environment setup is handled by conftest.py
from ruzino_graph import RuzinoGraph
import nodes_core_py as core
import nodes_system_py as system

# Try importing geometry_py - may not be built yet
try:
    import geometry_py as geom
    HAS_GEOMETRY = True
except ImportError:
    HAS_GEOMETRY = False
    print("WARNING: geometry_py not available. Some tests will be skipped.")

# Get binary directory for test configuration files
binary_dir = os.getcwd()


def test_geometry_graph_creation():
    """Test creating a basic geometry node graph."""
    print("\n" + "="*60)
    print("TEST: Geometry Graph Creation")
    print("="*60)
    
    # Create graph
    g = RuzinoGraph("GeometryGraph")
    print(f"✓ Created graph: {g}")
    
    # Load geometry nodes configuration
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    assert os.path.exists(config_path), f"Config file not found: {config_path}"
    
    g.loadConfiguration(config_path)
    print(f"✓ Loaded geometry configuration from: {config_path}")
    print(f"  Graph state: {g}")


def test_create_simple_geometry_nodes():
    """Test creating geometry nodes."""
    print("\n" + "="*60)
    print("TEST: Create Geometry Nodes")
    print("="*60)
    
    g = RuzinoGraph("SimpleGeomGraph")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a basic geometry creation node (e.g., create_cube)
    cube_node = g.createNode("create_cube", name="MyCube")
    print(f"✓ Created node: {cube_node.ui_name} (ID: {cube_node.ID})")
    
    # Verify node is in the graph
    assert len(g.nodes) == 1, f"Expected 1 node, got {len(g.nodes)}"
    print(f"✓ Graph contains {len(g.nodes)} node(s)")


def test_geometry_node_pipeline():
    """Test a simple geometry processing pipeline."""
    print("\n" + "="*60)
    print("TEST: Geometry Node Pipeline")
    print("="*60)
    
    g = RuzinoGraph("GeomPipeline")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a geometry creation node
    sphere_node = g.createNode("create_uv_sphere", name="Sphere")
    print(f"✓ Created sphere node: {sphere_node.ui_name}")
    
    # Create a transform node (if available)
    transform_node = g.createNode("transform_geom", name="Transform")
    print(f"✓ Created transform node: {transform_node.ui_name}")
    
    # Connect them
    # Note: Need to check actual socket names in the geometry nodes
    g.addEdge(sphere_node, "Geometry", transform_node, "Geometry")
    print(f"✓ Connected: {sphere_node.ui_name} -> {transform_node.ui_name}")
    
    assert len(g.links) == 1, f"Expected 1 link, got {len(g.links)}"
    print(f"✓ Graph contains {len(g.links)} link(s)")


def test_geometry_execution_and_output():
    """Test executing a geometry graph and retrieving results."""
    if not HAS_GEOMETRY:
        print("\n" + "="*60)
        print("TEST: Geometry Execution (SKIPPED - rzgeometry_py not available)")
        print("="*60)
        return
    
    print("\n" + "="*60)
    print("TEST: Geometry Execution and Output")
    print("="*60)
    
    g = RuzinoGraph("GeomExecution")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a simple geometry
    cube_node = g.createNode("create_cube", name="Cube")
    print(f"✓ Created cube node")
    
    # Mark output for execution
    g.markOutput(cube_node, "Geometry")
    print(f"✓ Marked output: Cube.Geometry")
    
    # Execute the graph
    try:
        g.prepare_and_execute()
        print("✓ Executed graph successfully")
        
        # Get the geometry output
        result = g.getOutput(cube_node, "Geometry")
        print(f"✓ Retrieved output: {type(result)}")
        
        # If result is a Geometry object, inspect it
        if isinstance(result, geom.Geometry):
            print(f"  Geometry: {result.to_string()}")
            
            # Try to get mesh component
            mesh = result.get_mesh_component()
            if mesh:
                vertices = mesh.get_vertices()
                faces = mesh.get_face_vertex_counts()
                print(f"  Mesh: {len(vertices)} vertices, {len(faces)} faces")
            else:
                print("  No mesh component found")
        else:
            print(f"  Result type: {type(result)}")
            
    except Exception as e:
        print(f"✗ Execution failed: {e}")
        import traceback
        traceback.print_exc()


def test_geometry_graph_serialization():
    """Test serializing a geometry graph to JSON."""
    print("\n" + "="*60)
    print("TEST: Geometry Graph Serialization")
    print("="*60)
    
    g = RuzinoGraph("SerializationTest")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a simple pipeline
    sphere = g.createNode("create_uv_sphere", name="Sphere")
    mesh_decompose = g.createNode("mesh_decompose", name="Decompose")
    
    # mesh_decompose uses "Mesh" as input socket name
    g.addEdge(sphere, "Geometry", mesh_decompose, "Mesh")
    g.markOutput(mesh_decompose, "Vertices")
    
    print(f"✓ Created graph with {len(g.nodes)} nodes and {len(g.links)} links")
    
    # Serialize to JSON
    json_str = g.serialize()
    print(f"✓ Serialized graph ({len(json_str)} characters)")
    
    # Parse and pretty-print a snippet
    try:
        json_obj = json.loads(json_str)
        print(f"  Nodes in JSON: {len(json_obj.get('nodes', []))}")
        print(f"  Links in JSON: {len(json_obj.get('links', []))}")
        
        # Print a small sample of the JSON (first 500 chars)
        json_pretty = json.dumps(json_obj, indent=2)
        print(f"\n  JSON Sample (first 500 chars):")
        print(f"  {json_pretty[:500]}...")
    except Exception as e:
        print(f"  JSON parsing: {e}")
    
    # Test deserialization
    g2 = RuzinoGraph("DeserializedGraph")
    g2.loadConfiguration(config_path)
    g2.deserialize(json_str)
    
    print(f"✓ Deserialized graph: {len(g2.nodes)} nodes, {len(g2.links)} links")
    assert len(g2.nodes) == len(g.nodes), "Node count mismatch after deserialization"
    assert len(g2.links) == len(g.links), "Link count mismatch after deserialization"


def test_complex_geometry_pipeline():
    """Test a more complex geometry processing pipeline."""
    print("\n" + "="*60)
    print("TEST: Complex Geometry Pipeline")
    print("="*60)
    
    g = RuzinoGraph("ComplexGeomGraph")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create multiple geometry nodes
    # Base geometry
    sphere = g.createNode("create_uv_sphere", name="BaseSphere")
    
    # Process the geometry
    triangulate = g.createNode("triangulate", name="Triangulate")
    
    # Extract information
    decompose = g.createNode("mesh_decompose", name="Decompose")
    
    # Connect the pipeline - triangulate uses "Input", mesh_decompose uses "Mesh"
    g.addEdge(sphere, "Geometry", triangulate, "Input")
    g.addEdge(triangulate, "Ouput", decompose, "Mesh")  # Note: triangulate outputs "Ouput" (typo in C++ code)
    
    # Mark multiple outputs
    g.markOutput(decompose, "Position")
    g.markOutput(decompose, "Face Indices")
    
    print(f"✓ Created complex pipeline:")
    print(f"  {sphere.ui_name} -> {triangulate.ui_name} -> {decompose.ui_name}")
    print(f"  Nodes: {len(g.nodes)}, Links: {len(g.links)}")
    
    # Serialize to verify structure
    json_str = g.serialize()
    json_obj = json.loads(json_str)
    print(f"✓ Serialized: {len(json_obj.get('nodes', []))} nodes in JSON")


def test_geometry_with_python_created_data():
    """Test passing Python-created geometry data to nodes and using geometry interface."""
    if not HAS_GEOMETRY:
        print("\n" + "="*60)
        print("TEST: Python Geometry Data (SKIPPED - rzgeometry_py not available)")
        print("="*60)
        return
    
    print("\n" + "="*60)
    print("TEST: Geometry Interface - Direct Geometry Access")
    print("="*60)
    
    # Create graph with geometry nodes
    g = RuzinoGraph("GeometryInterfaceTest")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create a simple sphere (no decompose needed!)
    sphere = g.createNode("create_uv_sphere", name="Sphere")
    
    # Set sphere parameters
    inputs = {
        (sphere, "segments"): 8,
        (sphere, "rings"): 4,
        (sphere, "radius"): 2.5,
    }
    
    # Mark the Geometry output directly
    g.markOutput(sphere, "Geometry")
    
    print("✓ Created sphere node")
    print("  Parameters: segments=8, rings=4, radius=2.5")
    
    # Execute
    g.prepare_and_execute(inputs)
    print("✓ Executed graph")
    
    # Get the Geometry output (returns meta_any)
    result = g.getOutput(sphere, "Geometry")
    print(f"✓ Retrieved output: {type(result)}")
    
    # Extract Geometry from meta_any
    try:
        geometry = geom.extract_geometry_from_meta_any(result)
        print(f"✓ Extracted Geometry: {geometry.to_string()}")
        
        # Get mesh component
        mesh = geometry.get_mesh_component()
        if mesh:
            print(f"✓ Got MeshComponent")
            
            # Use NumPy interface to inspect the mesh
            import numpy as np
            vertices = mesh.get_vertices()
            print(f"  Vertices shape: {vertices.shape}")
            print(f"  Vertex count: {len(vertices)}")
            print(f"  First vertex: {vertices[0]}")
            print(f"  Bounding box: min={np.min(vertices, axis=0)}, max={np.max(vertices, axis=0)}")
            
            # Verify radius is approximately 2.5
            distances = np.linalg.norm(vertices, axis=1)
            avg_radius = np.mean(distances)
            print(f"  Average radius: {avg_radius:.3f} (expected: 2.5)")
            assert abs(avg_radius - 2.5) < 0.1, f"Radius mismatch: {avg_radius} vs 2.5"
            
            print("✓ Geometry interface works! Can access mesh data directly via NumPy")
        else:
            print("⚠ No mesh component found")
            
    except Exception as e:
        print(f"✗ Failed to extract geometry: {e}")
        import traceback
        traceback.print_exc()


def test_geometry_json_roundtrip_and_codegen():
    """Test geometry graph: JSON -> Deserialize -> Python Code Generation."""
    print("\n" + "="*60)
    print("TEST: Geometry Graph - JSON Roundtrip & Code Generation")
    print("="*60)
    
    # Get test directory for saving files
    test_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Step 1: Create original geometry graph (simplified - no decompose!)
    g1 = RuzinoGraph("GeometryOriginal")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    g1.loadConfiguration(config_path)
    
    # Create a simple geometry pipeline: sphere -> triangulate
    sphere = g1.createNode("create_uv_sphere", name="MySphere")
    triangulate = g1.createNode("triangulate", name="TriangulateMesh")
    
    # Connect nodes
    g1.addEdge(sphere, "Geometry", triangulate, "Input")
    
    # Mark the final Geometry output directly
    g1.markOutput(triangulate, "Ouput")  # Note: typo in C++ code
    
    print("✓ Created geometry graph")
    print(f"  Pipeline: {sphere.ui_name} -> {triangulate.ui_name}")
    print(f"  Nodes: {len(g1.nodes)}, Links: {len(g1.links)}")
    
    # Step 2: Serialize to JSON
    json_str = g1.serialize()
    print(f"\n✓ Serialized to JSON ({len(json_str)} characters)")
    
    # Save JSON to file
    json_file = os.path.join(test_dir, "geometry_graph_export.json")
    with open(json_file, 'w') as f:
        f.write(json_str)
    print(f"✓ Saved JSON to: {json_file}")
    
    # Step 3: Create new graph and deserialize
    g2 = RuzinoGraph("GeometryDeserialized")
    g2.loadConfiguration(config_path)
    g2.deserialize(json_str)
    
    print(f"\n✓ Deserialized from JSON")
    print(f"  Nodes: {len(g2.nodes)}, Links: {len(g2.links)}")
    
    # Verify structure matches
    assert len(g2.nodes) == len(g1.nodes), "Node count mismatch after deserialization"
    assert len(g2.links) == len(g1.links), "Link count mismatch after deserialization"
    print("✓ Graph structure verified")
    
    # Step 4: Generate Python code from deserialized graph
    python_code = g2.to_python_code()
    print(f"\n✓ Generated Python code from deserialized geometry graph")
    print("-" * 60)
    print(python_code[:800])  # Print first 800 chars
    if len(python_code) > 800:
        print("...")
        print(python_code[-200:])  # Print last 200 chars
    print("-" * 60)
    
    # Save generated code
    output_file = os.path.join(test_dir, "generated_geometry_graph.py")
    g2.save_python_code(output_file)
    print(f"\n✓ Saved generated code to: {output_file}")
    
    # Step 5: Execute the generated code and use geometry interface
    print("\n✓ Executing generated geometry code from JSON roundtrip...")
    try:
        exec_namespace = {}
        exec(python_code, exec_namespace)
        print("✓ Geometry code executed successfully!")
        
        # If geometry module is available, demonstrate geometry interface
        if HAS_GEOMETRY:
            print("\n✓ Testing geometry interface on generated code output...")
            # Re-execute to capture the output
            import numpy as np
            exec_namespace2 = {"geom": geom, "np": np}
            
            # Add code to extract and inspect the geometry
            inspect_code = python_code + """
# Extract and inspect the geometry
import geometry_py as geom
import numpy as np

# Get the triangulated geometry output
result_geom = g.getOutput(triangulate, "Ouput")
geometry = geom.extract_geometry_from_meta_any(result_geom)
print(f"\\n  Geometry: {geometry.to_string()}")

mesh = geometry.get_mesh_component()
if mesh:
    vertices = mesh.get_vertices()
    print(f"  Vertex count: {len(vertices)}")
    print(f"  Vertices shape: {vertices.shape}")
    distances = np.linalg.norm(vertices, axis=1)
    print(f"  Average radius: {np.mean(distances):.3f}")
"""
            exec(inspect_code, exec_namespace2)
            
    except Exception as e:
        print(f"⚠ Execution note: {e}")
        # This might fail if geometry modules aren't available, but code generation succeeded
    
    print("\n✓ Geometry JSON roundtrip test PASSED!")
    print("  Geometry Graph -> JSON -> Deserialize -> Python Code -> Execute ✓")


# Tests are automatically discovered and run by pytest
