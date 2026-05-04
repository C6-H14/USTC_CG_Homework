#pragma once

#include "GCore/api.h"
#include "GOP.h"
#include "glm/glm.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
struct GEOMETRY_API GeometryComponent {
    virtual ~GeometryComponent();

    explicit GeometryComponent(Geometry* attached_operand);

    virtual GeometryComponentHandle copy(Geometry* operand) const = 0;
    virtual std::string to_string() const = 0;
    virtual size_t hash() const = 0;

    [[nodiscard]] Geometry* get_attached_operand() const;

    virtual void apply_transform(const glm::mat4& transform) = 0;

   protected:
    Geometry* attached_operand;
#if USE_USD_SCRATCH_BUFFER
    pxr::SdfPath scratch_buffer_path;
#endif
    friend class Geometry;
};

RUZINO_NAMESPACE_CLOSE_SCOPE
