#pragma once
#include <GCore/api.h>

#ifdef GEOM_USD_EXTENSION

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>

#include <string>
#include <vector>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
struct MeshComponent;

// USD View for working with USD types
struct GEOMETRY_API ConstMeshUSDView {
    explicit ConstMeshUSDView(const MeshComponent& mesh);
    ~ConstMeshUSDView();

    // Get vertex positions as VtArray
    pxr::VtArray<pxr::GfVec3f> get_vertices() const;

    // Get face vertex counts and indices
    pxr::VtArray<int> get_face_vertex_counts() const;
    pxr::VtArray<int> get_face_vertex_indices() const;

    // Get normals as VtArray
    pxr::VtArray<pxr::GfVec3f> get_normals() const;

    // Get UV coordinates as VtArray
    pxr::VtArray<pxr::GfVec2f> get_uv_coordinates() const;

    // Get display colors as VtArray
    pxr::VtArray<pxr::GfVec3f> get_display_colors() const;

    // Get scalar quantities
    pxr::VtArray<float> get_vertex_scalar_quantity(
        const std::string& name) const;
    pxr::VtArray<float> get_face_scalar_quantity(const std::string& name) const;

    // Get vector quantities
    pxr::VtArray<pxr::GfVec3f> get_vertex_vector_quantity(
        const std::string& name) const;
    pxr::VtArray<pxr::GfVec3f> get_face_vector_quantity(
        const std::string& name) const;

    // Get parameterization quantities
    pxr::VtArray<pxr::GfVec2f> get_vertex_parameterization_quantity(
        const std::string& name) const;
    pxr::VtArray<pxr::GfVec2f> get_face_corner_parameterization_quantity(
        const std::string& name) const;

   protected:
    const MeshComponent& mesh_;

    // Utility functions for conversion
    static pxr::VtArray<pxr::GfVec3f> vec3f_array_to_vt_array(
        const std::vector<glm::vec3>& array);
    static pxr::VtArray<pxr::GfVec2f> vec2f_array_to_vt_array(
        const std::vector<glm::vec2>& array);
    static pxr::VtArray<float> float_array_to_vt_array(
        const std::vector<float>& array);
    static pxr::VtArray<int> int_array_to_vt_array(
        const std::vector<int>& array);
};

struct GEOMETRY_API MeshUSDView : public ConstMeshUSDView {
    MeshUSDView(MeshComponent& mesh);
    ~MeshUSDView();

    // Set vertex positions from VtArray
    void set_vertices(const pxr::VtArray<pxr::GfVec3f>& vertices);

    // Set face topology from VtArrays
    void set_face_topology(
        const pxr::VtArray<int>& face_vertex_counts,
        const pxr::VtArray<int>& face_vertex_indices);

    // Set normals from VtArray
    void set_normals(const pxr::VtArray<pxr::GfVec3f>& normals);

    // Set UV coordinates from VtArray
    void set_uv_coordinates(const pxr::VtArray<pxr::GfVec2f>& uv_coords);

    // Set display colors from VtArray
    void set_display_colors(const pxr::VtArray<pxr::GfVec3f>& colors);

    // Set scalar quantities
    void set_vertex_scalar_quantity(
        const std::string& name,
        const pxr::VtArray<float>& values);
    void set_face_scalar_quantity(
        const std::string& name,
        const pxr::VtArray<float>& values);

    // Set vector quantities
    void set_vertex_vector_quantity(
        const std::string& name,
        const pxr::VtArray<pxr::GfVec3f>& vectors);
    void set_face_vector_quantity(
        const std::string& name,
        const pxr::VtArray<pxr::GfVec3f>& vectors);

    // Set parameterization quantities
    void set_vertex_parameterization_quantity(
        const std::string& name,
        const pxr::VtArray<pxr::GfVec2f>& params);
    void set_face_corner_parameterization_quantity(
        const std::string& name,
        const pxr::VtArray<pxr::GfVec2f>& params);

   private:
    MeshComponent& mutable_mesh_;

    // Utility functions for conversion
    static std::vector<glm::vec3> vt_array_to_vec3f_array(
        const pxr::VtArray<pxr::GfVec3f>& array);
    static std::vector<glm::vec2> vt_array_to_vec2f_array(
        const pxr::VtArray<pxr::GfVec2f>& array);
    static std::vector<float> vt_array_to_float_array(
        const pxr::VtArray<float>& array);
    static std::vector<int> vt_array_to_int_array(
        const pxr::VtArray<int>& array);
};

// External API functions for creating views
GEOMETRY_API MeshUSDView get_usd_view(MeshComponent& mesh);
GEOMETRY_API ConstMeshUSDView get_usd_view(const MeshComponent& mesh);

RUZINO_NAMESPACE_CLOSE_SCOPE

#endif  // GEOM_USD_EXTENSION
