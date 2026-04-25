"""Comprehensive API tests for MeshComponent and CUDAView"""

import sys
import os

import numpy as np
import torch
import geometry_py


def test_numpy_api():
    """Test MeshComponent with numpy arrays"""
    print("Testing MeshComponent numpy API...")

    # Create mesh
    vertices = [
        geometry_py.vec3(0, 0, 0),
        geometry_py.vec3(1, 0, 0),
        geometry_py.vec3(0, 1, 0),
    ]
    triangle = geometry_py.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()

    # Create numpy data
    verts = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float32)
    normals = np.array([[0, 0, 1], [0, 0, 1], [0, 0, 1]], dtype=np.float32)
    colors = np.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]], dtype=np.float32)
    texcoords = np.array([[0, 0], [1, 0], [0, 1]], dtype=np.float32)
    counts = np.array([3], dtype=np.int32)
    indices = np.array([0, 1, 2], dtype=np.int32)

    # Set data
    mesh.set_vertices(verts)
    mesh.set_normals(normals)
    mesh.set_display_color(colors)
    mesh.set_texcoords(texcoords)
    mesh.set_face_vertex_counts(counts)
    mesh.set_face_vertex_indices(indices)

    # Get data
    v = mesh.get_vertices()
    n = mesh.get_normals()
    c = mesh.get_display_color()
    t = mesh.get_texcoords()

    # Verify
    assert isinstance(v, np.ndarray), f"Expected ndarray, got {type(v)}"
    assert isinstance(n, np.ndarray)
    assert isinstance(c, np.ndarray)
    assert isinstance(t, np.ndarray)

    assert np.allclose(v, verts), f"Vertices mismatch"
    assert np.allclose(n, normals)
    assert np.allclose(c, colors)
    assert np.allclose(t, texcoords)

    print("✓ NumPy get/set methods work")


def test_torch_api():
    """Test CUDAView with torch tensors"""
    print("Testing CUDAView torch API...")

    # Create mesh and get CUDA view
    vertices = [
        geometry_py.vec3(0, 0, 0),
        geometry_py.vec3(1, 0, 0),
        geometry_py.vec3(0, 1, 0),
    ]
    triangle = geometry_py.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()
    cuda_view = mesh.get_cuda_view()

    # Create torch CUDA tensors
    verts = torch.tensor(
        [[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=torch.float32, device="cuda"
    )
    normals = torch.tensor(
        [[0, 0, 1], [0, 0, 1], [0, 0, 1]], dtype=torch.float32, device="cuda"
    )
    colors = torch.tensor(
        [[1, 0, 0], [0, 1, 0], [0, 0, 1]], dtype=torch.float32, device="cuda"
    )
    texcoords = torch.tensor(
        [[0, 0], [1, 0], [0, 1]], dtype=torch.float32, device="cuda"
    )

    # Set data
    cuda_view.set_vertices(verts)
    cuda_view.set_normals(normals)
    cuda_view.set_display_color(colors)
    cuda_view.set_texcoords(texcoords)

    # Get data
    v = cuda_view.get_vertices()
    n = cuda_view.get_normals()
    c = cuda_view.get_display_color()
    t = cuda_view.get_texcoords()

    # Verify
    assert isinstance(v, torch.Tensor), f"Expected torch.Tensor, got {type(v)}"
    assert isinstance(n, torch.Tensor)
    assert isinstance(c, torch.Tensor)
    assert isinstance(t, torch.Tensor)

    assert v.device.type == "cuda"
    assert n.device.type == "cuda"
    assert c.device.type == "cuda"
    assert t.device.type == "cuda"

    assert torch.allclose(v, verts)
    assert torch.allclose(n, normals)
    assert torch.allclose(c, colors)
    assert torch.allclose(t, texcoords)

    print("✓ PyTorch get/set methods work")


def test_numpy_inplace():
    """Test in-place modification of numpy arrays"""
    print("Testing NumPy in-place modification...")

    vertices = [
        geometry_py.vec3(0, 0, 0),
        geometry_py.vec3(1, 0, 0),
        geometry_py.vec3(0, 1, 0),
    ]
    triangle = geometry_py.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()

    # Get vertices (zero-copy view)
    verts = mesh.get_vertices()

    # Modify in-place (NO set call)
    verts[0, 0] = 999.0
    verts[1, 1] = 888.0

    # Get again to verify modification persisted
    verts2 = mesh.get_vertices()
    assert verts2[0, 0] == 999.0 and verts2[1, 1] == 888.0

    print("✓ NumPy in-place modification works - no set() needed")


def test_torch_inplace():
    """Test in-place modification of torch tensors"""
    print("Testing PyTorch in-place modification...")

    vertices = [
        geometry_py.vec3(0, 0, 0),
        geometry_py.vec3(1, 0, 0),
        geometry_py.vec3(0, 1, 0),
    ]
    triangle = geometry_py.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()
    cuda_view = mesh.get_cuda_view()

    # Set initial data
    verts_init = torch.tensor(
        [[10, 20, 30], [11, 21, 31], [12, 22, 32]], dtype=torch.float32, device="cuda"
    )
    cuda_view.set_vertices(verts_init)

    # Get vertices (zero-copy GPU view)
    verts = cuda_view.get_vertices()

    # Modify in-place (NO set call)
    verts[0, 0] = 999.0
    verts[1, 1] = 888.0

    # Get again to verify modification persisted
    verts2 = cuda_view.get_vertices()
    assert torch.allclose(verts2[0, 0], torch.tensor(999.0, device="cuda"))
    assert torch.allclose(verts2[1, 1], torch.tensor(888.0, device="cuda"))

    print("✓ PyTorch in-place modification works - no set() needed")


def test_api_separation():
    """Test that MeshComponent returns numpy and CUDAView returns torch"""
    print("Testing API type separation...")

    vertices = [
        geometry_py.vec3(0, 0, 0),
        geometry_py.vec3(1, 0, 0),
        geometry_py.vec3(0, 1, 0),
    ]
    triangle = geometry_py.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()

    # Set via numpy
    verts_np = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0]], dtype=np.float32)
    mesh.set_vertices(verts_np)

    # Get via numpy - should return numpy array
    v_np = mesh.get_vertices()
    assert isinstance(v_np, np.ndarray), "MeshComponent should return numpy array"

    # Get via CUDA view - should return torch tensor
    cuda_view = mesh.get_cuda_view()
    v_torch = cuda_view.get_vertices()
    assert isinstance(v_torch, torch.Tensor), "CUDAView should return torch tensor"
    assert v_torch.device.type == "cuda", "CUDAView should return CUDA tensor"

    print("✓ API type separation correct")


def test_quantities():
    """Test scalar/vector/parameterization quantities"""
    print("\n[TEST] Quantities (scalar/vector/parameterization)...")

    vertices = [
        geometry_py.vec3(0, 0, 0),
        geometry_py.vec3(1, 0, 0),
        geometry_py.vec3(0, 1, 0),
    ]
    triangle = geometry_py.create_mesh_from_arrays(vertices, [3], [0, 1, 2])
    mesh = triangle.get_mesh_component()

    # Test scalar quantities
    vert_scalars = np.array([1.0, 2.0, 3.0], dtype=np.float32)
    mesh.set_vertex_scalar_quantity("test_vert", vert_scalars)
    result = mesh.get_vertex_scalar_quantity("test_vert")
    assert np.allclose(result, vert_scalars)
    result[0] = 999.0  # In-place modification
    assert mesh.get_vertex_scalar_quantity("test_vert")[0] == 999.0

    # Test vector quantities
    vert_vectors = np.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]], dtype=np.float32)
    mesh.set_vertex_vector_quantity("test_vec", vert_vectors)
    result = mesh.get_vertex_vector_quantity("test_vec")
    assert result.shape == (3, 3)
    assert np.allclose(result, vert_vectors)
    result[0, 0] = 888.0  # In-place modification
    assert mesh.get_vertex_vector_quantity("test_vec")[0, 0] == 888.0

    # Test parameterization quantities
    vert_params = np.array([[0, 0], [1, 0], [0, 1]], dtype=np.float32)
    mesh.set_vertex_parameterization_quantity("uv", vert_params)
    result = mesh.get_vertex_parameterization_quantity("uv")
    assert result.shape == (3, 2)
    assert np.allclose(result, vert_params)
    result[0, 0] = 0.5  # In-place modification
    assert mesh.get_vertex_parameterization_quantity("uv")[0, 0] == 0.5

    # Test empty quantities
    empty = mesh.get_vertex_scalar_quantity("nonexistent")
    assert len(empty) == 0
    empty_vec = mesh.get_vertex_vector_quantity("nonexistent")
    assert empty_vec.shape == (0, 3)

    print("  ✓ Scalar quantities (1D arrays)")
    print("  ✓ Vector quantities (Nx3 arrays)")
    print("  ✓ Parameterization quantities (Nx2 arrays)")
    print("  ✓ In-place modification")
    print("  ✓ Empty quantity handling")


if __name__ == "__main__":
    print("=" * 70)
    print("Comprehensive API Tests")
    print("=" * 70)

    test_numpy_api()
    test_torch_api()
    test_numpy_inplace()
    test_torch_inplace()
    test_api_separation()
    test_quantities()

    print("\n" + "=" * 70)
    print("🎉 ALL TESTS PASSED!")
    print("=" * 70)
    print("\nSummary:")
    print("  • MeshComponent.get_*() returns NumPy arrays (CPU, zero-copy)")
    print("  • CUDAView.get_*() returns PyTorch CUDA tensors (GPU, zero-copy)")
    print("  • Direct modification of arrays/tensors works without calling set_*()")
    print("  • set_*() methods available for replacing entire buffers")
    print("  • Quantities: scalars (1D), vectors (Nx3), params (Nx2) - all zero-copy")
    print("=" * 70)
