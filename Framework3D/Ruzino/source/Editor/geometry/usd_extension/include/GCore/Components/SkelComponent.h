#pragma once
#include <pxr/usd/usdSkel/skeleton.h>

#include <string>

#include "GCore/Components.h"
#include "GCore/GOP.h"

RUZINO_NAMESPACE_OPEN_SCOPE
struct GEOMETRY_API SkelComponent : public GeometryComponent {
    explicit SkelComponent(Geometry* attached_operand)
        : GeometryComponent(attached_operand)
    {
    }

    void apply_transform(const glm::mat4& transform) override
    {
        throw std::runtime_error("Not implemented");
    }

    size_t hash() const override
    {
        // Simple hash implementation for skeleton component
        size_t h = 0;
        h ^= std::hash<size_t>{}(jointOrder.size()) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<size_t>{}(localTransforms.size()) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<size_t>{}(bindTransforms.size()) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }

    std::string to_string() const override;

    pxr::VtTokenArray jointOrder;
    pxr::UsdSkelTopology topology;
    pxr::VtArray<pxr::GfMatrix4f> localTransforms;
    pxr::VtArray<pxr::GfMatrix4d> bindTransforms;
    pxr::VtArray<float> jointWeight;
    pxr::VtArray<int> jointIndices;

    GeometryComponentHandle copy(Geometry* operand) const override;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
