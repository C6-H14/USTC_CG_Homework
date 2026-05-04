#include "GCore/Components/XformComponent.h"

#include "glm/ext/matrix_transform.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
GeometryComponentHandle XformComponent::copy(Geometry* operand) const
{
    auto ret = std::make_shared<XformComponent>(attached_operand);
    ret->attached_operand = operand;

    ret->rotation = rotation;
    ret->translation = translation;
    ret->scale = scale;

    return ret;
}

std::string XformComponent::to_string() const
{
    return std::string("XformComponent");
}

void XformComponent::apply_transform(const glm::mat4& transform)
{
    // Clear the transform chain
    translation.clear();
    scale.clear();
    rotation.clear();
}
bool XformComponent::is_identity() const
{
    return translation.empty() && scale.empty() && rotation.empty();
}

glm::mat4 XformComponent::get_transform() const
{
    assert(translation.size() == rotation.size());
    glm::mat4 final_transform = glm::mat4(1.0f);
    for (int i = 0; i < translation.size(); ++i) {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), translation[i]);
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale[i]);
        glm::mat4 r_x = glm::rotate(
            glm::mat4(1.0f), glm::radians(rotation[i][0]), glm::vec3(1, 0, 0));
        glm::mat4 r_y = glm::rotate(
            glm::mat4(1.0f), glm::radians(rotation[i][1]), glm::vec3(0, 1, 0));
        glm::mat4 r_z = glm::rotate(
            glm::mat4(1.0f), glm::radians(rotation[i][2]), glm::vec3(0, 0, 1));
        auto transform = t * r_z * r_y * r_x * s;
        final_transform = final_transform * transform;
    }
    return final_transform;
}

RUZINO_NAMESPACE_CLOSE_SCOPE
