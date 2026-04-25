#pragma once
#include "GCore/Components.h"
#include "GCore/api.h"
#include "pxr/base/vt/array.h"
#include "glm/gtc/quaternion.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE

class GEOMETRY_API InstancerComponent final : public GeometryComponent {
   public:
    explicit InstancerComponent(Geometry* attached_operand);

    ~InstancerComponent() override;
    GeometryComponentHandle copy(Geometry* operand) const override;
    std::string to_string() const override;

    size_t hash() const override
    {
        size_t h = 0;
        for (size_t i = 0; i < positions_.size(); ++i) {
            h ^= std::hash<float>{}(positions_[i].x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(positions_[i].y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(positions_[i].z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    void apply_transform(const glm::mat4& transform) override;

    void add_instance(const glm::vec3& position, const glm::quat& orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), const glm::vec3& scale = glm::vec3(1.0f));
    
    // Batch add instances for better performance
    void add_instances(const std::vector<glm::vec3>& positions, const std::vector<glm::quat>& orientations, const std::vector<glm::vec3>& scales);
    
    // Set instance data independently (arrays must be either empty or match positions size)
    void set_positions(const std::vector<glm::vec3>& positions);
    void set_orientations(const std::vector<glm::quat>& orientations); // Can be empty
    void set_scales(const std::vector<glm::vec3>& scales); // Can be empty
    
    // Reserve space for instances to avoid reallocations
    void reserve_instances(size_t count);
    
    size_t get_instance_count() const { return positions_.size(); }
    const std::vector<glm::vec3>& get_positions() const { return positions_; }
    const std::vector<glm::quat>& get_orientations() const { return orientations_; }
    const std::vector<glm::vec3>& get_scales() const { return scales_; }
    pxr::VtArray<int> get_proto_indices() const;

    bool has_rotations_enabled() const
    {
        return has_rotations;
    }
    void set_has_rotations_enabled(bool enabled)
    {
        has_rotations = enabled;
    }

   private:
    bool has_rotations = false;
    std::vector<glm::vec3> positions_;
    std::vector<glm::quat> orientations_;
    std::vector<glm::vec3> scales_;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
