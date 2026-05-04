#pragma once

#include "GCore/GOP.h"
#include "GCore/api.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "nvrhi/nvrhi.h"

namespace glm {
struct ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float tmin = 0.0f;
    float tmax = 1e30f;  // Large number for "infinite" rays
};
}  // namespace glm
#ifdef GPU_GEOM_ALGORITHM
#include "RHI/ResourceManager/resource_allocator.hpp"
#endif

RUZINO_NAMESPACE_OPEN_SCOPE
struct MeshDesc;

struct GEOMETRY_API PointSample {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    unsigned valid;
};

struct GEOMETRY_API PointPairs {
    unsigned p1;
    unsigned p2;
};

#ifdef GPU_GEOM_ALGORITHM

GEOMETRY_API void init_gpu_geometry_algorithms();
GEOMETRY_API void deinit_gpu_geometry_algorithms();
// Remember to destroy the geometry explicitly with the resource allocator after
// use.
GEOMETRY_API ResourceAllocator& get_resource_allocator();

GEOMETRY_API nvrhi::rt::AccelStructHandle get_geomtry_tlas(
    const Geometry& geometry,
    MeshDesc* out_mesh_desc = nullptr,
    nvrhi::BufferHandle* out_vertex_buffer = nullptr);

GEOMETRY_API std::vector<PointSample> IntersectWithBuffer(
    const nvrhi::BufferHandle& ray_buffer,
    size_t ray_count,
    const Geometry& BaseMesh);

// New version using external TLAS and buffers
GEOMETRY_API nvrhi::BufferHandle IntersectToBuffer(
    const nvrhi::BufferHandle& ray_buffer,
    size_t ray_count,
    nvrhi::rt::IAccelStruct* tlas,
    nvrhi::IBuffer* instance_desc_buffer,
    nvrhi::IBuffer* mesh_desc_buffer,
    nvrhi::IDescriptorTable* bindless_descriptor_table,
    nvrhi::BindingLayoutHandle bindless_layout);

GEOMETRY_API std::vector<PointSample> Intersect(
    const std::vector<glm::ray>& rays,
    const Geometry& BaseMesh);

// New version using external TLAS and buffers with vector interface
GEOMETRY_API std::vector<PointSample> IntersectWithScene(
    const std::vector<glm::ray>& rays,
    nvrhi::rt::IAccelStruct* tlas,
    nvrhi::IBuffer* instance_desc_buffer,
    nvrhi::IBuffer* mesh_desc_buffer,
    nvrhi::IDescriptorTable* bindless_descriptor_table,
    nvrhi::BindingLayoutHandle bindless_layout);

GEOMETRY_API std::vector<PointSample> Intersect(
    const std::vector<glm::vec3>& start_point,
    const std::vector<glm::vec3>& next_point,
    const Geometry& BaseMesh);

// result should be of size start_point.size() * next_point.size()
GEOMETRY_API std::vector<PointSample> IntersectInterweaved(
    const std::vector<glm::vec3>& start_point,
    const std::vector<glm::vec3>& next_point,
    const Geometry& BaseMesh);

// ============================================
// ============ Geometry Contacts =============
// ============================================

// Return all contact that geom_a's edges touches geom_b's surface
GEOMETRY_API std::vector<PointSample> IntersectContacts(
    const Geometry& geom_a,
    const Geometry& geom_b);

// GPU buffer version - returns buffer containing contact points
GEOMETRY_API nvrhi::BufferHandle IntersectContactsToBuffer(
    const Geometry& geom_a,
    const Geometry& geom_b,
    size_t& out_contact_count);

// ============================================
// ============= Neighbor search ==============
// ============================================

// Find neighbors from Geometry (convenience wrapper)
GEOMETRY_API nvrhi::BufferHandle FindNeighborsToBuffer(
    const Geometry& point_cloud,
    float radius,
    unsigned& out_pair_count);

// Find neighbors directly from GPU position buffer (for per-substep updates)
GEOMETRY_API nvrhi::BufferHandle FindNeighborsFromPositionBuffer(
    const nvrhi::BufferHandle& position_buffer,
    size_t point_count,
    float radius,
    unsigned& out_pair_count);

GEOMETRY_API std::vector<PointPairs> FindNeighbors(
    const Geometry& point_cloud,
    float radius,
    unsigned& out_pair_count);

#endif

RUZINO_NAMESPACE_CLOSE_SCOPE
