#pragma once

#include <map>
#include <string>
#include <vector>

#include "GCore/Components.h"
#include "GCore/GOP.h"

RUZINO_NAMESPACE_OPEN_SCOPE

// Forward declarations for view types
struct ConstMeshIGLView;
struct MeshIGLView;
#ifdef GEOM_USD_EXTENSION
struct ConstMeshUSDView;
struct MeshUSDView;
#endif
#if RUZINO_WITH_CUDA
struct ConstMeshCUDAView;
struct MeshCUDAView;
#endif
struct ConstMeshNVRHIView;
struct MeshNVRHIView;

struct GEOMETRY_API MeshComponent : public GeometryComponent {
    explicit MeshComponent(Geometry* attached_operand);

    ~MeshComponent() override;

    void apply_transform(const glm::mat4& transform) override;

    std::string to_string() const override;
    GeometryComponentHandle copy(Geometry* operand) const override;

    size_t hash() const override
    {
        size_t h = 0;
        for (const auto& v : vertices) {
            h ^= std::hash<float>{}(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (const auto& i : faceVertexIndices) {
            h ^= std::hash<int>{}(i) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    [[nodiscard]] const std::vector<glm::vec3>& get_vertices() const
    {
        return vertices;
    }

    [[nodiscard]] std::vector<int> get_face_vertex_counts() const
    {
        return faceVertexCounts;
    }

    [[nodiscard]] std::vector<int> get_face_vertex_indices() const
    {
        return faceVertexIndices;
    }

    [[nodiscard]] const std::vector<glm::vec3>& get_normals() const
    {
        return normals;
    }

    [[nodiscard]] const std::vector<glm::vec3>& get_display_color() const
    {
        return displayColor;
    }

    [[nodiscard]] const std::vector<glm::vec2>& get_texcoords_array() const
    {
        return texcoordsArray;
    }

    [[nodiscard]] const std::vector<float>& get_vertex_scalar_quantity(
        const std::string& name) const
    {
        static const std::vector<float> empty;
        auto it = vertex_scalar_quantities.find(name);
        if (it != vertex_scalar_quantities.end()) {
            return it->second;
        }
        return empty;
    }

    [[nodiscard]] std::vector<std::string> get_vertex_scalar_quantity_names()
        const
    {
        std::vector<std::string> names;
        for (const auto& pair : vertex_scalar_quantities) {
            names.push_back(pair.first);
        }
        return names;
    }

    [[nodiscard]] const std::vector<float>& get_face_scalar_quantity(
        const std::string& name) const
    {
        static const std::vector<float> empty;
        auto it = face_scalar_quantities.find(name);
        if (it != face_scalar_quantities.end()) {
            return it->second;
        }
        return empty;
    }

    [[nodiscard]] std::vector<std::string> get_face_scalar_quantity_names()
        const
    {
        std::vector<std::string> names;
        for (const auto& pair : face_scalar_quantities) {
            names.push_back(pair.first);
        }
        return names;
    }

    [[nodiscard]] const std::vector<glm::vec3>& get_vertex_vector_quantity(
        const std::string& name) const
    {
        static const std::vector<glm::vec3> empty;
        auto it = vertex_vector_quantities.find(name);
        if (it != vertex_vector_quantities.end()) {
            return it->second;
        }
        return empty;
    }

    [[nodiscard]] std::vector<std::string> get_vertex_vector_quantity_names()
        const
    {
        std::vector<std::string> names;
        for (const auto& pair : vertex_vector_quantities) {
            names.push_back(pair.first);
        }
        return names;
    }

    [[nodiscard]] const std::vector<glm::vec3>& get_face_vector_quantity(
        const std::string& name) const
    {
        static const std::vector<glm::vec3> empty;
        auto it = face_vector_quantities.find(name);
        if (it != face_vector_quantities.end()) {
            return it->second;
        }
        return empty;
    }

    [[nodiscard]] std::vector<std::string> get_face_vector_quantity_names()
        const
    {
        std::vector<std::string> names;
        for (const auto& pair : face_vector_quantities) {
            names.push_back(pair.first);
        }
        return names;
    }

    [[nodiscard]] const std::vector<glm::vec2>&
    get_face_corner_parameterization_quantity(const std::string& name) const
    {
        static const std::vector<glm::vec2> empty;
        auto it = face_corner_parameterization_quantities.find(name);
        if (it != face_corner_parameterization_quantities.end()) {
            return it->second;
        }
        return empty;
    }

    [[nodiscard]] std::vector<std::string>
    get_face_corner_parameterization_quantity_names() const
    {
        std::vector<std::string> names;
        for (const auto& pair : face_corner_parameterization_quantities) {
            names.push_back(pair.first);
        }
        return names;
    }

    [[nodiscard]] const std::vector<glm::vec2>&
    get_vertex_parameterization_quantity(const std::string& name) const
    {
        static const std::vector<glm::vec2> empty;
        auto it = vertex_parameterization_quantities.find(name);
        if (it != vertex_parameterization_quantities.end()) {
            return it->second;
        }
        return empty;
    }

    [[nodiscard]] std::vector<std::string>
    get_vertex_parameterization_quantity_names() const
    {
        std::vector<std::string> names;
        for (const auto& pair : vertex_parameterization_quantities) {
            names.push_back(pair.first);
        }
        return names;
    }

    void set_vertices(const std::vector<glm::vec3>& vertices)
    {
        this->vertices = vertices;
    }

    void set_face_vertex_counts(const std::vector<int>& face_vertex_counts)
    {
        this->faceVertexCounts = face_vertex_counts;
    }

    void set_face_vertex_indices(const std::vector<int>& face_vertex_indices)
    {
        this->faceVertexIndices = face_vertex_indices;
    }

    void set_normals(const std::vector<glm::vec3>& normals)
    {
#if USE_USD_SCRATCH_BUFFER
        mesh.CreateNormalsAttr().Set(normals);
#else
        this->normals = normals;
#endif
    }

    void set_texcoords_array(const std::vector<glm::vec2>& texcoords_array)
    {
#if USE_USD_SCRATCH_BUFFER
        auto PrimVarAPI = pxr::UsdGeomPrimvarsAPI(mesh);
        auto primvar = PrimVarAPI.CreatePrimvar(
            pxr::TfToken("UVMap"), pxr::SdfValueTypeNames->TexCoord2fArray);
        primvar.Set(texcoords_array);

        if (get_texcoords_array().size() == get_vertices().size()) {
            primvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
        }
        else {
            primvar.SetInterpolation(pxr::UsdGeomTokens->faceVarying);
        }
#else
        this->texcoordsArray = texcoords_array;
#endif
    }

    void set_display_color(const std::vector<glm::vec3>& display_color)
    {
#if USE_USD_SCRATCH_BUFFER
        auto PrimVarAPI = pxr::UsdGeomPrimvarsAPI(mesh);
        pxr::UsdGeomPrimvar colorPrimvar = PrimVarAPI.CreatePrimvar(
            pxr::TfToken("displayColor"), pxr::SdfValueTypeNames->Color3fArray);
        colorPrimvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
        colorPrimvar.Set(display_color);
#else
        this->displayColor = display_color;
#endif
    }

    void set_vertex_scalar_quantities(
        const std::map<std::string, std::vector<float>>& scalar)
    {
        vertex_scalar_quantities = scalar;
    }

    void set_face_scalar_quantities(
        const std::map<std::string, std::vector<float>>& scalar)
    {
        face_scalar_quantities = scalar;
    }

    void set_vertex_vector_quantities(
        const std::map<std::string, std::vector<glm::vec3>>& vector)
    {
        vertex_vector_quantities = vector;
    }

    void set_face_vector_quantities(
        const std::map<std::string, std::vector<glm::vec3>>& vector)
    {
        face_vector_quantities = vector;
    }

    void set_face_corner_parameterization_quantities(
        const std::map<std::string, std::vector<glm::vec2>>& parameterization)
    {
        face_corner_parameterization_quantities = parameterization;
    }

    void set_vertex_parameterization_quantities(
        const std::map<std::string, std::vector<glm::vec2>>& parameterization)
    {
        vertex_parameterization_quantities = parameterization;
    }

    void add_vertex_scalar_quantity(
        const std::string& name,
        const std::vector<float>& scalar)
    {
        vertex_scalar_quantities[name] = scalar;
    }

    void add_face_scalar_quantity(
        const std::string& name,
        const std::vector<float>& scalar)
    {
        face_scalar_quantities[name] = scalar;
    }

    void add_vertex_vector_quantity(
        const std::string& name,
        const std::vector<glm::vec3>& vector)
    {
        vertex_vector_quantities[name] = vector;
    }

    void add_face_vector_quantity(
        const std::string& name,
        const std::vector<glm::vec3>& vector)
    {
        face_vector_quantities[name] = vector;
    }

    void add_face_corner_parameterization_quantity(
        const std::string& name,
        const std::vector<glm::vec2>& parameterization)
    {
        face_corner_parameterization_quantities[name] = parameterization;
    }

    void add_vertex_parameterization_quantity(
        const std::string& name,
        const std::vector<glm::vec2>& parameterization)
    {
        vertex_parameterization_quantities[name] = parameterization;
    }

    void append_mesh(const std::shared_ptr<MeshComponent>& mesh);

   private:
    // Local cache for mesh attributes when USD cache is not enabled
    std::vector<glm::vec3> vertices;
    std::vector<int> faceVertexCounts;
    std::vector<int> faceVertexIndices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> displayColor;
    std::vector<glm::vec2> texcoordsArray;

    // After adding these quantities, you need to modify the copy() function

    // Quantities for polyscope
    // Edge quantities are not supported because the indexing is not clear
    std::map<std::string, std::vector<float>> vertex_scalar_quantities;
    std::map<std::string, std::vector<float>> face_scalar_quantities;
    std::map<std::string, std::vector<glm::vec3>> vertex_vector_quantities;
    std::map<std::string, std::vector<glm::vec3>> face_vector_quantities;
    std::map<std::string, std::vector<glm::vec2>>
        face_corner_parameterization_quantities;
    std::map<std::string, std::vector<glm::vec2>>
        vertex_parameterization_quantities;

    // View lock mechanism
    mutable bool has_active_view_ = false;

    friend struct ConstMeshIGLView;
    friend struct MeshIGLView;
    friend struct ConstMeshUSDView;
    friend struct MeshUSDView;
    friend struct ConstMeshCUDAView;
    friend struct MeshCUDAView;
    friend struct ConstMeshNVRHIView;
    friend struct MeshNVRHIView;

    void acquire_view_lock() const;
    void release_view_lock() const;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
