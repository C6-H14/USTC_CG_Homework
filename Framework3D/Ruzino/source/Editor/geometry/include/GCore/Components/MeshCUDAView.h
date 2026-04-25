#pragma once
#include <GCore/api.h>

#if RUZINO_WITH_CUDA

#include <RHI/internal/cuda_extension.hpp>
#include <map>
#include <set>
#include <string>

RUZINO_NAMESPACE_OPEN_SCOPE
struct MeshComponent;

// CUDA View for working with GPU buffers via CudaLinearBuffer
struct GEOMETRY_API ConstMeshCUDAView {
    explicit ConstMeshCUDAView(const MeshComponent& mesh);
    ~ConstMeshCUDAView();

    // Get vertex positions as CUDA buffer (lazy creation)
    cuda::CUDALinearBufferHandle get_vertices() const;

    // Get face vertex counts and indices
    cuda::CUDALinearBufferHandle get_face_vertex_counts() const;
    cuda::CUDALinearBufferHandle get_face_vertex_indices() const;

    // Get normals as CUDA buffer
    cuda::CUDALinearBufferHandle get_normals() const;

    // Get UV coordinates as CUDA buffer
    cuda::CUDALinearBufferHandle get_uv_coordinates() const;

    // Get display colors as CUDA buffer
    cuda::CUDALinearBufferHandle get_display_colors() const;

    // Get scalar quantities
    cuda::CUDALinearBufferHandle get_vertex_scalar_quantity(
        const std::string& name) const;
    cuda::CUDALinearBufferHandle get_face_scalar_quantity(
        const std::string& name) const;

    // Get vector quantities
    cuda::CUDALinearBufferHandle get_vertex_vector_quantity(
        const std::string& name) const;
    cuda::CUDALinearBufferHandle get_face_vector_quantity(
        const std::string& name) const;

    // Get parameterization quantities
    cuda::CUDALinearBufferHandle get_vertex_parameterization_quantity(
        const std::string& name) const;
    cuda::CUDALinearBufferHandle get_face_corner_parameterization_quantity(
        const std::string& name) const;

   protected:
    const MeshComponent& mesh_;

    // Lazy-created CUDA buffers
    mutable std::map<std::string, cuda::CUDALinearBufferHandle> cuda_buffers_;

    cuda::CUDALinearBufferHandle get_or_create_buffer(
        const std::string& key,
        const void* data,
        size_t element_count,
        size_t element_size) const;
};

struct GEOMETRY_API MeshCUDAView : public ConstMeshCUDAView {
    MeshCUDAView(MeshComponent& mesh);
    ~MeshCUDAView();  // RAII: sync data back to CPU on destruction

    // Set vertex positions from CUDA buffer
    void set_vertices(cuda::CUDALinearBufferHandle vertices);

    // Set face topology from CUDA buffers
    void set_face_topology(
        cuda::CUDALinearBufferHandle face_vertex_counts,
        cuda::CUDALinearBufferHandle face_vertex_indices);

    // Set normals from CUDA buffer
    void set_normals(cuda::CUDALinearBufferHandle normals);

    // Set UV coordinates from CUDA buffer
    void set_uv_coordinates(cuda::CUDALinearBufferHandle uv_coords);

    // Set display colors from CUDA buffer
    void set_display_colors(cuda::CUDALinearBufferHandle colors);

    // Set scalar quantities
    void set_vertex_scalar_quantity(
        const std::string& name,
        cuda::CUDALinearBufferHandle values);
    void set_face_scalar_quantity(
        const std::string& name,
        cuda::CUDALinearBufferHandle values);

    // Set vector quantities
    void set_vertex_vector_quantity(
        const std::string& name,
        cuda::CUDALinearBufferHandle vectors);
    void set_face_vector_quantity(
        const std::string& name,
        cuda::CUDALinearBufferHandle vectors);

    // Set parameterization quantities
    void set_vertex_parameterization_quantity(
        const std::string& name,
        cuda::CUDALinearBufferHandle params);
    void set_face_corner_parameterization_quantity(
        const std::string& name,
        cuda::CUDALinearBufferHandle params);

   private:
    MeshComponent& mutable_mesh_;

    // Track modified buffers for sync back to CPU
    mutable std::set<std::string> modified_buffers_;

    void sync_buffer_to_cpu(const std::string& buffer_name);
    void sync_all_to_cpu();
};

// External API functions for creating views
GEOMETRY_API MeshCUDAView get_cuda_view(MeshComponent& mesh);
GEOMETRY_API ConstMeshCUDAView get_cuda_view(const MeshComponent& mesh);

RUZINO_NAMESPACE_CLOSE_SCOPE

#endif  // RUZINO_WITH_CUDA
