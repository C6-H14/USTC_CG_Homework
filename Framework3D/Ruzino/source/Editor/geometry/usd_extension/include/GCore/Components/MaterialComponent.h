#pragma once

#include <string>
#include <pxr/usd/usdShade/material.h>

#include "GCore/Components.h"
#include "GCore/GOP.h"

RUZINO_NAMESPACE_OPEN_SCOPE
struct GEOMETRY_API MaterialComponent : public GeometryComponent {
    explicit MaterialComponent(Geometry* attached_operand)
        : GeometryComponent(attached_operand)
    {
    }

    void apply_transform(const glm::mat4& transform) override
    {
    }

    size_t hash() const override
    {
        size_t h = 0;
        for (const auto& texture : textures) {
            h ^= std::hash<std::string>{}(texture) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    GeometryComponentHandle copy(Geometry* operand) const override
    {
        auto ret = std::make_shared<MaterialComponent>(operand);

        // This is fast because the VtArray has the copy on write mechanism
        ret->textures = this->textures;
        return ret;
    }

    std::string to_string() const override
    {
        return {};
    }

    pxr::UsdShadeMaterial define_material(
        pxr::UsdStageRefPtr stage,
        pxr::SdfPath path) const;

    void set_materialx_path(pxr::SdfPath path)
    {
        mtlx_material_path = path;
    }

    pxr::SdfPath get_material_path() const;

    std::vector<std::string> textures;

    pxr::SdfPath mtlx_material_path;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
