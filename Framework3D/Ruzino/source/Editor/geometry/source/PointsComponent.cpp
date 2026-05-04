// #undef _MSC_VER
#include "GCore/Components/PointsComponent.h"

#include <sstream>

#include "GCore/GOP.h"
#include "global_stage.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
PointsComponent::PointsComponent(Geometry* attached_operand)
    : GeometryComponent(attached_operand)
{
}

std::string PointsComponent::to_string() const
{
    std::ostringstream out;
    // Loop over the vertices and print the data
    out << "Points component. "
        << "Vertices count " << get_vertices().size()
        << ". Face vertices count "
        << ".";
    return out.str();
}

void PointsComponent::apply_transform(const glm::mat4& transform)
{
    auto vertices = get_vertices();
    for (auto& vertex : vertices) {
        glm::vec4 transformed = transform * glm::vec4(vertex, 1.0f);
        vertex = glm::vec3(transformed);
    }

    auto normals = get_normals();
    if (!normals.empty()) {
        // Transform normals with the inverse transpose to preserve
        // orthogonality
        glm::mat3 normalTransform =
            glm::transpose(glm::inverse(glm::mat3(transform)));
        for (auto& normal : normals) {
            normal = glm::normalize(normalTransform * normal);
        }
        set_normals(normals);
    }

    set_vertices(vertices);
}

GeometryComponentHandle PointsComponent::copy(Geometry* operand) const
{
    auto ret = std::make_shared<PointsComponent>(operand);

    ret->set_vertices(this->get_vertices());
    ret->set_display_color(this->get_display_color());
    ret->set_normals(this->get_normals());
    ret->set_width(this->get_width());

    return ret;
}

RUZINO_NAMESPACE_CLOSE_SCOPE
