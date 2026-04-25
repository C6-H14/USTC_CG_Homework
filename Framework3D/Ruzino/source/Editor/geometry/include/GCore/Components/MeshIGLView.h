#pragma once
#include <GCore/api.h>

#include <Eigen/Eigen>
#include <string>
#include <vector>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
struct MeshComponent;

struct GEOMETRY_API ConstMeshIGLView {
    explicit ConstMeshIGLView(const MeshComponent& mesh);
    ~ConstMeshIGLView();

    // Delete copy and move to prevent bypassing view lock
    ConstMeshIGLView(const ConstMeshIGLView&) = delete;
    ConstMeshIGLView& operator=(const ConstMeshIGLView&) = delete;
    ConstMeshIGLView(ConstMeshIGLView&&) = delete;
    ConstMeshIGLView& operator=(ConstMeshIGLView&&) = delete;

    // Vertex positions as Nx3 matrix (using float for better compatibility)
    Eigen::MatrixXf get_vertices() const;

    // Face indices as Fx3 matrix (assuming triangular faces)
    Eigen::MatrixXi get_faces() const;

    // Face vertex counts
    Eigen::VectorXi get_face_vertex_counts() const;

    // Face vertex indices (for general polygons)
    Eigen::VectorXi get_face_vertex_indices() const;

    // Normals as Nx3 matrix
    Eigen::MatrixXf get_normals() const;

    // UV coordinates as Nx2 matrix
    Eigen::MatrixXf get_uv_coordinates() const;

    // Display colors as Nx3 matrix
    Eigen::MatrixXf get_display_colors() const;

    // Get scalar quantities
    Eigen::VectorXf get_vertex_scalar_quantity(const std::string& name) const;
    Eigen::VectorXf get_face_scalar_quantity(const std::string& name) const;

    // Get vector quantities
    Eigen::MatrixXf get_vertex_vector_quantity(const std::string& name) const;
    Eigen::MatrixXf get_face_vector_quantity(const std::string& name) const;

    // Get parameterization quantities
    Eigen::MatrixXf get_vertex_parameterization_quantity(
        const std::string& name) const;
    Eigen::MatrixXf get_face_corner_parameterization_quantity(
        const std::string& name) const;

   protected:
    const MeshComponent& mesh_;

    // Utility functions for conversion using Eigen Map for zero-copy when
    // possible
    static Eigen::MatrixXf vec3f_array_to_matrix(
        const std::vector<glm::vec3>& array);
    static Eigen::MatrixXf vec2f_array_to_matrix(
        const std::vector<glm::vec2>& array);
    static Eigen::VectorXf float_array_to_vector(
        const std::vector<float>& array);
    static Eigen::VectorXi int_array_to_vector(const std::vector<int>& array);
};

struct GEOMETRY_API MeshIGLView : public ConstMeshIGLView {
    MeshIGLView(MeshComponent& mesh);
    ~MeshIGLView();

    // Delete copy and move to prevent bypassing view lock
    MeshIGLView(const MeshIGLView&) = delete;
    MeshIGLView& operator=(const MeshIGLView&) = delete;
    MeshIGLView(MeshIGLView&&) = delete;
    MeshIGLView& operator=(MeshIGLView&&) = delete;

    // Set vertex positions from Nx3 matrix
    void set_vertices(const Eigen::MatrixXf& vertices);

    // Set face indices from Fx3 matrix (for triangular meshes)
    void set_faces(const Eigen::MatrixXi& faces);

    // Set face vertex counts and indices (for general polygons)
    void set_face_topology(
        const Eigen::VectorXi& face_vertex_counts,
        const Eigen::VectorXi& face_vertex_indices);

    // Set normals from Nx3 matrix
    void set_normals(const Eigen::MatrixXf& normals);

    // Set UV coordinates from Nx2 matrix
    void set_uv_coordinates(const Eigen::MatrixXf& uv_coords);

    // Set display colors from Nx3 matrix
    void set_display_colors(const Eigen::MatrixXf& colors);

    // Set scalar quantities
    void set_vertex_scalar_quantity(
        const std::string& name,
        const Eigen::VectorXf& values);
    void set_face_scalar_quantity(
        const std::string& name,
        const Eigen::VectorXf& values);

    // Set vector quantities
    void set_vertex_vector_quantity(
        const std::string& name,
        const Eigen::MatrixXf& vectors);
    void set_face_vector_quantity(
        const std::string& name,
        const Eigen::MatrixXf& vectors);

    // Set parameterization quantities
    void set_vertex_parameterization_quantity(
        const std::string& name,
        const Eigen::MatrixXf& params);
    void set_face_corner_parameterization_quantity(
        const std::string& name,
        const Eigen::MatrixXf& params);

   private:
    MeshComponent& mutable_mesh_;

    // Utility functions for conversion
    static std::vector<glm::vec3> matrix_to_vec3f_array(
        const Eigen::MatrixXf& matrix);
    static std::vector<glm::vec2> matrix_to_vec2f_array(
        const Eigen::MatrixXf& matrix);
    static std::vector<float> vector_to_float_array(
        const Eigen::VectorXf& vector);
    static std::vector<int> vector_to_int_array(const Eigen::VectorXi& vector);
};

// External API functions for creating views
GEOMETRY_API MeshIGLView get_igl_view(MeshComponent& mesh);
GEOMETRY_API ConstMeshIGLView get_igl_view(const MeshComponent& mesh);

RUZINO_NAMESPACE_CLOSE_SCOPE
