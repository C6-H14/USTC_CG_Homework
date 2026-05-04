#include <GCore/Components/InstancerComponent.h>

#include "glm/gtx/matrix_decompose.hpp"
#include "pxr/base/vt/array.h"


RUZINO_NAMESPACE_OPEN_SCOPE
InstancerComponent::InstancerComponent(Geometry* attached_operand)
    : GeometryComponent(attached_operand)
{
}

InstancerComponent::~InstancerComponent()
{
}

GeometryComponentHandle InstancerComponent::copy(Geometry* operand) const
{
    auto instancer = std::make_shared<InstancerComponent>(operand);
    instancer->positions_ = positions_;
    instancer->orientations_ = orientations_;
    instancer->scales_ = scales_;
    instancer->has_rotations = has_rotations;
    return instancer;
}

std::string InstancerComponent::to_string() const
{
    return "InstancerComponent";
}

void InstancerComponent::apply_transform(const glm::mat4& transform)
{
    // Decompose the transform
    glm::vec3 t_translation, t_scale, t_skew;
    glm::quat t_rotation;
    glm::vec4 t_perspective;
    glm::decompose(
        transform, t_scale, t_rotation, t_translation, t_skew, t_perspective);

    // Apply to all instances
    for (size_t i = 0; i < positions_.size(); ++i) {
        positions_[i] = glm::vec3(transform * glm::vec4(positions_[i], 1.0f));
        orientations_[i] = t_rotation * orientations_[i];
        scales_[i] *= t_scale;
    }
}

void InstancerComponent::add_instance(
    const glm::vec3& position,
    const glm::quat& orientation,
    const glm::vec3& scale)
{
    positions_.push_back(position);
    orientations_.push_back(orientation);
    scales_.push_back(scale);
}

void InstancerComponent::add_instances(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::quat>& orientations,
    const std::vector<glm::vec3>& scales)
{
    // Arrays must be either empty or match positions size
    assert(orientations.empty() || orientations.size() == positions.size());
    assert(scales.empty() || scales.size() == positions.size());

    positions_.insert(positions_.end(), positions.begin(), positions.end());
    
    if (!orientations.empty()) {
        orientations_.insert(
            orientations_.end(), orientations.begin(), orientations.end());
    }
    
    if (!scales.empty()) {
        scales_.insert(scales_.end(), scales.begin(), scales.end());
    }
}

void InstancerComponent::set_positions(const std::vector<glm::vec3>& positions)
{
    positions_ = positions;
}

void InstancerComponent::set_orientations(const std::vector<glm::quat>& orientations)
{
    assert(orientations.empty() || orientations.size() == positions_.size());
    orientations_ = orientations;
}

void InstancerComponent::set_scales(const std::vector<glm::vec3>& scales)
{
    assert(scales.empty() || scales.size() == positions_.size());
    scales_ = scales;
}

void InstancerComponent::reserve_instances(size_t count)
{
    positions_.reserve(count);
    orientations_.reserve(count);
    scales_.reserve(count);
}

pxr::VtArray<int> InstancerComponent::get_proto_indices() const
{
    return pxr::VtArray<int>(positions_.size(), 0);
}

RUZINO_NAMESPACE_CLOSE_SCOPE
