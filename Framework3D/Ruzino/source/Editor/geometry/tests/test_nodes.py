"""
Comprehensive node graph tests
Tests geometry nodes, node execution, and graph operations
"""
import sys
import os

# Add Binaries/Debug to path to find the modules
binary_dir = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'Binaries', 'Release')
sys.path.insert(0, os.path.abspath(binary_dir))

# Add rznode python path for ruzino_graph
rznode_python = os.path.join(os.path.dirname(__file__), '..', '..', '..', 'Core', 'rznode', 'python')
sys.path.insert(0, os.path.abspath(rznode_python))

# Change working directory to binary_dir so DLLs can be loaded
os.chdir(binary_dir)

from ruzino_graph import RuzinoGraph
import nodes_core_py as core
import geometry_py as geom
import numpy as np


def test_simple_add_node():
    """Test basic add node execution"""
    print("\n" + "="*70)
    print("TEST: Simple Add Node")
    print("="*70)
    
    g = RuzinoGraph("SimpleAddTest")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    
    g.loadConfiguration(config_path)
    print(f"✓ Loaded test_nodes configuration")
    
    # Create add node
    add_node = g.createNode("add", name="test_add")
    print(f"✓ Created node: {add_node.ui_name}")
    
    # Execute
    inputs = {
        (add_node, "value"): 5,
        (add_node, "value2"): 3
    }
    
    g.prepare_and_execute(inputs, required_node=add_node)
    print("✓ Executed")
    
    # Get output
    result = g.getOutput(add_node, "value")
    assert result == 8, f"Expected 8, got {result}"
    print(f"✓ Result: 5 + 3 = {result}")


def test_create_grid_geometry():
    """Test creating a grid using create_grid node"""
    print("\n" + "="*70)
    print("TEST: Create Grid Geometry")
    print("="*70)
    
    # Create graph and load geometry nodes configuration
    g = RuzinoGraph("GridGeometryTest")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    
    g.loadConfiguration(config_path)
    print(f"✓ Loaded geometry nodes configuration")
    
    # Create grid node
    grid_node = g.createNode("create_grid", name="grid")
    print(f"✓ Created node: {grid_node.ui_name}")
    
    # Mark output
    g.markOutput(grid_node, "Geometry")
    
    # Prepare inputs
    inputs = {
        (grid_node, "resolution"): 10,
        (grid_node, "size"): 5.0
    }
    print(f"  resolution=10, size=5.0")
    
    # Execute
    print(f"\n🚀 Executing graph...")
    g.prepare_and_execute(inputs, required_node=grid_node)
    print(f"✓ Executed successfully")
    
    # Get geometry result (using elegant getOutput API)
    print(f"\n📤 Getting output...")
    result = g.getOutput(grid_node, "Geometry")
    print(f"✓ Got result")
    
    # Extract Geometry object
    print(f"\n🔍 Extracting Geometry object...")
    geometry = geom.extract_geometry_from_meta_any(result)
    print(f"  ✓ Got Geometry object: {geometry}")
    
    # Get mesh component
    mesh = geometry.get_mesh_component(0)
    assert mesh is not None, "No MeshComponent found"
    print(f"  ✓ Got MeshComponent")
    
    # Verify geometry data using NumPy arrays
    print(f"\n📊 Verifying geometry data (zero-copy NumPy)...")
    
    # Get vertices
    vertices = geom.get_vertices_as_array(mesh)
    print(f"  ✓ Vertices shape: {vertices.shape}, dtype: {vertices.dtype}")
    
    # For 10x10 grid: 11x11 = 121 vertices
    expected_vertices = 121
    assert len(vertices) == expected_vertices, f"Expected {expected_vertices} vertices, got {len(vertices)}"
    print(f"  ✓ Vertex count: {expected_vertices}")
    
    # Check bounds (size=5.0, grid in YZ plane)
    min_coords = vertices.min(axis=0)
    max_coords = vertices.max(axis=0)
    print(f"  ✓ Vertex bounds: {min_coords} to {max_coords}")
    
    expected_min = [0.0, 0.0, 0.0]
    expected_max = [0.0, 5.0, 5.0]
    assert np.allclose(min_coords, expected_min, atol=0.01) and \
           np.allclose(max_coords, expected_max, atol=0.01), \
           "Vertex bounds incorrect"
    print(f"  ✓ Bounds correct ([0,0,0] to [0,5,5] in YZ plane)")
    
    # Get face indices
    faces = geom.get_face_indices_as_array(mesh)
    print(f"  ✓ Face indices shape: {faces.shape}")
    
    # 10x10 grid = 100 quads, 4 indices each = 400 indices
    expected_face_indices = 400
    assert len(faces) == expected_face_indices, f"Expected {expected_face_indices} indices, got {len(faces)}"
    print(f"  ✓ Face index count: {expected_face_indices}")
    
    print(f"\n✅ ALL CHECKS PASSED!")


def test_node_graph_chain():
    """Test chaining multiple geometry nodes"""
    print("\n" + "="*70)
    print("TEST: Node Graph Chain")
    print("="*70)
    
    g = RuzinoGraph("ChainTest")
    config_path = os.path.join(binary_dir, "geometry_nodes.json")
    
    g.loadConfiguration(config_path)
    print(f"✓ Loaded configuration")
    
    # Create two geometry nodes
    grid1 = g.createNode("create_grid", name="grid1")
    grid2 = g.createNode("create_grid", name="grid2")
    
    print(f"✓ Created nodes: {grid1.ui_name}, {grid2.ui_name}")
    
    # Mark both outputs
    g.markOutput(grid1, "Geometry")
    g.markOutput(grid2, "Geometry")
    
    # Execute with different parameters
    inputs1 = {
        (grid1, "resolution"): 5,
        (grid1, "size"): 2.0
    }
    
    inputs2 = {
        (grid2, "resolution"): 8,
        (grid2, "size"): 3.0
    }
    
    # Execute grid1
    g.prepare_and_execute(inputs1, required_node=grid1)
    result1 = g.getOutput(grid1, "Geometry")
    
    # Execute grid2
    g.prepare_and_execute(inputs2, required_node=grid2)
    result2 = g.getOutput(grid2, "Geometry")
    
    print(f"✓ Executed both nodes")
    
    # Verify both results
    geom1 = geom.extract_geometry_from_meta_any(result1)
    geom2 = geom.extract_geometry_from_meta_any(result2)
    
    mesh1 = geom1.get_mesh_component(0)
    mesh2 = geom2.get_mesh_component(0)
    
    verts1 = geom.get_vertices_as_array(mesh1)
    verts2 = geom.get_vertices_as_array(mesh2)
    
    # Grid1: 5x5 -> 6x6 = 36 vertices
    # Grid2: 8x8 -> 9x9 = 81 vertices
    assert len(verts1) == 36, f"Grid1 should have 36 vertices, got {len(verts1)}"
    assert len(verts2) == 81, f"Grid2 should have 81 vertices, got {len(verts2)}"
    
    print(f"✓ Grid1: {len(verts1)} vertices (expected 36)")
    print(f"✓ Grid2: {len(verts2)} vertices (expected 81)")
    print(f"✓ Multiple node execution verified")


if __name__ == "__main__":
    try:
        test_simple_add_node()
        test_create_grid_geometry()
        test_node_graph_chain()
        
        print("\n" + "="*70)
        print("  ALL NODE GRAPH TESTS PASSED! 🎉")
        print("="*70)
        
    except Exception as e:
        print(f"\n✗ TEST FAILED: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
