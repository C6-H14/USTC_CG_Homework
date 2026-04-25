#pragma once
#include <glm/glm.hpp>
#include <string>

#include "GCore/Components.h"
#include "GCore/GOP.h"

RUZINO_NAMESPACE_OPEN_SCOPE
struct GEOMETRY_API CurveComponent : public GeometryComponent {
    explicit CurveComponent(Geometry* attached_operand);

    std::string to_string() const override;

    size_t hash() const override
    {
        size_t h = 0;
        for (const auto& v : vertices) {
            h ^= std::hash<float>{}(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (const auto& w : width) {
            h ^= std::hash<float>{}(w) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (const auto& c : vert_count) {
            h ^= std::hash<int>{}(c) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        h ^= std::hash<bool>{}(periodic) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(static_cast<int>(curve_type)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }

    void apply_transform(const glm::mat4& transform) override
    {
        auto vertices = get_vertices();
        for (auto& vertex : vertices) {
            vertex = glm::vec3(transform * glm::vec4(vertex, 1.0));
        }
        set_vertices(vertices);
    }

    [[nodiscard]] std::vector<glm::vec3> get_vertices() const
    {
        return vertices;
    }

    void set_vertices(const std::vector<glm::vec3>& vertices)
    {
        this->vertices = vertices;
    }

    [[nodiscard]] std::vector<float> get_width() const
    {
        return width;
    }

    void set_width(const std::vector<float>& width)
    {
        this->width = width;
    }

    // Alias for compatibility
    [[nodiscard]] std::vector<float> get_widths() const
    {
        return width;
    }

    void set_widths(const std::vector<float>& widths)
    {
        this->width = widths;
    }

    [[nodiscard]] std::vector<int> get_vert_count() const
    {
        return vert_count;
    }

    void set_vert_count(const std::vector<int>& vert_count)
    {
        this->vert_count = vert_count;
    }

    // Alias for compatibility
    [[nodiscard]] std::vector<int> get_curve_counts() const
    {
        return vert_count;
    }

    void set_curve_counts(const std::vector<int>& counts)
    {
        this->vert_count = counts;
    }

    [[nodiscard]] std::vector<glm::vec3> get_display_color() const
    {
        return displayColor;
    }

    void set_display_color(const std::vector<glm::vec3>& display_color)
    {
        this->displayColor = display_color;
    }

#if USE_USD_SCRATCH_BUFFER
    pxr::UsdGeomBasisCurves get_usd_curve() const
    {
        return curves;
    }
#endif

    [[nodiscard]] bool get_periodic() const
    {
        return periodic;
    }

    void set_periodic(bool is_periodic)
    {
        periodic = is_periodic;
    }

    [[nodiscard]] std::vector<glm::vec3> get_curve_normals() const
    {
        return curve_normals;
    }

    void set_curve_normals(const std::vector<glm::vec3>& normals)
    {
        curve_normals = normals;
    }

    GeometryComponentHandle copy(Geometry* operand) const override;

    enum class CurveType {
        Linear,
        Cubic,

    } curve_type = CurveType::Linear;
    [[nodiscard]] CurveType get_type() const
    {
        return curve_type;
    }

    void set_type(CurveType type)
    {
        curve_type = type;
    }

   private:
    std::vector<glm::vec3> vertices;
    std::vector<float> width;
    std::vector<int> vert_count;
    std::vector<glm::vec3> displayColor;

    bool periodic = false;
    std::vector<glm::vec3> curve_normals;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
