#pragma once

#include "GCore/Components.h"
#include "GCore/api.h"

RUZINO_NAMESPACE_OPEN_SCOPE
// Stores the chain of transformation

class GEOMETRY_API XformComponent : public GeometryComponent {
   public:
    GeometryComponentHandle copy(Geometry* operand) const override;
    std::string to_string() const override;

    size_t hash() const override
    {
        size_t h = 0;
        for (const auto& t : translation) {
            h ^= std::hash<float>{}(t.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(t.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(t.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (const auto& s : scale) {
            h ^= std::hash<float>{}(s.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(s.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(s.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        for (const auto& r : rotation) {
            h ^= std::hash<float>{}(r.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(r.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(r.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    explicit XformComponent(Geometry* attached_operand)
        : GeometryComponent(attached_operand)
    {
    }

    void apply_transform(const glm::mat4& transform) override;

    bool is_identity() const;

    glm::mat4 get_transform() const;

    std::vector<glm::vec3> translation;
    std::vector<glm::vec3> scale;
    std::vector<glm::vec3> rotation;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
