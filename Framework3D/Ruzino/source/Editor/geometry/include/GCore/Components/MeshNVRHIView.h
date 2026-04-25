#pragma once
#include <GCore/api.h>
#include <nvrhi/nvrhi.h>

#include <map>
#include <set>
#include <string>

RUZINO_NAMESPACE_OPEN_SCOPE
struct MeshComponent;

// NVRHI View for working with nvrhi::IBuffer
struct GEOMETRY_API ConstMeshNVRHIView {
    explicit ConstMeshNVRHIView(
        const MeshComponent& mesh,
        nvrhi::IDevice* device);
    ~ConstMeshNVRHIView();

    // Get vertex positions as NVRHI buffer (lazy creation)
    // commandList is optional - if provided, buffer will be initialized
    // immediately
    nvrhi::IBuffer* get_vertices(
        nvrhi::ICommandList* commandList = nullptr) const;

    // Get face vertex counts and indices
    nvrhi::IBuffer* get_face_vertex_counts(
        nvrhi::ICommandList* commandList = nullptr) const;
    nvrhi::IBuffer* get_face_vertex_indices(
        nvrhi::ICommandList* commandList = nullptr) const;

    // Get normals as NVRHI buffer
    nvrhi::IBuffer* get_normals(
        nvrhi::ICommandList* commandList = nullptr) const;

    // Get UV coordinates as NVRHI buffer
    nvrhi::IBuffer* get_uv_coordinates(
        nvrhi::ICommandList* commandList = nullptr) const;

    // Get display colors as NVRHI buffer
    nvrhi::IBuffer* get_display_colors(
        nvrhi::ICommandList* commandList = nullptr) const;

    // Get scalar quantities
    nvrhi::IBuffer* get_vertex_scalar_quantity(
        const std::string& name,
        nvrhi::ICommandList* commandList = nullptr) const;
    nvrhi::IBuffer* get_face_scalar_quantity(
        const std::string& name,
        nvrhi::ICommandList* commandList = nullptr) const;

    // Get vector quantities
    nvrhi::IBuffer* get_vertex_vector_quantity(
        const std::string& name,
        nvrhi::ICommandList* commandList = nullptr) const;
    nvrhi::IBuffer* get_face_vector_quantity(
        const std::string& name,
        nvrhi::ICommandList* commandList = nullptr) const;

    // Get parameterization quantities
    nvrhi::IBuffer* get_vertex_parameterization_quantity(
        const std::string& name,
        nvrhi::ICommandList* commandList = nullptr) const;
    nvrhi::IBuffer* get_face_corner_parameterization_quantity(
        const std::string& name,
        nvrhi::ICommandList* commandList = nullptr) const;

   protected:
    const MeshComponent& mesh_;
    nvrhi::IDevice* device_;

    // Lazy-created NVRHI buffers
    mutable std::map<std::string, nvrhi::BufferHandle> nvrhi_buffers_;

    nvrhi::BufferHandle get_or_create_buffer(
        const std::string& key,
        const void* data,
        size_t element_count,
        size_t element_size,
        nvrhi::ICommandList* commandList) const;
};

struct GEOMETRY_API MeshNVRHIView : public ConstMeshNVRHIView {
    MeshNVRHIView(MeshComponent& mesh, nvrhi::IDevice* device);
    ~MeshNVRHIView();  // RAII: clears modified buffer tracking

    // Set vertex positions from NVRHI buffer
    void set_vertices(nvrhi::IBuffer* vertices);

    // Set face topology from NVRHI buffers
    void set_face_topology(
        nvrhi::IBuffer* face_vertex_counts,
        nvrhi::IBuffer* face_vertex_indices);

    // Set normals from NVRHI buffer
    void set_normals(nvrhi::IBuffer* normals);

    // Set UV coordinates from NVRHI buffer
    void set_uv_coordinates(nvrhi::IBuffer* uv_coords);

    // Set display colors from NVRHI buffer
    void set_display_colors(nvrhi::IBuffer* colors);

    // Set scalar quantities
    void set_vertex_scalar_quantity(
        const std::string& name,
        nvrhi::IBuffer* values);
    void set_face_scalar_quantity(
        const std::string& name,
        nvrhi::IBuffer* values);

    // Set vector quantities
    void set_vertex_vector_quantity(
        const std::string& name,
        nvrhi::IBuffer* vectors);
    void set_face_vector_quantity(
        const std::string& name,
        nvrhi::IBuffer* vectors);

    // Set parameterization quantities
    void set_vertex_parameterization_quantity(
        const std::string& name,
        nvrhi::IBuffer* params);
    void set_face_corner_parameterization_quantity(
        const std::string& name,
        nvrhi::IBuffer* params);

   private:
    MeshComponent& mutable_mesh_;

    // Track modified buffers - user needs to manually sync if needed
    mutable std::set<std::string> modified_buffers_;
};

// External API functions for creating views
GEOMETRY_API MeshNVRHIView
get_nvrhi_view(MeshComponent& mesh, nvrhi::IDevice* device);
GEOMETRY_API ConstMeshNVRHIView
get_nvrhi_view(const MeshComponent& mesh, nvrhi::IDevice* device);

RUZINO_NAMESPACE_CLOSE_SCOPE
