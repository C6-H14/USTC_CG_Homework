"""
Comprehensive geometry_py functionality tests
Tests vec3, mesh creation, NumPy arrays, and vertex colors
"""
import sys
import os

# Add Binaries/Debug to path to find the modules
binary_dir = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'Binaries', 'Release')
sys.path.insert(0, os.path.abspath(binary_dir))

import numpy as np
import geometry_py as geom


def test_vec3_operations():
    """Test vec3 basic operations"""
    print("\n" + "="*60)
    print("TEST: vec3 Operations")
    print("="*60)
    
    v1 = geom.vec3(1.0, 2.0, 3.0)
    v2 = geom.vec3(4.0, 5.0, 6.0)
    
    print(f"v1 = {v1}")
    print(f"v2 = {v2}")
    
    # Test access
    print(f"\nAccess test:")
    print(f"   v1.x = {v1.x}, v1.y = {v1.y}, v1.z = {v1.z}")
    print(f"   v1[0] = {v1[0]}, v1[1] = {v1[1]}, v1[2] = {v1[2]}")
    
    # Test modification
    v1.x = 10.0
    v1[1] = 20.0
    print(f"\nAfter modification:")
    print(f"   v1 = {v1}")
    
    assert v1.x == 10.0, "x modification failed"
    assert v1.y == 20.0, "y modification failed"
    assert v1.z == 3.0, "z should be unchanged"
    
    print("✓ vec3 operations work correctly")


def test_empty_geometry():
    """Test creating empty geometry"""
    print("\n" + "="*60)
    print("TEST: Empty Geometry")
    print("="*60)
    
    # Create empty mesh
    empty_mesh = geom.CreateMesh()
    print(f"✓ Created empty mesh: {empty_mesh.to_string()}")
    
    mesh = empty_mesh.get_mesh_component()
    if mesh:
        verts = mesh.get_vertices()
        print(f"   Empty mesh has {len(verts)} vertices")
        assert len(verts) == 0, "Empty mesh should have 0 vertices"
    
    # Create empty points
    empty_points = geom.CreatePoints()
    print(f"✓ Created empty points: {empty_points.to_string()}")
    
    points = empty_points.get_points_component()
    if points:
        pts = points.get_vertices()
        print(f"   Empty points has {len(pts)} points")
        assert len(pts) == 0, "Empty points should have 0 points"


def test_create_triangle():
    """Test creating a triangle with vertex colors"""
    print("\n" + "="*60)
    print("TEST: Create Triangle with Colors")
    print("="*60)
    
    # Create vertices
    vertices = [
        geom.vec3(0.0, 0.0, 0.0),
        geom.vec3(1.0, 0.0, 0.0),
        geom.vec3(0.5, 1.0, 0.0)
    ]
    
    print(f"✓ Created {len(vertices)} vertices")
    
    # Create mesh
    triangle = geom.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()
    assert mesh is not None, "Failed to get mesh component"
    
    print(f"✓ Created triangle: {triangle.to_string()}")
    
    # Test vertices
    verts = mesh.get_vertices()
    assert len(verts) == 3, "Should have 3 vertices"
    
    # Add vertex colors
    colors = [
        geom.vec3(1.0, 0.0, 0.0),  # Red
        geom.vec3(0.0, 1.0, 0.0),  # Green
        geom.vec3(0.0, 0.0, 1.0),  # Blue
    ]
    mesh.set_display_color(colors)
    print(f"✓ Set vertex colors")
    
    # Verify colors
    colors_back = mesh.get_display_color()
    assert len(colors_back) == 3, "Should have 3 colors"
    print(f"✓ Retrieved {len(colors_back)} colors")


def test_create_quad():
    """Test creating a quad mesh with face data"""
    print("\n" + "="*60)
    print("TEST: Create Quad with Face Data")
    print("="*60)
    
    # Create a quad
    vertices = [
        geom.vec3(0.0, 0.0, 0.0),
        geom.vec3(1.0, 0.0, 0.0),
        geom.vec3(1.0, 1.0, 0.0),
        geom.vec3(0.0, 1.0, 0.0),
    ]
    
    quad = geom.create_mesh_from_arrays(vertices, [4], [0, 1, 2, 3])
    mesh = quad.get_mesh_component()
    
    print(f"✓ Created quad: {quad.to_string()}")
    
    # Check face data
    face_counts = mesh.get_face_vertex_counts()
    face_indices = mesh.get_face_vertex_indices()
    
    print(f"📋 Face data:")
    print(f"   Face counts: {face_counts}")
    print(f"   Face indices: {face_indices}")
    
    assert face_counts == [4], "Should have one quad face"
    assert face_indices == [0, 1, 2, 3], "Face indices mismatch"
    
    # Check bounds
    verts = mesh.get_vertices()
    vert_array = np.array([[v.x, v.y, v.z] for v in verts])
    assert np.allclose(vert_array.min(axis=0), [0, 0, 0]), "Min bound wrong"
    assert np.allclose(vert_array.max(axis=0), [1, 1, 0]), "Max bound wrong"
    
    print("✓ Quad geometry verified")


def test_numpy_arrays():
    """Test NumPy zero-copy array operations"""
    print("\n" + "="*60)
    print("TEST: NumPy Zero-Copy Arrays")
    print("="*60)
    
    # Create triangle
    vertices = [
        geom.vec3(0.0, 0.0, 0.0),
        geom.vec3(1.0, 0.0, 0.0),
        geom.vec3(0.5, 1.0, 0.0)
    ]
    
    triangle = geom.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()
    
    print("✓ Created triangle mesh")
    
    # Get vertices as numpy array
    vert_array = geom.get_vertices_as_array(mesh)
    
    print(f"📊 NumPy Vertex Array:")
    print(f"   Shape: {vert_array.shape}")
    print(f"   Dtype: {vert_array.dtype}")
    
    assert vert_array.shape == (3, 3), "Shape should be (3, 3)"
    assert vert_array.dtype == np.float32, "Dtype should be float32"
    assert np.allclose(vert_array[0], [0.0, 0.0, 0.0]), "Vertex 0 mismatch"
    
    print("✓ Array retrieval works")
    
    # Modify via numpy
    new_vertices = np.array([
        [1.0, 0.0, 0.0],
        [2.0, 0.0, 0.0],
        [1.5, 2.0, 0.0]
    ], dtype=np.float32)
    
    geom.set_vertices_from_array(mesh, new_vertices)
    print("✓ Set vertices from NumPy array")
    
    # Verify update
    updated = geom.get_vertices_as_array(mesh)
    assert np.allclose(updated, new_vertices), "Vertices not updated correctly"
    print("✓ Zero-copy update successful")


def test_numpy_face_indices():
    """Test NumPy face index arrays"""
    print("\n" + "="*60)
    print("TEST: NumPy Face Indices")
    print("="*60)
    
    # Create quad
    vertices = [
        geom.vec3(0.0, 0.0, 0.0),
        geom.vec3(1.0, 0.0, 0.0),
        geom.vec3(1.0, 1.0, 0.0),
        geom.vec3(0.0, 1.0, 0.0),
    ]
    
    quad = geom.create_mesh_from_arrays(vertices, [4], [0, 1, 2, 3])
    mesh = quad.get_mesh_component()
    
    # Get face indices as numpy array
    indices = geom.get_face_indices_as_array(mesh)
    
    print(f"📊 Face Index Array:")
    print(f"   Shape: {indices.shape}")
    print(f"   Dtype: {indices.dtype}")
    print(f"   Indices: {indices}")
    
    assert indices.shape == (4,), "Shape should be (4,)"
    assert indices.dtype == np.int32, "Dtype should be int32"
    assert np.array_equal(indices, [0, 1, 2, 3]), "Indices mismatch"
    
    print("✓ Face indices correct")


def test_numpy_transform():
    """Test applying transforms via NumPy"""
    print("\n" + "="*60)
    print("TEST: NumPy Transform Operations")
    print("="*60)
    
    # Create quad
    vertices = [
        geom.vec3(0.0, 0.0, 0.0),
        geom.vec3(1.0, 0.0, 0.0),
        geom.vec3(1.0, 1.0, 0.0),
        geom.vec3(0.0, 1.0, 0.0),
    ]
    
    quad = geom.create_mesh_from_arrays(vertices, [4], [0, 1, 2, 3])
    mesh = quad.get_mesh_component()
    
    # Get as numpy
    verts = geom.get_vertices_as_array(mesh)
    
    # Apply transform: scale by 2, translate by (10, 20, 30)
    scale = 2.0
    translate = np.array([10.0, 20.0, 30.0])
    transformed = verts * scale + translate
    
    geom.set_vertices_from_array(mesh, transformed)
    
    # Verify
    result = geom.get_vertices_as_array(mesh)
    expected_min = translate
    expected_max = translate + scale * np.array([1.0, 1.0, 0.0])
    
    assert np.allclose(result.min(axis=0), expected_min), "Min bound wrong"
    assert np.allclose(result.max(axis=0), expected_max), "Max bound wrong"
    
    print(f"✓ Transform applied: scale={scale}, translate={translate}")
    print(f"  Bounds: {result.min(axis=0)} to {result.max(axis=0)}")


if __name__ == "__main__":
    try:
        test_vec3_operations()
        test_empty_geometry()
        test_create_triangle()
        test_create_quad()
        test_numpy_arrays()
        test_numpy_face_indices()
        test_numpy_transform()
        
        print("\n" + "="*70)
        print("  ALL GEOMETRY TESTS PASSED! 🎉")
        print("="*70)
        
    except Exception as e:
        print(f"\n✗ TEST FAILED: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
