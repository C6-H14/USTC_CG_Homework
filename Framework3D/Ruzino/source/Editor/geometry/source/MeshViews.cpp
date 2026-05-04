#include <GCore/Components/MeshIGLView.h>
#include <GCore/Components/MeshNVRHIView.h>
#include <GCore/Components/MeshViews.h>
#ifdef GEOM_USD_EXTENSION
#include <GCore/Components/MeshUSDView.h>
#endif
#if RUZINO_WITH_CUDA
#include <GCore/Components/MeshCUDAView.h>
#endif

#include <set>
#include <stdexcept>

#include "GCore/Components/MeshComponent.h"

RUZINO_NAMESPACE_OPEN_SCOPE

// View lock implementation
void MeshComponent::acquire_view_lock() const
{
    if (has_active_view_) {
        throw std::runtime_error(
            "Cannot create multiple views of the same MeshComponent "
            "simultaneously. "
            "Please destroy the existing view before creating a new one.");
    }
    has_active_view_ = true;
}

void MeshComponent::release_view_lock() const
{
    has_active_view_ = false;
}

// ConstMeshIGLView Implementation
ConstMeshIGLView::ConstMeshIGLView(const MeshComponent& mesh) : mesh_(mesh)
{
    mesh_.acquire_view_lock();
}

ConstMeshIGLView::~ConstMeshIGLView()
{
    mesh_.release_view_lock();
}

Eigen::MatrixXf ConstMeshIGLView::get_vertices() const
{
    return vec3f_array_to_matrix(mesh_.get_vertices());
}

Eigen::MatrixXi ConstMeshIGLView::get_faces() const
{
    auto face_vertex_counts = mesh_.get_face_vertex_counts();
    auto face_vertex_indices = mesh_.get_face_vertex_indices();

    // Count triangular faces only
    int triangle_count = 0;
    for (int count : face_vertex_counts) {
        if (count == 3)
            triangle_count++;
    }

    Eigen::MatrixXi faces(triangle_count, 3);
    int face_idx = 0;
    int vertex_idx = 0;

    for (int count : face_vertex_counts) {
        if (count == 3) {
            faces(face_idx, 0) = face_vertex_indices[vertex_idx];
            faces(face_idx, 1) = face_vertex_indices[vertex_idx + 1];
            faces(face_idx, 2) = face_vertex_indices[vertex_idx + 2];
            face_idx++;
        }
        vertex_idx += count;
    }

    return faces;
}

Eigen::VectorXi ConstMeshIGLView::get_face_vertex_counts() const
{
    return int_array_to_vector(mesh_.get_face_vertex_counts());
}

Eigen::VectorXi ConstMeshIGLView::get_face_vertex_indices() const
{
    return int_array_to_vector(mesh_.get_face_vertex_indices());
}

Eigen::MatrixXf ConstMeshIGLView::get_normals() const
{
    return vec3f_array_to_matrix(mesh_.get_normals());
}

Eigen::MatrixXf ConstMeshIGLView::get_uv_coordinates() const
{
    return vec2f_array_to_matrix(mesh_.get_texcoords_array());
}

Eigen::MatrixXf ConstMeshIGLView::get_display_colors() const
{
    return vec3f_array_to_matrix(mesh_.get_display_color());
}

Eigen::VectorXf ConstMeshIGLView::get_vertex_scalar_quantity(
    const std::string& name) const
{
    return float_array_to_vector(mesh_.get_vertex_scalar_quantity(name));
}

Eigen::VectorXf ConstMeshIGLView::get_face_scalar_quantity(
    const std::string& name) const
{
    return float_array_to_vector(mesh_.get_face_scalar_quantity(name));
}

Eigen::MatrixXf ConstMeshIGLView::get_vertex_vector_quantity(
    const std::string& name) const
{
    return vec3f_array_to_matrix(mesh_.get_vertex_vector_quantity(name));
}

Eigen::MatrixXf ConstMeshIGLView::get_face_vector_quantity(
    const std::string& name) const
{
    return vec3f_array_to_matrix(mesh_.get_face_vector_quantity(name));
}

Eigen::MatrixXf ConstMeshIGLView::get_vertex_parameterization_quantity(
    const std::string& name) const
{
    return vec2f_array_to_matrix(
        mesh_.get_vertex_parameterization_quantity(name));
}

Eigen::MatrixXf ConstMeshIGLView::get_face_corner_parameterization_quantity(
    const std::string& name) const
{
    return vec2f_array_to_matrix(
        mesh_.get_face_corner_parameterization_quantity(name));
}

// Utility functions for ConstMeshIGLView
Eigen::MatrixXf ConstMeshIGLView::vec3f_array_to_matrix(
    const std::vector<glm::vec3>& array)
{
    if (array.empty()) {
        return Eigen::MatrixXf(0, 3);
    }

    // Use Eigen Map for zero-copy when possible
    // Note: glm::vec3 uses floats, so we can map directly
    Eigen::MatrixXf matrix(array.size(), 3);
    for (size_t i = 0; i < array.size(); ++i) {
        matrix.row(i) = Eigen::Map<const Eigen::Vector3f>(&array[i][0]);
    }
    return matrix;
}

Eigen::MatrixXf ConstMeshIGLView::vec2f_array_to_matrix(
    const std::vector<glm::vec2>& array)
{
    if (array.empty()) {
        return Eigen::MatrixXf(0, 2);
    }

    Eigen::MatrixXf matrix(array.size(), 2);
    for (size_t i = 0; i < array.size(); ++i) {
        matrix.row(i) = Eigen::Map<const Eigen::Vector2f>(&array[i][0]);
    }
    return matrix;
}

Eigen::VectorXf ConstMeshIGLView::float_array_to_vector(
    const std::vector<float>& array)
{
    if (array.empty()) {
        return Eigen::VectorXf(0);
    }

    // Use Eigen Map for zero-copy
    return Eigen::Map<const Eigen::VectorXf>(array.data(), array.size());
}

Eigen::VectorXi ConstMeshIGLView::int_array_to_vector(
    const std::vector<int>& array)
{
    Eigen::VectorXi vector(array.size());
    std::memcpy(vector.data(), array.data(), array.size() * sizeof(int));
    return vector;
}

// MeshIGLView Implementation
MeshIGLView::MeshIGLView(MeshComponent& mesh)
    : ConstMeshIGLView(mesh),
      mutable_mesh_(mesh)
{
}

MeshIGLView::~MeshIGLView()
{
}

void MeshIGLView::set_vertices(const Eigen::MatrixXf& vertices)
{
    mutable_mesh_.set_vertices(matrix_to_vec3f_array(vertices));
}

void MeshIGLView::set_faces(const Eigen::MatrixXi& faces)
{
    // Convert triangular faces to face vertex counts and indices
    std::vector<int> face_vertex_counts(faces.rows(), 3);
    std::vector<int> face_vertex_indices(faces.rows() * 3);

    for (int i = 0; i < faces.rows(); ++i) {
        face_vertex_indices[i * 3] = faces(i, 0);
        face_vertex_indices[i * 3 + 1] = faces(i, 1);
        face_vertex_indices[i * 3 + 2] = faces(i, 2);
    }

    mutable_mesh_.set_face_vertex_counts(face_vertex_counts);
    mutable_mesh_.set_face_vertex_indices(face_vertex_indices);
}

void MeshIGLView::set_face_topology(
    const Eigen::VectorXi& face_vertex_counts,
    const Eigen::VectorXi& face_vertex_indices)
{
    mutable_mesh_.set_face_vertex_counts(
        vector_to_int_array(face_vertex_counts));
    mutable_mesh_.set_face_vertex_indices(
        vector_to_int_array(face_vertex_indices));
}

void MeshIGLView::set_normals(const Eigen::MatrixXf& normals)
{
    mutable_mesh_.set_normals(matrix_to_vec3f_array(normals));
}

void MeshIGLView::set_uv_coordinates(const Eigen::MatrixXf& uv_coords)
{
    mutable_mesh_.set_texcoords_array(matrix_to_vec2f_array(uv_coords));
}

void MeshIGLView::set_display_colors(const Eigen::MatrixXf& colors)
{
    mutable_mesh_.set_display_color(matrix_to_vec3f_array(colors));
}

void MeshIGLView::set_vertex_scalar_quantity(
    const std::string& name,
    const Eigen::VectorXf& values)
{
    mutable_mesh_.add_vertex_scalar_quantity(
        name, vector_to_float_array(values));
}

void MeshIGLView::set_face_scalar_quantity(
    const std::string& name,
    const Eigen::VectorXf& values)
{
    mutable_mesh_.add_face_scalar_quantity(name, vector_to_float_array(values));
}

void MeshIGLView::set_vertex_vector_quantity(
    const std::string& name,
    const Eigen::MatrixXf& vectors)
{
    mutable_mesh_.add_vertex_vector_quantity(
        name, matrix_to_vec3f_array(vectors));
}

void MeshIGLView::set_face_vector_quantity(
    const std::string& name,
    const Eigen::MatrixXf& vectors)
{
    mutable_mesh_.add_face_vector_quantity(
        name, matrix_to_vec3f_array(vectors));
}

void MeshIGLView::set_vertex_parameterization_quantity(
    const std::string& name,
    const Eigen::MatrixXf& params)
{
    mutable_mesh_.add_vertex_parameterization_quantity(
        name, matrix_to_vec2f_array(params));
}

void MeshIGLView::set_face_corner_parameterization_quantity(
    const std::string& name,
    const Eigen::MatrixXf& params)
{
    mutable_mesh_.add_face_corner_parameterization_quantity(
        name, matrix_to_vec2f_array(params));
}

// Utility functions for MeshIGLView
std::vector<glm::vec3> MeshIGLView::matrix_to_vec3f_array(
    const Eigen::MatrixXf& matrix)
{
    std::vector<glm::vec3> array(matrix.rows());
    for (int i = 0; i < matrix.rows(); ++i) {
        array[i] = glm::vec3(matrix(i, 0), matrix(i, 1), matrix(i, 2));
    }
    return array;
}

std::vector<glm::vec2> MeshIGLView::matrix_to_vec2f_array(
    const Eigen::MatrixXf& matrix)
{
    std::vector<glm::vec2> array(matrix.rows());
    for (int i = 0; i < matrix.rows(); ++i) {
        array[i] = glm::vec2(matrix(i, 0), matrix(i, 1));
    }
    return array;
}

std::vector<float> MeshIGLView::vector_to_float_array(
    const Eigen::VectorXf& vector)
{
    std::vector<float> array(vector.size());
    // Use zero-copy with Map when possible
    std::memcpy(array.data(), vector.data(), vector.size() * sizeof(float));
    return array;
}

std::vector<int> MeshIGLView::vector_to_int_array(const Eigen::VectorXi& vector)
{
    std::vector<int> array(vector.size());
    std::memcpy(array.data(), vector.data(), vector.size() * sizeof(int));
    return array;
}

// External API function implementations
MeshIGLView get_igl_view(MeshComponent& mesh)
{
    return MeshIGLView(mesh);
}

ConstMeshIGLView get_igl_view(const MeshComponent& mesh)
{
    return ConstMeshIGLView(mesh);
}
#ifdef GEOM_USD_EXTENSION

// ConstMeshUSDView Implementation
ConstMeshUSDView::ConstMeshUSDView(const MeshComponent& mesh) : mesh_(mesh)
{
    mesh_.acquire_view_lock();
}

ConstMeshUSDView::~ConstMeshUSDView()
{
    mesh_.release_view_lock();
}

pxr::VtArray<pxr::GfVec3f> ConstMeshUSDView::get_vertices() const
{
    return vec3f_array_to_vt_array(mesh_.get_vertices());
}

pxr::VtArray<int> ConstMeshUSDView::get_face_vertex_counts() const
{
    return int_array_to_vt_array(mesh_.get_face_vertex_counts());
}

pxr::VtArray<int> ConstMeshUSDView::get_face_vertex_indices() const
{
    return int_array_to_vt_array(mesh_.get_face_vertex_indices());
}

pxr::VtArray<pxr::GfVec3f> ConstMeshUSDView::get_normals() const
{
    return vec3f_array_to_vt_array(mesh_.get_normals());
}

pxr::VtArray<pxr::GfVec2f> ConstMeshUSDView::get_uv_coordinates() const
{
    return vec2f_array_to_vt_array(mesh_.get_texcoords_array());
}

pxr::VtArray<pxr::GfVec3f> ConstMeshUSDView::get_display_colors() const
{
    return vec3f_array_to_vt_array(mesh_.get_display_color());
}

pxr::VtArray<float> ConstMeshUSDView::get_vertex_scalar_quantity(
    const std::string& name) const
{
    return float_array_to_vt_array(mesh_.get_vertex_scalar_quantity(name));
}

pxr::VtArray<float> ConstMeshUSDView::get_face_scalar_quantity(
    const std::string& name) const
{
    return float_array_to_vt_array(mesh_.get_face_scalar_quantity(name));
}

pxr::VtArray<pxr::GfVec3f> ConstMeshUSDView::get_vertex_vector_quantity(
    const std::string& name) const
{
    return vec3f_array_to_vt_array(mesh_.get_vertex_vector_quantity(name));
}

pxr::VtArray<pxr::GfVec3f> ConstMeshUSDView::get_face_vector_quantity(
    const std::string& name) const
{
    return vec3f_array_to_vt_array(mesh_.get_face_vector_quantity(name));
}

pxr::VtArray<pxr::GfVec2f>
ConstMeshUSDView::get_vertex_parameterization_quantity(
    const std::string& name) const
{
    return vec2f_array_to_vt_array(
        mesh_.get_vertex_parameterization_quantity(name));
}

pxr::VtArray<pxr::GfVec2f>
ConstMeshUSDView::get_face_corner_parameterization_quantity(
    const std::string& name) const
{
    return vec2f_array_to_vt_array(
        mesh_.get_face_corner_parameterization_quantity(name));
}

// Utility functions for ConstMeshUSDView
pxr::VtArray<pxr::GfVec3f> ConstMeshUSDView::vec3f_array_to_vt_array(
    const std::vector<glm::vec3>& array)
{
    // glm::vec3 and pxr::GfVec3f have compatible memory layout (both are 3
    // floats)
    static_assert(
        sizeof(glm::vec3) == sizeof(pxr::GfVec3f),
        "glm::vec3 and pxr::GfVec3f must have the same size");
    return pxr::VtArray<pxr::GfVec3f>(
        reinterpret_cast<const pxr::GfVec3f*>(array.data()),
        reinterpret_cast<const pxr::GfVec3f*>(array.data() + array.size()));
}

pxr::VtArray<pxr::GfVec2f> ConstMeshUSDView::vec2f_array_to_vt_array(
    const std::vector<glm::vec2>& array)
{
    // glm::vec2 and pxr::GfVec2f have compatible memory layout (both are 2
    // floats)
    static_assert(
        sizeof(glm::vec2) == sizeof(pxr::GfVec2f),
        "glm::vec2 and pxr::GfVec2f must have the same size");
    return pxr::VtArray<pxr::GfVec2f>(
        reinterpret_cast<const pxr::GfVec2f*>(array.data()),
        reinterpret_cast<const pxr::GfVec2f*>(array.data() + array.size()));
}

pxr::VtArray<float> ConstMeshUSDView::float_array_to_vt_array(
    const std::vector<float>& array)
{
    return pxr::VtArray<float>(array.begin(), array.end());
}

pxr::VtArray<int> ConstMeshUSDView::int_array_to_vt_array(
    const std::vector<int>& array)
{
    return pxr::VtArray<int>(array.begin(), array.end());
}

// MeshUSDView Implementation
MeshUSDView::MeshUSDView(MeshComponent& mesh)
    : ConstMeshUSDView(mesh),
      mutable_mesh_(mesh)
{
}

MeshUSDView::~MeshUSDView()
{
}

void MeshUSDView::set_vertices(const pxr::VtArray<pxr::GfVec3f>& vertices)
{
    mutable_mesh_.set_vertices(vt_array_to_vec3f_array(vertices));
}

void MeshUSDView::set_face_topology(
    const pxr::VtArray<int>& face_vertex_counts,
    const pxr::VtArray<int>& face_vertex_indices)
{
    mutable_mesh_.set_face_vertex_counts(
        vt_array_to_int_array(face_vertex_counts));
    mutable_mesh_.set_face_vertex_indices(
        vt_array_to_int_array(face_vertex_indices));
}

void MeshUSDView::set_normals(const pxr::VtArray<pxr::GfVec3f>& normals)
{
    mutable_mesh_.set_normals(vt_array_to_vec3f_array(normals));
}

void MeshUSDView::set_uv_coordinates(
    const pxr::VtArray<pxr::GfVec2f>& uv_coords)
{
    mutable_mesh_.set_texcoords_array(vt_array_to_vec2f_array(uv_coords));
}

void MeshUSDView::set_display_colors(const pxr::VtArray<pxr::GfVec3f>& colors)
{
    mutable_mesh_.set_display_color(vt_array_to_vec3f_array(colors));
}

void MeshUSDView::set_vertex_scalar_quantity(
    const std::string& name,
    const pxr::VtArray<float>& values)
{
    mutable_mesh_.add_vertex_scalar_quantity(
        name, vt_array_to_float_array(values));
}

void MeshUSDView::set_face_scalar_quantity(
    const std::string& name,
    const pxr::VtArray<float>& values)
{
    mutable_mesh_.add_face_scalar_quantity(
        name, vt_array_to_float_array(values));
}

void MeshUSDView::set_vertex_vector_quantity(
    const std::string& name,
    const pxr::VtArray<pxr::GfVec3f>& vectors)
{
    mutable_mesh_.add_vertex_vector_quantity(
        name, vt_array_to_vec3f_array(vectors));
}

void MeshUSDView::set_face_vector_quantity(
    const std::string& name,
    const pxr::VtArray<pxr::GfVec3f>& vectors)
{
    mutable_mesh_.add_face_vector_quantity(
        name, vt_array_to_vec3f_array(vectors));
}

void MeshUSDView::set_vertex_parameterization_quantity(
    const std::string& name,
    const pxr::VtArray<pxr::GfVec2f>& params)
{
    mutable_mesh_.add_vertex_parameterization_quantity(
        name, vt_array_to_vec2f_array(params));
}

void MeshUSDView::set_face_corner_parameterization_quantity(
    const std::string& name,
    const pxr::VtArray<pxr::GfVec2f>& params)
{
    mutable_mesh_.add_face_corner_parameterization_quantity(
        name, vt_array_to_vec2f_array(params));
}

// Utility functions for MeshUSDView
std::vector<glm::vec3> MeshUSDView::vt_array_to_vec3f_array(
    const pxr::VtArray<pxr::GfVec3f>& array)
{
    std::vector<glm::vec3> result(array.size());
    for (size_t i = 0; i < array.size(); ++i) {
        result[i] = glm::vec3(array[i][0], array[i][1], array[i][2]);
    }
    return result;
}

std::vector<glm::vec2> MeshUSDView::vt_array_to_vec2f_array(
    const pxr::VtArray<pxr::GfVec2f>& array)
{
    std::vector<glm::vec2> result(array.size());
    for (size_t i = 0; i < array.size(); ++i) {
        result[i] = glm::vec2(array[i][0], array[i][1]);
    }
    return result;
}

std::vector<float> MeshUSDView::vt_array_to_float_array(
    const pxr::VtArray<float>& array)
{
    std::vector<float> result(array.size());
    std::memcpy(result.data(), array.data(), array.size() * sizeof(float));
    return result;
}

std::vector<int> MeshUSDView::vt_array_to_int_array(
    const pxr::VtArray<int>& array)
{
    std::vector<int> result(array.size());
    std::memcpy(result.data(), array.data(), array.size() * sizeof(int));
    return result;
}

MeshUSDView get_usd_view(MeshComponent& mesh)
{
    return MeshUSDView(mesh);
}

ConstMeshUSDView get_usd_view(const MeshComponent& mesh)
{
    return ConstMeshUSDView(mesh);
}

#endif

#if RUZINO_WITH_CUDA
// ===== CUDA View Implementation =====

ConstMeshCUDAView::ConstMeshCUDAView(const MeshComponent& mesh) : mesh_(mesh)
{
    mesh_.acquire_view_lock();
}

ConstMeshCUDAView::~ConstMeshCUDAView()
{
    mesh_.release_view_lock();
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_or_create_buffer(
    const std::string& key,
    const void* data,
    size_t element_count,
    size_t element_size) const
{
    auto it = cuda_buffers_.find(key);
    if (it != cuda_buffers_.end()) {
        return it->second;
    }

    // Lazy create buffer
    cuda::CUDALinearBufferDesc desc(element_count, element_size);
    auto buffer =
        cuda::create_cuda_linear_buffer(desc, const_cast<void*>(data));
    cuda_buffers_[key] = buffer;
    return buffer;
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_vertices() const
{
    auto vertices = mesh_.get_vertices();
    return get_or_create_buffer(
        "vertices", vertices.data(), vertices.size(), sizeof(glm::vec3));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_face_vertex_counts() const
{
    auto counts = mesh_.get_face_vertex_counts();
    return get_or_create_buffer(
        "face_vertex_counts", counts.data(), counts.size(), sizeof(int));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_face_vertex_indices() const
{
    auto indices = mesh_.get_face_vertex_indices();
    return get_or_create_buffer(
        "face_vertex_indices", indices.data(), indices.size(), sizeof(int));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_normals() const
{
    auto normals = mesh_.get_normals();
    return get_or_create_buffer(
        "normals", normals.data(), normals.size(), sizeof(glm::vec3));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_uv_coordinates() const
{
    auto uvs = mesh_.get_texcoords_array();
    return get_or_create_buffer(
        "uv_coordinates", uvs.data(), uvs.size(), sizeof(glm::vec2));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_display_colors() const
{
    auto colors = mesh_.get_display_color();
    return get_or_create_buffer(
        "display_colors", colors.data(), colors.size(), sizeof(glm::vec3));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_vertex_scalar_quantity(
    const std::string& name) const
{
    auto values = mesh_.get_vertex_scalar_quantity(name);
    return get_or_create_buffer(
        "vertex_scalar_" + name, values.data(), values.size(), sizeof(float));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_face_scalar_quantity(
    const std::string& name) const
{
    auto values = mesh_.get_face_scalar_quantity(name);
    return get_or_create_buffer(
        "face_scalar_" + name, values.data(), values.size(), sizeof(float));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_vertex_vector_quantity(
    const std::string& name) const
{
    auto vectors = mesh_.get_vertex_vector_quantity(name);
    return get_or_create_buffer(
        "vertex_vector_" + name,
        vectors.data(),
        vectors.size(),
        sizeof(glm::vec3));
}

cuda::CUDALinearBufferHandle ConstMeshCUDAView::get_face_vector_quantity(
    const std::string& name) const
{
    auto vectors = mesh_.get_face_vector_quantity(name);
    return get_or_create_buffer(
        "face_vector_" + name,
        vectors.data(),
        vectors.size(),
        sizeof(glm::vec3));
}

cuda::CUDALinearBufferHandle
ConstMeshCUDAView::get_vertex_parameterization_quantity(
    const std::string& name) const
{
    auto params = mesh_.get_vertex_parameterization_quantity(name);
    return get_or_create_buffer(
        "vertex_param_" + name,
        params.data(),
        params.size(),
        sizeof(glm::vec2));
}

cuda::CUDALinearBufferHandle
ConstMeshCUDAView::get_face_corner_parameterization_quantity(
    const std::string& name) const
{
    auto params = mesh_.get_face_corner_parameterization_quantity(name);
    return get_or_create_buffer(
        "face_corner_param_" + name,
        params.data(),
        params.size(),
        sizeof(glm::vec2));
}

MeshCUDAView::MeshCUDAView(MeshComponent& mesh)
    : ConstMeshCUDAView(mesh),
      mutable_mesh_(mesh)
{
}

MeshCUDAView::~MeshCUDAView()
{
    // RAII: sync all modified buffers back to CPU
    sync_all_to_cpu();
}

void MeshCUDAView::sync_buffer_to_cpu(const std::string& buffer_name)
{
    auto it = cuda_buffers_.find(buffer_name);
    if (it == cuda_buffers_.end()) {
        return;
    }

    auto buffer = it->second;

    // Sync based on buffer type
    if (buffer_name == "vertices") {
        mutable_mesh_.set_vertices(buffer->get_host_vector<glm::vec3>());
    }
    else if (buffer_name == "face_vertex_counts") {
        mutable_mesh_.set_face_vertex_counts(buffer->get_host_vector<int>());
    }
    else if (buffer_name == "face_vertex_indices") {
        mutable_mesh_.set_face_vertex_indices(buffer->get_host_vector<int>());
    }
    else if (buffer_name == "normals") {
        mutable_mesh_.set_normals(buffer->get_host_vector<glm::vec3>());
    }
    else if (buffer_name == "uv_coordinates") {
        mutable_mesh_.set_texcoords_array(buffer->get_host_vector<glm::vec2>());
    }
    else if (buffer_name == "display_colors") {
        mutable_mesh_.set_display_color(buffer->get_host_vector<glm::vec3>());
    }
    else if (buffer_name.substr(0, 14) == "vertex_scalar_") {
        auto name = buffer_name.substr(14);
        mutable_mesh_.add_vertex_scalar_quantity(
            name, buffer->get_host_vector<float>());
    }
    else if (buffer_name.substr(0, 12) == "face_scalar_") {
        auto name = buffer_name.substr(12);
        mutable_mesh_.add_face_scalar_quantity(
            name, buffer->get_host_vector<float>());
    }
    else if (buffer_name.substr(0, 14) == "vertex_vector_") {
        auto name = buffer_name.substr(14);
        mutable_mesh_.add_vertex_vector_quantity(
            name, buffer->get_host_vector<glm::vec3>());
    }
    else if (buffer_name.substr(0, 12) == "face_vector_") {
        auto name = buffer_name.substr(12);
        mutable_mesh_.add_face_vector_quantity(
            name, buffer->get_host_vector<glm::vec3>());
    }
    else if (buffer_name.substr(0, 13) == "vertex_param_") {
        auto name = buffer_name.substr(13);
        mutable_mesh_.add_vertex_parameterization_quantity(
            name, buffer->get_host_vector<glm::vec2>());
    }
    else if (buffer_name.substr(0, 18) == "face_corner_param_") {
        auto name = buffer_name.substr(18);
        mutable_mesh_.add_face_corner_parameterization_quantity(
            name, buffer->get_host_vector<glm::vec2>());
    }
}

void MeshCUDAView::sync_all_to_cpu()
{
    for (const auto& buffer_name : modified_buffers_) {
        sync_buffer_to_cpu(buffer_name);
    }
    modified_buffers_.clear();
}

void MeshCUDAView::set_vertices(cuda::CUDALinearBufferHandle vertices)
{
    cuda_buffers_["vertices"] = vertices;
    modified_buffers_.insert("vertices");
}

void MeshCUDAView::set_face_topology(
    cuda::CUDALinearBufferHandle face_vertex_counts,
    cuda::CUDALinearBufferHandle face_vertex_indices)
{
    cuda_buffers_["face_vertex_counts"] = face_vertex_counts;
    cuda_buffers_["face_vertex_indices"] = face_vertex_indices;
    modified_buffers_.insert("face_vertex_counts");
    modified_buffers_.insert("face_vertex_indices");
}

void MeshCUDAView::set_normals(cuda::CUDALinearBufferHandle normals)
{
    cuda_buffers_["normals"] = normals;
    modified_buffers_.insert("normals");
}

void MeshCUDAView::set_uv_coordinates(cuda::CUDALinearBufferHandle uv_coords)
{
    cuda_buffers_["uv_coordinates"] = uv_coords;
    modified_buffers_.insert("uv_coordinates");
}

void MeshCUDAView::set_display_colors(cuda::CUDALinearBufferHandle colors)
{
    cuda_buffers_["display_colors"] = colors;
    modified_buffers_.insert("display_colors");
}

void MeshCUDAView::set_vertex_scalar_quantity(
    const std::string& name,
    cuda::CUDALinearBufferHandle values)
{
    auto key = "vertex_scalar_" + name;
    cuda_buffers_[key] = values;
    modified_buffers_.insert(key);
}

void MeshCUDAView::set_face_scalar_quantity(
    const std::string& name,
    cuda::CUDALinearBufferHandle values)
{
    auto key = "face_scalar_" + name;
    cuda_buffers_[key] = values;
    modified_buffers_.insert(key);
}

void MeshCUDAView::set_vertex_vector_quantity(
    const std::string& name,
    cuda::CUDALinearBufferHandle vectors)
{
    auto key = "vertex_vector_" + name;
    cuda_buffers_[key] = vectors;
    modified_buffers_.insert(key);
}

void MeshCUDAView::set_face_vector_quantity(
    const std::string& name,
    cuda::CUDALinearBufferHandle vectors)
{
    auto key = "face_vector_" + name;
    cuda_buffers_[key] = vectors;
    modified_buffers_.insert(key);
}

void MeshCUDAView::set_vertex_parameterization_quantity(
    const std::string& name,
    cuda::CUDALinearBufferHandle params)
{
    auto key = "vertex_param_" + name;
    cuda_buffers_[key] = params;
    modified_buffers_.insert(key);
}

void MeshCUDAView::set_face_corner_parameterization_quantity(
    const std::string& name,
    cuda::CUDALinearBufferHandle params)
{
    auto key = "face_corner_param_" + name;
    cuda_buffers_[key] = params;
    modified_buffers_.insert(key);
}

MeshCUDAView get_cuda_view(MeshComponent& mesh)
{
    return MeshCUDAView(mesh);
}

ConstMeshCUDAView get_cuda_view(const MeshComponent& mesh)
{
    return ConstMeshCUDAView(mesh);
}
#endif

// ===== NVRHI View Implementation =====

ConstMeshNVRHIView::ConstMeshNVRHIView(
    const MeshComponent& mesh,
    nvrhi::IDevice* device)
    : mesh_(mesh),
      device_(device)
{
    mesh_.acquire_view_lock();
}

ConstMeshNVRHIView::~ConstMeshNVRHIView()
{
    mesh_.release_view_lock();
}

nvrhi::BufferHandle ConstMeshNVRHIView::get_or_create_buffer(
    const std::string& key,
    const void* data,
    size_t element_count,
    size_t element_size,
    nvrhi::ICommandList* commandList) const
{
    auto it = nvrhi_buffers_.find(key);
    if (it != nvrhi_buffers_.end()) {
        return it->second;
    }

    // Lazy create buffer
    nvrhi::BufferDesc desc;
    desc.byteSize = element_count * element_size;
    desc.debugName = key;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.canHaveUAVs = true;
    desc.isVertexBuffer =
        (key == "vertices" || key == "normals" || key == "uv_coordinates" ||
         key == "display_colors");
    desc.isIndexBuffer = (key == "face_vertex_indices");

    auto buffer = device_->createBuffer(desc);

    // Upload initial data if commandList is provided
    if (commandList && data && element_count > 0) {
        commandList->writeBuffer(buffer, data, element_count * element_size);
    }

    nvrhi_buffers_[key] = buffer;
    return buffer;
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_vertices(
    nvrhi::ICommandList* commandList) const
{
    auto vertices = mesh_.get_vertices();
    return get_or_create_buffer(
        "vertices",
        vertices.data(),
        vertices.size(),
        sizeof(glm::vec3),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_face_vertex_counts(
    nvrhi::ICommandList* commandList) const
{
    auto counts = mesh_.get_face_vertex_counts();
    return get_or_create_buffer(
        "face_vertex_counts",
        counts.data(),
        counts.size(),
        sizeof(int),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_face_vertex_indices(
    nvrhi::ICommandList* commandList) const
{
    auto indices = mesh_.get_face_vertex_indices();
    return get_or_create_buffer(
        "face_vertex_indices",
        indices.data(),
        indices.size(),
        sizeof(int),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_normals(
    nvrhi::ICommandList* commandList) const
{
    auto normals = mesh_.get_normals();
    return get_or_create_buffer(
        "normals",
        normals.data(),
        normals.size(),
        sizeof(glm::vec3),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_uv_coordinates(
    nvrhi::ICommandList* commandList) const
{
    auto uvs = mesh_.get_texcoords_array();
    return get_or_create_buffer(
        "uv_coordinates",
        uvs.data(),
        uvs.size(),
        sizeof(glm::vec2),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_display_colors(
    nvrhi::ICommandList* commandList) const
{
    auto colors = mesh_.get_display_color();
    return get_or_create_buffer(
        "display_colors",
        colors.data(),
        colors.size(),
        sizeof(glm::vec3),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_vertex_scalar_quantity(
    const std::string& name,
    nvrhi::ICommandList* commandList) const
{
    auto values = mesh_.get_vertex_scalar_quantity(name);
    return get_or_create_buffer(
        "vertex_scalar_" + name,
        values.data(),
        values.size(),
        sizeof(float),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_face_scalar_quantity(
    const std::string& name,
    nvrhi::ICommandList* commandList) const
{
    auto values = mesh_.get_face_scalar_quantity(name);
    return get_or_create_buffer(
        "face_scalar_" + name,
        values.data(),
        values.size(),
        sizeof(float),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_vertex_vector_quantity(
    const std::string& name,
    nvrhi::ICommandList* commandList) const
{
    auto vectors = mesh_.get_vertex_vector_quantity(name);
    return get_or_create_buffer(
        "vertex_vector_" + name,
        vectors.data(),
        vectors.size(),
        sizeof(glm::vec3),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_face_vector_quantity(
    const std::string& name,
    nvrhi::ICommandList* commandList) const
{
    auto vectors = mesh_.get_face_vector_quantity(name);
    return get_or_create_buffer(
        "face_vector_" + name,
        vectors.data(),
        vectors.size(),
        sizeof(glm::vec3),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_vertex_parameterization_quantity(
    const std::string& name,
    nvrhi::ICommandList* commandList) const
{
    auto params = mesh_.get_vertex_parameterization_quantity(name);
    return get_or_create_buffer(
        "vertex_param_" + name,
        params.data(),
        params.size(),
        sizeof(glm::vec2),
        commandList);
}

nvrhi::IBuffer* ConstMeshNVRHIView::get_face_corner_parameterization_quantity(
    const std::string& name,
    nvrhi::ICommandList* commandList) const
{
    auto params = mesh_.get_face_corner_parameterization_quantity(name);
    return get_or_create_buffer(
        "face_corner_param_" + name,
        params.data(),
        params.size(),
        sizeof(glm::vec2),
        commandList);
}

MeshNVRHIView::MeshNVRHIView(MeshComponent& mesh, nvrhi::IDevice* device)
    : ConstMeshNVRHIView(mesh, device),
      mutable_mesh_(mesh)
{
}

MeshNVRHIView::~MeshNVRHIView()
{
    // Just clear tracking, no automatic sync
    // User should manually copy data back if needed using staging buffers
}

void MeshNVRHIView::set_vertices(nvrhi::IBuffer* vertices)
{
    nvrhi_buffers_["vertices"] = vertices;
    modified_buffers_.insert("vertices");
}

void MeshNVRHIView::set_face_topology(
    nvrhi::IBuffer* face_vertex_counts,
    nvrhi::IBuffer* face_vertex_indices)
{
    nvrhi_buffers_["face_vertex_counts"] = face_vertex_counts;
    nvrhi_buffers_["face_vertex_indices"] = face_vertex_indices;
    modified_buffers_.insert("face_vertex_counts");
    modified_buffers_.insert("face_vertex_indices");
}

void MeshNVRHIView::set_normals(nvrhi::IBuffer* normals)
{
    nvrhi_buffers_["normals"] = normals;
    modified_buffers_.insert("normals");
}

void MeshNVRHIView::set_uv_coordinates(nvrhi::IBuffer* uv_coords)
{
    nvrhi_buffers_["uv_coordinates"] = uv_coords;
    modified_buffers_.insert("uv_coordinates");
}

void MeshNVRHIView::set_display_colors(nvrhi::IBuffer* colors)
{
    nvrhi_buffers_["display_colors"] = colors;
    modified_buffers_.insert("display_colors");
}

void MeshNVRHIView::set_vertex_scalar_quantity(
    const std::string& name,
    nvrhi::IBuffer* values)
{
    auto key = "vertex_scalar_" + name;
    nvrhi_buffers_[key] = values;
    modified_buffers_.insert(key);
}

void MeshNVRHIView::set_face_scalar_quantity(
    const std::string& name,
    nvrhi::IBuffer* values)
{
    auto key = "face_scalar_" + name;
    nvrhi_buffers_[key] = values;
    modified_buffers_.insert(key);
}

void MeshNVRHIView::set_vertex_vector_quantity(
    const std::string& name,
    nvrhi::IBuffer* vectors)
{
    auto key = "vertex_vector_" + name;
    nvrhi_buffers_[key] = vectors;
    modified_buffers_.insert(key);
}

void MeshNVRHIView::set_face_vector_quantity(
    const std::string& name,
    nvrhi::IBuffer* vectors)
{
    auto key = "face_vector_" + name;
    nvrhi_buffers_[key] = vectors;
    modified_buffers_.insert(key);
}

void MeshNVRHIView::set_vertex_parameterization_quantity(
    const std::string& name,
    nvrhi::IBuffer* params)
{
    auto key = "vertex_param_" + name;
    nvrhi_buffers_[key] = params;
    modified_buffers_.insert(key);
}

void MeshNVRHIView::set_face_corner_parameterization_quantity(
    const std::string& name,
    nvrhi::IBuffer* params)
{
    auto key = "face_corner_param_" + name;
    nvrhi_buffers_[key] = params;
    modified_buffers_.insert(key);
}

MeshNVRHIView get_nvrhi_view(MeshComponent& mesh, nvrhi::IDevice* device)
{
    return MeshNVRHIView(mesh, device);
}

ConstMeshNVRHIView get_nvrhi_view(
    const MeshComponent& mesh,
    nvrhi::IDevice* device)
{
    return ConstMeshNVRHIView(mesh, device);
}

RUZINO_NAMESPACE_CLOSE_SCOPE
