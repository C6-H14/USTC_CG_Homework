#include <GCore/algorithms/intersection.h>
#include <spdlog/spdlog.h>

#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/XformComponent.h"

#ifdef GPU_GEOM_ALGORITHM
#include "GPUContext/compute_context.hpp"
#include "GPUContext/program_vars.hpp"
#include "GPUContext/raytracing_context.hpp"
#include "RHI/ResourceManager/resource_allocator.hpp"
#include "RHI/internal/resources.hpp"
#include "RHI/rhi.hpp"
#include "Scene/SceneTypes.slang"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "nvrhi/nvrhi.h"

RUZINO_NAMESPACE_OPEN_SCOPE
ResourceAllocator resource_allocator_;
std::shared_ptr<ShaderFactory> shader_factory;

ResourceAllocator& get_resource_allocator()
{
    init_gpu_geometry_algorithms();
    return resource_allocator_;
}

void init_gpu_geometry_algorithms()
{
    if (!shader_factory) {
        resource_allocator_.set_device(RHI::get_device());
        shader_factory = std::make_shared<ShaderFactory>();
        shader_factory->add_search_path(RENDERER_SHADER_DIR "shaders");
        shader_factory->add_search_path(GEOM_COMPUTE_SHADER_DIR);
        resource_allocator_.shader_factory = shader_factory.get();
    }
}

void deinit_gpu_geometry_algorithms()
{
    resource_allocator_.terminate();
    shader_factory.reset();
}

nvrhi::rt::AccelStructHandle get_geomtry_tlas(
    const Geometry& geometry,
    MeshDesc* out_mesh_desc,
    nvrhi::BufferHandle* out_vertex_buffer)
{
    init_gpu_geometry_algorithms();
    auto mesh_component = geometry.get_component<MeshComponent>();
    if (!mesh_component) {
        return nullptr;
    }

    auto transform = glm::identity<glm::mat4>();
    auto xform_component = geometry.get_component<XformComponent>();
    if (xform_component) {
        transform = xform_component->get_transform();
    }

    // First build the BLAS for the mesh
    MeshDesc mesh_desc;
    auto device = RHI::get_device();

    size_t total_buffer_size = 0;
    size_t index_buffer_offset = 0;
    size_t normal_buffer_offset = 0;
    size_t texcoord_buffer_offset = 0;

    auto vertices = mesh_component->get_vertices();
    auto indices = mesh_component->get_face_vertex_indices();
    auto normals = mesh_component->get_normals();
    auto uvs = mesh_component->get_texcoords_array();

    // Calculate buffer offsets and total size
    total_buffer_size = vertices.size() * 3 * sizeof(float);
    index_buffer_offset = total_buffer_size;
    total_buffer_size += indices.size() * sizeof(uint);

    if (!normals.empty()) {
        normal_buffer_offset = total_buffer_size;
        total_buffer_size += normals.size() * 3 * sizeof(float);
    }

    if (!uvs.empty()) {
        texcoord_buffer_offset = total_buffer_size;
        total_buffer_size += uvs.size() * 2 * sizeof(float);
    }

    // Create vertex buffer
    nvrhi::BufferDesc desc =
        nvrhi::BufferDesc{}
            .setCanHaveRawViews(true)
            .setByteSize(total_buffer_size)
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setCpuAccess(nvrhi::CpuAccessMode::None)
            .setIsAccelStructBuildInput(true)
            .setKeepInitialState(true)
            .setDebugName("vertexBuffer");

    auto vertexBuffer = device->createBuffer(desc);

    // Create command list for all operations
    auto commandlist =
        device->createCommandList({ .enableImmediateExecution = false });
    commandlist->open();

    // Copy vertices
    commandlist->writeBuffer(
        vertexBuffer, vertices.data(), vertices.size() * 3 * sizeof(float), 0);

    // Copy indices
    commandlist->writeBuffer(
        vertexBuffer,
        indices.data(),
        indices.size() * sizeof(uint),
        index_buffer_offset);

    // Copy normals if available
    if (!normals.empty()) {
        commandlist->writeBuffer(
            vertexBuffer,
            normals.data(),
            normals.size() * 3 * sizeof(float),
            normal_buffer_offset);
    }

    // Copy UVs if available
    if (!uvs.empty()) {
        commandlist->writeBuffer(
            vertexBuffer,
            uvs.data(),
            uvs.size() * 2 * sizeof(float),
            texcoord_buffer_offset);
    }

    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Set up mesh_desc - initialize all fields to avoid undefined behavior
    mesh_desc = {};  // Zero-initialize first
    mesh_desc.vbOffset = 0;
    mesh_desc.ibOffset = index_buffer_offset;
    mesh_desc.normalOffset = normal_buffer_offset;
    mesh_desc.texCrdOffset = texcoord_buffer_offset;
    mesh_desc.tangentOffset = 0;
    mesh_desc.skinningVbOffset = 0;
    mesh_desc.prevVbOffset = 0;
    mesh_desc.flags = 0;
    mesh_desc.subsetMatIdOffset = 0;
    mesh_desc.normalInterpolation = InterpolationType::Vertex;
    mesh_desc.texCrdInterpolation = InterpolationType::Vertex;
    mesh_desc.tangentInterpolation = InterpolationType::Vertex;
    mesh_desc.padding = 0;
    mesh_desc.bindlessIndex = 0;  // Will be set later when needed

    // Create BLAS
    nvrhi::rt::AccelStructDesc blas_desc;
    nvrhi::rt::GeometryDesc geometry_desc;
    geometry_desc.geometryType = nvrhi::rt::GeometryType::Triangles;
    nvrhi::rt::GeometryTriangles triangles;
    triangles.setVertexBuffer(vertexBuffer)
        .setVertexOffset(0)
        .setIndexBuffer(vertexBuffer)
        .setIndexOffset(index_buffer_offset)
        .setIndexCount(indices.size())
        .setVertexCount(vertices.size())
        .setVertexStride(3 * sizeof(float))
        .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
        .setIndexFormat(nvrhi::Format::R32_UINT);
    geometry_desc.setTriangles(triangles);
    blas_desc.addBottomLevelGeometry(geometry_desc);
    blas_desc.isTopLevel = false;
    auto BLAS = device->createAccelStruct(blas_desc);

    commandlist->open();
    nvrhi::utils::BuildBottomLevelAccelStruct(commandlist, BLAS, blas_desc);
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Now create TLAS
    nvrhi::rt::AccelStructDesc tlas_desc;
    tlas_desc.isTopLevel = true;
    tlas_desc.topLevelMaxInstances = 1;
    nvrhi::rt::InstanceDesc instance_desc;
    instance_desc.setBLAS(BLAS);
    instance_desc.setInstanceID(0);
    instance_desc.setInstanceMask(0xFF);
    // set the transform
    nvrhi::rt::AffineTransform affine_transform;

    memcpy(
        affine_transform,
        glm::value_ptr(transpose(transform)),
        sizeof(nvrhi::rt::AffineTransform));

    instance_desc.setTransform(affine_transform);

    auto TLAS = device->createAccelStruct(tlas_desc);

    commandlist->open();
    commandlist->buildTopLevelAccelStruct(
        TLAS, std::vector{ instance_desc }.data(), 1);
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Fill output parameters if provided
    if (out_mesh_desc) {
        *out_mesh_desc = mesh_desc;
    }

    if (out_vertex_buffer) {
        *out_vertex_buffer = vertexBuffer;
    }

    assert(TLAS);
    return TLAS;
}

nvrhi::BufferHandle IntersectToBuffer_Single(
    const nvrhi::BufferHandle& ray_buffer,
    size_t ray_count,
    nvrhi::rt::IAccelStruct* tlas,
    nvrhi::IBuffer* instance_desc_buffer,
    nvrhi::IBuffer* mesh_desc_buffer,
    nvrhi::IBuffer* vertex_buffer)
{
    init_gpu_geometry_algorithms();

    if (!tlas || !instance_desc_buffer || !mesh_desc_buffer || !vertex_buffer) {
        spdlog::error("IntersectToBuffer_Single: Missing required resources");
        return nullptr;
    }

    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    ProgramDesc desc;
    desc.shaderType = nvrhi::ShaderType::AllRayTracing;
    desc.set_path(GEOM_COMPUTE_SHADER_DIR "intersection_single.slang");
    auto program = resource_allocator.create(desc);

    // Create output buffer for intersection results
    auto result_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(ray_count * sizeof(PointSample))
            .setStructStride(sizeof(PointSample))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setCanHaveUAVs(true)
            .setDebugName("resultBuffer"));

    ProgramVars program_vars(resource_allocator, program);

    // Bind resources to the shader - single mesh version without bindless
    program_vars["g_rays"] = ray_buffer;
    program_vars["g_Result"] = result_buffer;
    program_vars["SceneBVH"] = tlas;
    program_vars["instanceDescBuffer"] = instance_desc_buffer;
    program_vars["meshDescBuffer"] = mesh_desc_buffer;
    program_vars["vertex_buffer"] = vertex_buffer;  // Direct buffer binding

    program_vars.finish_setting_vars();

    RaytracingContext raytracing_context(resource_allocator, program_vars);

    // Set up shader names
    raytracing_context.announce_raygeneration("RayGen");
    raytracing_context.announce_hitgroup("ClosestHit");
    raytracing_context.announce_miss("Miss");
    raytracing_context.finish_announcing_shader_names();

    // Execute ray tracing
    raytracing_context.begin();
    raytracing_context.trace_rays({}, program_vars, ray_count, 1, 1);
    raytracing_context.finish();

    // Wait for GPU to finish before releasing any resources
    device->waitForIdle();

    // Clean up resources except for result_buffer
    resource_allocator.destroy(program);

    // Return the GPU buffer with results
    return result_buffer;
}

nvrhi::BufferHandle IntersectToBuffer(
    const nvrhi::BufferHandle& ray_buffer,
    size_t ray_count,
    nvrhi::rt::IAccelStruct* tlas,
    nvrhi::IBuffer* instance_desc_buffer,
    nvrhi::IBuffer* mesh_desc_buffer,
    nvrhi::IDescriptorTable* bindless_descriptor_table,
    nvrhi::BindingLayoutHandle bindless_layout)
{
    init_gpu_geometry_algorithms();

    if (!tlas || !instance_desc_buffer || !mesh_desc_buffer ||
        !bindless_descriptor_table || !bindless_layout) {
        spdlog::error("IntersectToBuffer: Missing required resources");
        return nullptr;
    }

    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    ProgramDesc desc;
    desc.shaderType = nvrhi::ShaderType::AllRayTracing;
    desc.set_path(GEOM_COMPUTE_SHADER_DIR "intersection.slang");
    auto program = resource_allocator.create(desc);

    // Create output buffer for intersection results
    auto result_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(ray_count * sizeof(PointSample))
            .setStructStride(sizeof(PointSample))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setCanHaveUAVs(true)
            .setDebugName("resultBuffer"));

    ProgramVars program_vars(resource_allocator, program);

    // Bind resources to the shader
    program_vars["g_rays"] = ray_buffer;
    program_vars["g_Result"] = result_buffer;
    program_vars["SceneBVH"] = tlas;
    program_vars["instanceDescBuffer"] = instance_desc_buffer;
    program_vars["meshDescBuffer"] = mesh_desc_buffer;

    // Bind bindless buffer descriptor table
    program_vars.set_descriptor_table(
        "t_BindlessBuffers", bindless_descriptor_table, bindless_layout);

    program_vars.finish_setting_vars();

    RaytracingContext raytracing_context(resource_allocator, program_vars);

    // Set up shader names
    raytracing_context.announce_raygeneration("RayGen");
    raytracing_context.announce_hitgroup("ClosestHit");
    raytracing_context.announce_miss("Miss");
    raytracing_context.finish_announcing_shader_names();

    // Execute ray tracing
    raytracing_context.begin();
    raytracing_context.trace_rays({}, program_vars, ray_count, 1, 1);
    raytracing_context.finish();

    // Wait for GPU to finish before releasing any resources
    device->waitForIdle();

    // Clean up resources except for result_buffer
    resource_allocator.destroy(program);

    // Return the GPU buffer with results
    return result_buffer;
}

std::vector<PointSample> IntersectWithBuffer(
    const nvrhi::BufferHandle& ray_buffer,
    size_t ray_count,
    const Geometry& BaseMesh)
{
    init_gpu_geometry_algorithms();
    auto mesh_component = BaseMesh.get_component<MeshComponent>();

    if (!mesh_component) {
        return std::vector<PointSample>(ray_count);
    }

    std::vector<glm::vec3> vertices = mesh_component->get_vertices();
    if (vertices.empty()) {
        return std::vector<PointSample>(ray_count);
    }

    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    // Build TLAS and get buffers for this geometry
    MeshDesc mesh_desc;
    nvrhi::BufferHandle vertex_buffer;
    auto accel =
        get_geomtry_tlas(BaseMesh, &mesh_desc, std::addressof(vertex_buffer));

    if (!accel) {
        return std::vector<PointSample>(ray_count);
    }

    // Create temporary instance descriptor buffer
    GeometryInstanceData instance_data;
    instance_data.geometryID = 0;
    instance_data.materialID = 0;
    instance_data.flags = 0;
    instance_data.padding = 0;
    instance_data.transform = float4x4::identity();

    auto instance_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(sizeof(GeometryInstanceData))
            .setStructStride(sizeof(GeometryInstanceData))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setCpuAccess(nvrhi::CpuAccessMode::Write)
            .setDebugName("tempInstanceBuffer"));

    void* inst_data =
        device->mapBuffer(instance_buffer, nvrhi::CpuAccessMode::Write);
    memcpy(inst_data, &instance_data, sizeof(GeometryInstanceData));
    device->unmapBuffer(instance_buffer);

    // Create temporary mesh descriptor buffer
    auto mesh_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(sizeof(MeshDesc))
            .setStructStride(sizeof(MeshDesc))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setCpuAccess(nvrhi::CpuAccessMode::Write)
            .setDebugName("tempMeshBuffer"));

    // No bindless descriptor table needed for single mesh version

    // Write mesh desc
    void* mesh_data =
        device->mapBuffer(mesh_buffer, nvrhi::CpuAccessMode::Write);
    memcpy(mesh_data, &mesh_desc, sizeof(MeshDesc));
    device->unmapBuffer(mesh_buffer);

    // Call the single mesh non-bindless version
    auto result_buffer = IntersectToBuffer_Single(
        ray_buffer,
        ray_count,
        accel.Get(),
        instance_buffer.Get(),
        mesh_buffer.Get(),
        vertex_buffer.Get());

    // If we got a null buffer, return empty results
    if (!result_buffer) {
        resource_allocator.destroy(instance_buffer);
        resource_allocator.destroy(mesh_buffer);
        return std::vector<PointSample>(ray_count);
    }

    std::vector<PointSample> result;
    result.resize(ray_count);

    // Create readback buffer
    auto readback_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(ray_count * sizeof(PointSample))
            .setCpuAccess(nvrhi::CpuAccessMode::Read)
            .setDebugName("resultReadbackBuffer"));

    // Create command list to copy data
    auto commandlist = resource_allocator.create(CommandListDesc{});
    commandlist->open();
    commandlist->copyBuffer(
        readback_buffer, 0, result_buffer, 0, ray_count * sizeof(PointSample));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Map and read the results
    void* mapped_data =
        device->mapBuffer(readback_buffer, nvrhi::CpuAccessMode::Read);
    memcpy(result.data(), mapped_data, ray_count * sizeof(PointSample));
    device->unmapBuffer(readback_buffer);

    // Clean up resources
    resource_allocator.destroy(result_buffer);
    resource_allocator.destroy(readback_buffer);
    resource_allocator.destroy(commandlist);
    resource_allocator.destroy(instance_buffer);
    resource_allocator.destroy(mesh_buffer);

    return result;
}

// New version using external TLAS and buffers with vector interface
std::vector<PointSample> IntersectWithScene(
    const std::vector<glm::ray>& rays,
    nvrhi::rt::IAccelStruct* tlas,
    nvrhi::IBuffer* instance_desc_buffer,
    nvrhi::IBuffer* mesh_desc_buffer,
    nvrhi::IDescriptorTable* bindless_descriptor_table,
    nvrhi::BindingLayoutHandle bindless_layout)
{
    init_gpu_geometry_algorithms();
    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    // Create ray buffer with input rays
    auto ray_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(rays.size() * sizeof(glm::ray))
            .setStructStride(sizeof(glm::ray))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setCpuAccess(nvrhi::CpuAccessMode::Write)
            .setDebugName("rayBuffer"));

    // Copy rays to the ray buffer
    void* data = device->mapBuffer(ray_buffer, nvrhi::CpuAccessMode::Write);
    memcpy(data, rays.data(), rays.size() * sizeof(glm::ray));
    device->unmapBuffer(ray_buffer);

    // Call the buffer version
    auto result_buffer = IntersectToBuffer(
        ray_buffer,
        rays.size(),
        tlas,
        instance_desc_buffer,
        mesh_desc_buffer,
        bindless_descriptor_table,
        bindless_layout);

    if (!result_buffer) {
        resource_allocator.destroy(ray_buffer);
        return {};
    }

    // Read back results
    auto readback_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(rays.size() * sizeof(PointSample))
            .setStructStride(sizeof(PointSample))
            .setInitialState(nvrhi::ResourceStates::CopyDest)
            .setKeepInitialState(true)
            .setCpuAccess(nvrhi::CpuAccessMode::Read)
            .setDebugName("readbackBuffer"));

    auto commandlist =
        device->createCommandList({ .enableImmediateExecution = false });
    commandlist->open();
    commandlist->copyBuffer(
        readback_buffer,
        0,
        result_buffer,
        0,
        rays.size() * sizeof(PointSample));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    std::vector<PointSample> result(rays.size());
    void* readback_data =
        device->mapBuffer(readback_buffer, nvrhi::CpuAccessMode::Read);
    memcpy(result.data(), readback_data, rays.size() * sizeof(PointSample));
    device->unmapBuffer(readback_buffer);

    // Clean up resources
    resource_allocator.destroy(ray_buffer);
    resource_allocator.destroy(result_buffer);
    resource_allocator.destroy(readback_buffer);
    resource_allocator.destroy(commandlist);

    return result;
}

std::vector<PointSample> Intersect(
    const std::vector<glm::ray>& rays,
    const Geometry& BaseMesh)
{
    init_gpu_geometry_algorithms();
    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    // Create ray buffer with input rays
    auto ray_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(rays.size() * sizeof(glm::ray))
            .setStructStride(sizeof(glm::ray))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setCpuAccess(nvrhi::CpuAccessMode::Write)
            .setDebugName("rayBuffer"));

    // Copy rays to the ray buffer
    void* data = device->mapBuffer(ray_buffer, nvrhi::CpuAccessMode::Write);
    memcpy(data, rays.data(), rays.size() * sizeof(glm::ray));
    device->unmapBuffer(ray_buffer);

    // Call the implementation with the buffer
    auto result = IntersectWithBuffer(ray_buffer, rays.size(), BaseMesh);

    // Clean up the buffer we created
    resource_allocator.destroy(ray_buffer);

    return result;
}

std::vector<PointSample> Intersect(
    const std::vector<glm::vec3>& start_point,
    const std::vector<glm::vec3>& next_point,
    const Geometry& BaseMesh)
{
    std::vector<glm::ray> rays;
    rays.reserve(start_point.size());
    for (size_t i = 0; i < start_point.size(); ++i) {
        rays.push_back(
            glm::ray(start_point[i], next_point[i] - start_point[i]));
    }

    return Intersect(rays, BaseMesh);
}

std::vector<PointSample> IntersectInterweaved(
    const std::vector<glm::vec3>& start_point,
    const std::vector<glm::vec3>& next_point,
    const Geometry& BaseMesh)
{
    std::vector<glm::ray> rays;
    rays.reserve(start_point.size() * next_point.size());
    for (size_t i = 0; i < start_point.size(); ++i) {
        for (size_t j = 0; j < next_point.size(); ++j) {
            rays.push_back(
                glm::ray(start_point[i], next_point[j] - start_point[i]));
        }
    }
    return Intersect(rays, BaseMesh);
}

// ============================================
// ============ Geometry Contacts =============
// ============================================

nvrhi::BufferHandle IntersectContactsToBuffer(
    const Geometry& geom_a,
    const Geometry& geom_b,
    size_t& out_contact_count)
{
    init_gpu_geometry_algorithms();

    auto mesh_a = geom_a.get_component<MeshComponent>();
    if (!mesh_a) {
        out_contact_count = 0;
        return nullptr;
    }

    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    // Get TLAS and buffers for geom_b (the surface to intersect)
    MeshDesc mesh_desc_b;
    nvrhi::BufferHandle vertex_buffer_b;
    auto tlas_b =
        get_geomtry_tlas(geom_b, &mesh_desc_b, std::addressof(vertex_buffer_b));

    if (!tlas_b) {
        out_contact_count = 0;
        return nullptr;
    }

    // Apply transform to geom_a to get world-space vertices
    Geometry geom_a_copy = geom_a;
    geom_a_copy.apply_transform();

    // Extract edges from transformed geom_a
    auto mesh_a_transformed = geom_a_copy.get_component<MeshComponent>();
    auto vertices_a = mesh_a_transformed->get_vertices();
    auto face_indices_a = mesh_a_transformed->get_face_vertex_indices();
    auto face_counts_a = mesh_a_transformed->get_face_vertex_counts();

    // Build edge list from faces
    std::vector<glm::ray> edge_rays;
    size_t index_offset = 0;

    // Statistics
    float max_edge_length = 0.0f;
    float min_edge_length = FLT_MAX;
    float total_edge_length = 0.0f;
    int edge_count = 0;

    for (size_t face_idx = 0; face_idx < face_counts_a.size(); ++face_idx) {
        int face_vertex_count = face_counts_a[face_idx];

        // For each edge in the face
        for (int i = 0; i < face_vertex_count; ++i) {
            int next_i = (i + 1) % face_vertex_count;

            int v0_idx = face_indices_a[index_offset + i];
            int v1_idx = face_indices_a[index_offset + next_i];

            // Vertices are already in world space after apply_transform
            glm::vec3 v0 = vertices_a[v0_idx];
            glm::vec3 v1 = vertices_a[v1_idx];

            // Create ray from v0 to v1
            glm::vec3 direction = v1 - v0;
            float edge_length = glm::length(direction);

            if (edge_length > 1e-6f) {
                direction /= edge_length;  // Normalize
                glm::ray edge_ray;
                edge_ray.origin = v0;
                edge_ray.direction = direction;
                edge_ray.tmin = 0.0f;
                edge_ray.tmax = edge_length;  // Limit ray to edge length
                edge_rays.push_back(edge_ray);

                // Update statistics
                max_edge_length = std::max(max_edge_length, edge_length);
                min_edge_length = std::min(min_edge_length, edge_length);
                total_edge_length += edge_length;
                edge_count++;
            }
        }

        index_offset += face_vertex_count;
    }

    // Print edge statistics
    if (edge_count > 0) {
        spdlog::info("Edge Statistics for geom_a:");
        spdlog::info("  Total edges: {}", edge_count);
        spdlog::info("  Min edge length: {}", min_edge_length);
        spdlog::info("  Max edge length: {}", max_edge_length);
        spdlog::info(
            "  Average edge length: {}", total_edge_length / edge_count);

        // Print first few edge origins for debugging
        spdlog::info("  Sample edge origins (first 5):");
        for (size_t i = 0; i < std::min(size_t(5), edge_rays.size()); ++i) {
            spdlog::info(
                "    Edge {}: origin=({}, {}, {}), tmax={}",
                i,
                edge_rays[i].origin.x,
                edge_rays[i].origin.y,
                edge_rays[i].origin.z,
                edge_rays[i].tmax);
        }
    }

    if (edge_rays.empty()) {
        out_contact_count = 0;
        return nullptr;
    }

    // Create GPU buffer for rays
    auto ray_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(edge_rays.size() * sizeof(glm::ray))
            .setStructStride(sizeof(glm::ray))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("edgeRaysBuffer"));

    auto commandlist = resource_allocator.create(CommandListDesc{});
    commandlist->open();
    commandlist->writeBuffer(
        ray_buffer, edge_rays.data(), edge_rays.size() * sizeof(glm::ray));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Create instance desc buffer
    GeometryInstanceData instance_desc{};
    instance_desc.materialID = 0;
    instance_desc.geometryID = 0;
    instance_desc.flags = 0;
    instance_desc.padding = 0;
    // Set identity transform
    instance_desc.transform[0][0] = 1.0f;
    instance_desc.transform[1][1] = 1.0f;
    instance_desc.transform[2][2] = 1.0f;
    instance_desc.transform[3][3] = 1.0f;

    auto instance_desc_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(sizeof(GeometryInstanceData))
            .setStructStride(sizeof(GeometryInstanceData))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("instanceDescBuffer"));

    commandlist->open();
    commandlist->writeBuffer(
        instance_desc_buffer, &instance_desc, sizeof(GeometryInstanceData));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Create mesh desc buffer
    auto mesh_desc_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(sizeof(MeshDesc))
            .setStructStride(sizeof(MeshDesc))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("meshDescBuffer"));

    commandlist->open();
    commandlist->writeBuffer(mesh_desc_buffer, &mesh_desc_b, sizeof(MeshDesc));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Perform intersection using single-mesh version
    auto result_buffer = IntersectToBuffer_Single(
        ray_buffer,
        edge_rays.size(),
        tlas_b.Get(),
        instance_desc_buffer.Get(),
        mesh_desc_buffer.Get(),
        vertex_buffer_b.Get());

    out_contact_count = edge_rays.size();

    // Clean up temporary resources
    resource_allocator.destroy(ray_buffer);
    resource_allocator.destroy(instance_desc_buffer);
    resource_allocator.destroy(mesh_desc_buffer);
    resource_allocator.destroy(commandlist);

    return result_buffer;
}

std::vector<PointSample> IntersectContacts(
    const Geometry& geom_a,
    const Geometry& geom_b)
{
    init_gpu_geometry_algorithms();

    size_t contact_count = 0;
    auto contact_buffer =
        IntersectContactsToBuffer(geom_a, geom_b, contact_count);

    if (!contact_buffer || contact_count == 0) {
        if (contact_buffer) {
            get_resource_allocator().destroy(contact_buffer);
        }
        return std::vector<PointSample>();
    }

    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    std::vector<PointSample> result;
    result.resize(contact_count);

    // Create readback buffer
    auto readback_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(contact_count * sizeof(PointSample))
            .setCpuAccess(nvrhi::CpuAccessMode::Read)
            .setInitialState(nvrhi::ResourceStates::CopyDest)
            .setKeepInitialState(true)
            .setDebugName("contactReadbackBuffer"));

    // Copy data from GPU to readback buffer
    auto commandlist = resource_allocator.create(CommandListDesc{});
    commandlist->open();
    commandlist->copyBuffer(
        readback_buffer,
        0,
        contact_buffer,
        0,
        contact_count * sizeof(PointSample));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Map and read the results
    void* mapped_data =
        device->mapBuffer(readback_buffer, nvrhi::CpuAccessMode::Read);
    memcpy(result.data(), mapped_data, contact_count * sizeof(PointSample));
    device->unmapBuffer(readback_buffer);

    // Clean up resources
    resource_allocator.destroy(contact_buffer);
    resource_allocator.destroy(readback_buffer);
    resource_allocator.destroy(commandlist);

    return result;
}

// ============================================
// ============= Neighbor search ==============
// ============================================

nvrhi::BufferHandle FindNeighborsFromPositionBuffer(
    const nvrhi::BufferHandle& position_buffer,
    size_t point_count,
    float radius,
    unsigned& out_pair_count)
{
    init_gpu_geometry_algorithms();

    if (!position_buffer || point_count == 0) {
        out_pair_count = 0;
        return nullptr;
    }

    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    // Create single command list for all operations
    auto commandlist = resource_allocator.create(CommandListDesc{});

    // Step 1: Create AABB buffer and compute AABBs using toAABB.slang
    auto aabb_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(point_count * 2 * sizeof(glm::vec3))
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setKeepInitialState(true)
            .setCanHaveRawViews(true)
            .setCanHaveUAVs(true)
            .setIsAccelStructBuildInput(true)
            .setDebugName("aabbBuffer"));

    // Compile and run toAABB compute shader
    ProgramDesc aabb_desc;
    aabb_desc.set_path(GEOM_COMPUTE_SHADER_DIR "Points/toAABB.slang")
        .set_entry_name("main")
        .set_shader_type(nvrhi::ShaderType::Compute);

    auto aabb_program = resource_allocator.create(aabb_desc);

    if (!aabb_program->get_error_string().empty()) {
        spdlog::error(
            "toAABB shader compilation failed: {}",
            aabb_program->get_error_string());
        resource_allocator.destroy(aabb_program);
        resource_allocator.destroy(aabb_buffer);
        resource_allocator.destroy(commandlist);
        out_pair_count = 0;
        return nullptr;
    }

    ProgramVars aabb_vars(resource_allocator, aabb_program);

    // Create constant buffer for radius
    auto radius_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(16)
            .setIsConstantBuffer(true)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
            .setKeepInitialState(true)
            .setDebugName("radiusBuffer"));

    commandlist->open();
    float radius_data[4] = { radius, 0, 0, 0 };
    commandlist->writeBuffer(radius_buffer, radius_data, sizeof(radius_data));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    aabb_vars["SearchParams"] = radius_buffer;
    aabb_vars["positions"] = position_buffer;
    aabb_vars["AABBs"] = aabb_buffer;
    aabb_vars.finish_setting_vars();

    ComputeContext compute_context(resource_allocator, aabb_vars);
    compute_context.finish_setting_pso();
    compute_context.begin();
    compute_context.dispatch({}, aabb_vars, point_count, 32);
    compute_context.finish();

    resource_allocator.destroy(aabb_program);
    resource_allocator.destroy(radius_buffer);

    // Step 2: Build BLAS with AABBs
    nvrhi::rt::AccelStructDesc blas_desc;
    nvrhi::rt::GeometryDesc geometry_desc;
    geometry_desc.geometryType = nvrhi::rt::GeometryType::AABBs;
    geometry_desc.flags = nvrhi::rt::GeometryFlags::NoDuplicateAnyHitInvocation;
    nvrhi::rt::GeometryAABBs aabbs;
    aabbs.setBuffer(aabb_buffer)
        .setOffset(0)
        .setCount(point_count)
        .setStride(2 * sizeof(glm::vec3));
    geometry_desc.setAABBs(aabbs);
    blas_desc.addBottomLevelGeometry(geometry_desc);
    blas_desc.isTopLevel = false;
    auto BLAS = resource_allocator.create(blas_desc);

    commandlist->open();
    nvrhi::utils::BuildBottomLevelAccelStruct(commandlist, BLAS, blas_desc);
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Step 3: Build TLAS
    nvrhi::rt::AccelStructDesc tlas_desc;
    tlas_desc.isTopLevel = true;
    tlas_desc.topLevelMaxInstances = 1;

    nvrhi::rt::InstanceDesc instance_desc;
    instance_desc.setBLAS(BLAS);
    instance_desc.setInstanceID(0);
    instance_desc.setInstanceMask(0xFF);

    nvrhi::rt::AffineTransform affine_transform;
    memset(affine_transform, 0, sizeof(nvrhi::rt::AffineTransform));
    affine_transform[0] = 1.0f;
    affine_transform[5] = 1.0f;
    affine_transform[10] = 1.0f;
    instance_desc.setTransform(affine_transform);

    auto TLAS = resource_allocator.create(tlas_desc);

    commandlist->open();
    commandlist->buildTopLevelAccelStruct(
        TLAS, std::vector{ instance_desc }.data(), 1);
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Step 4: Create output buffers for pairs
    size_t max_pairs = point_count * 30;

    auto pairs_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(max_pairs * sizeof(PointPairs))
            .setStructStride(sizeof(PointPairs))
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setKeepInitialState(true)
            .setCanHaveUAVs(true)
            .setDebugName("pairsBuffer"));

    auto pairs_count_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(sizeof(unsigned))
            .setStructStride(sizeof(unsigned))
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setKeepInitialState(true)
            .setCanHaveUAVs(true)
            .setDebugName("pairsCountBuffer"));

    commandlist->open();
    uint32_t zero = 0;
    commandlist->writeBuffer(pairs_count_buffer, &zero, sizeof(uint32_t));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Step 5: Run contact.slang ray tracing shader
    ProgramDesc contact_desc;
    contact_desc.shaderType = nvrhi::ShaderType::AllRayTracing;
    contact_desc.set_path(GEOM_COMPUTE_SHADER_DIR "Points/contact.slang");
    auto contact_program = resource_allocator.create(contact_desc);

    if (!contact_program || !contact_program->getBufferPointer() ||
        !contact_program->get_error_string().empty()) {
        if (!contact_program->get_error_string().empty()) {
            spdlog::error(
                "contact shader compilation failed: {}",
                contact_program->get_error_string());
        }
        resource_allocator.destroy(contact_program);
        resource_allocator.destroy(pairs_count_buffer);
        resource_allocator.destroy(pairs_buffer);
        resource_allocator.destroy(TLAS);
        resource_allocator.destroy(BLAS);
        resource_allocator.destroy(aabb_buffer);
        resource_allocator.destroy(commandlist);
        out_pair_count = 0;
        return nullptr;
    }

    ProgramVars contact_vars(resource_allocator, contact_program);

    auto contact_radius_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(16)
            .setIsConstantBuffer(true)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
            .setKeepInitialState(true)
            .setDebugName("contactRadiusBuffer"));

    commandlist->open();
    float contact_radius_data[4] = { radius, 0, 0, 0 };
    commandlist->writeBuffer(
        contact_radius_buffer,
        contact_radius_data,
        sizeof(contact_radius_data));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    contact_vars["SearchParams"] = contact_radius_buffer;
    contact_vars["SceneBVH"] = TLAS;
    contact_vars["positions"] = position_buffer;
    contact_vars["Pairs"] = pairs_buffer;
    contact_vars["PairsCount"] = pairs_count_buffer;
    contact_vars.finish_setting_vars();

    RaytracingContext raytracing_context(resource_allocator, contact_vars);
    raytracing_context.announce_raygeneration("RayGen");
    raytracing_context.announce_hitgroup(
        "ClosestHit", "AnyHit", "Intersection");
    raytracing_context.announce_miss("Miss");
    raytracing_context.finish_announcing_shader_names();

    raytracing_context.begin();
    raytracing_context.trace_rays({}, contact_vars, point_count, 1, 1);
    raytracing_context.finish();

    // Step 6: Read back the pair count
    auto count_readback_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(sizeof(unsigned))
            .setCpuAccess(nvrhi::CpuAccessMode::Read)
            .setInitialState(nvrhi::ResourceStates::CopyDest)
            .setKeepInitialState(true)
            .setDebugName("pairsCountReadbackBuffer"));

    commandlist->open();
    commandlist->copyBuffer(
        count_readback_buffer, 0, pairs_count_buffer, 0, sizeof(unsigned));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    void* count_data =
        device->mapBuffer(count_readback_buffer, nvrhi::CpuAccessMode::Read);
    memcpy(&out_pair_count, count_data, sizeof(unsigned));
    device->unmapBuffer(count_readback_buffer);

    // Clean up temporary resources
    resource_allocator.destroy(aabb_buffer);
    resource_allocator.destroy(BLAS);
    resource_allocator.destroy(TLAS);
    resource_allocator.destroy(pairs_count_buffer);
    resource_allocator.destroy(contact_program);
    resource_allocator.destroy(contact_radius_buffer);
    resource_allocator.destroy(count_readback_buffer);
    resource_allocator.destroy(commandlist);

    return pairs_buffer;
}

nvrhi::BufferHandle FindNeighborsToBuffer(
    const Geometry& point_cloud,
    float radius,
    unsigned& out_pair_count)
{
    init_gpu_geometry_algorithms();
    auto points_component = point_cloud.get_component<PointsComponent>();

    if (!points_component) {
        out_pair_count = 0;
        return nullptr;
    }

    std::vector<glm::vec3> positions = points_component->get_vertices();

    if (positions.empty()) {
        out_pair_count = 0;
        return nullptr;
    }

    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    size_t point_count = positions.size();

    // Create position buffer from geometry
    auto position_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(point_count * sizeof(glm::vec3))
            .setStructStride(sizeof(glm::vec3))
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("positionBuffer"));

    auto commandlist = resource_allocator.create(CommandListDesc{});
    commandlist->open();
    commandlist->writeBuffer(
        position_buffer, positions.data(), point_count * sizeof(glm::vec3));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();
    resource_allocator.destroy(commandlist);

    // Call the core function with the position buffer
    auto result = FindNeighborsFromPositionBuffer(
        position_buffer, point_count, radius, out_pair_count);

    // Clean up the position buffer we created
    resource_allocator.destroy(position_buffer);

    return result;
}

std::vector<PointPairs> FindNeighbors(
    const Geometry& point_cloud,
    float radius,
    unsigned& out_pair_count)
{
    auto& resource_allocator = get_resource_allocator();
    auto device = RHI::get_device();

    // Get the GPU buffer with neighbor pairs
    auto pairs_buffer =
        FindNeighborsToBuffer(point_cloud, radius, out_pair_count);

    // If we got a null buffer or no pairs found, return empty results
    if (!pairs_buffer || out_pair_count == 0) {
        if (pairs_buffer) {
            resource_allocator.destroy(pairs_buffer);
        }
        return std::vector<PointPairs>();
    }

    std::vector<PointPairs> result;
    result.resize(out_pair_count);

    // Create readback buffer
    auto readback_buffer = resource_allocator.create(
        nvrhi::BufferDesc{}
            .setByteSize(out_pair_count * sizeof(PointPairs))
            .setCpuAccess(nvrhi::CpuAccessMode::Read)
            .setInitialState(nvrhi::ResourceStates::CopyDest)
            .setKeepInitialState(true)
            .setDebugName("pairsReadbackBuffer"));

    // Create command list to copy data
    auto commandlist = resource_allocator.create(CommandListDesc{});
    commandlist->open();
    commandlist->copyBuffer(
        readback_buffer,
        0,
        pairs_buffer,
        0,
        out_pair_count * sizeof(PointPairs));
    commandlist->close();
    device->executeCommandList(commandlist);
    device->waitForIdle();

    // Map and read the results
    void* mapped_data =
        device->mapBuffer(readback_buffer, nvrhi::CpuAccessMode::Read);
    memcpy(result.data(), mapped_data, out_pair_count * sizeof(PointPairs));
    device->unmapBuffer(readback_buffer);

    // Clean up resources
    resource_allocator.destroy(pairs_buffer);
    resource_allocator.destroy(readback_buffer);
    resource_allocator.destroy(commandlist);

    return result;
}

RUZINO_NAMESPACE_CLOSE_SCOPE
#endif
